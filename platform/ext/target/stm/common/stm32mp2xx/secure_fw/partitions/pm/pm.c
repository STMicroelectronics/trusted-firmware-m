/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <critical_section.h>
#include <errno.h>
#include <pm/device.h>
#include <pm/pm.h>
#include <psa/error.h>
#include <psa/service.h>
#include <stm32_dcache.h>
#include <stm32mp2_lp_fw_api.h>
#include <stm32mp2_ramcfg.h>
#include <tfm_arch.h>
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>

typedef struct context {
	uint32_t VTOR;
	uint32_t MSPLIM;
	uint32_t PSPLIM;
	uint32_t CONTROL;
	uint32_t FPSCR;
	uint32_t MSP;
	uint32_t PSP;
	uint32_t BASEPRI;
} cm33_context_t;

cm33_context_t tfm_context;

static void save_it_status(void)
{
	tfm_context.BASEPRI = __get_BASEPRI();
	tfm_context.VTOR = SCB->VTOR;

	__set_BASEPRI(0);
}

static void restore_it_status(void)
{
	SCB->VTOR = tfm_context.VTOR;
	__DSB();
	__ISB();

	__set_BASEPRI(tfm_context.BASEPRI);
}

static int jump_low_power_fw(stm32mp2_lp_fw_suspend_mode_t lpfwmode)
{
	const struct device *ramcfg_retram = DT_RAMCFG_DEVICE(retram);
	int res;
	int err;

	save_it_status();

	/* save tfm execution context */
	tfm_context.MSPLIM = __get_MSPLIM();
	tfm_context.PSPLIM = __get_PSPLIM();
	tfm_context.PSP = __get_PSP();
	tfm_context.MSP = __get_MSP();
	tfm_context.CONTROL = __get_CONTROL();
	tfm_context.FPSCR = __get_FPSCR();

	stm32mp2_lp_fw_set_lpmode(lpfwmode);
	stm32mp2_lp_fw_mark_data_valid();

	/* Clean cache in order to prevent unsynchronized shared data */
	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		res = stm32_dcache_clean(0x0, 0xFFFFFFFF);
		if (res)
			goto cache_clean_error;

		res = stm32_dcache_disable();
		if (res)
			goto cache_error;
	}

	/*
	 * CRC and signature check is enabled only during Standby
	 * to avoid LP FW execution on cortex-M33 reset
	 */
	if (lpfwmode == STM32MP2_LP_FW_LPMODE_STANDBY1)
		stm32_ramcfg_crc_enable(ramcfg_retram);

	res = stm32mp2_lp_fw_exec();

	stm32_ramcfg_crc_disable(ramcfg_retram);

	/* restore tfm execution context after stm32mp2_lp_fw_exec() */
	__set_MSPLIM(tfm_context.MSPLIM);
	__set_PSPLIM(tfm_context.PSPLIM);
	__set_PSP(tfm_context.PSP);
	__set_MSP(tfm_context.MSP);
	__set_CONTROL(tfm_context.CONTROL);
	__set_FPSCR(tfm_context.FPSCR);

cache_error:
	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		err = stm32_dcache_enable(true, true);
		if (err && !res)
			res = err;
	}

cache_clean_error:
	restore_it_status();

	return res;
}

static int _pm_suspend(enum pm_suspend_mode_t mode)
{
	struct critical_section_t cs_assert = CRITICAL_SECTION_STATIC_INIT;
	stm32mp2_lp_fw_suspend_mode_t lpfwmode;
	uint32_t pm_hint = PM_HINT_CLOCK_STATE; /* default for Stop2 modes */
	int err = 0;

	switch(mode) {
	case PM_STOP2:
		lpfwmode = STM32MP2_LP_FW_LPMODE_STOP2;
		break;
	case PM_LP_STOP2:
		lpfwmode = STM32MP2_LP_FW_LPMODE_LP_STOP2;
		break;
	case PM_LPLV_STOP2:
		lpfwmode = STM32MP2_LP_FW_LPMODE_LPLV_STOP2;
		break;
	case PM_STANDBY1:
		lpfwmode = STM32MP2_LP_FW_LPMODE_STANDBY1;
		pm_hint = PM_HINT_CLOCK_STATE | PM_HINT_CONTEXT_STATE;
		break;
	default:
		return -EINVAL;
	};

	/* platform state = mode, used by some driver as STPMIC2 */
	pm_hint |= mode << PM_HINT_PLATFORM_STATE_SHIFT;

	/* Mask interruptions but allow TF-M scheduling during driver suspend */
	__set_BASEPRI(PENDSV_PRIO_FOR_SCHED);

	/* call suspend of each device */
	if (pm_suspend_devices(pm_hint)) {
		CRITICAL_SECTION_ENTER(cs_assert);
		err = jump_low_power_fw(lpfwmode);
		CRITICAL_SECTION_LEAVE(cs_assert);
	} else {
		err = -EINVAL;
	}

	pm_resume_devices(pm_hint);

	__set_BASEPRI(0);

	return err;
}

psa_status_t tfm_pm_suspend(const psa_msg_t *msg)
{
	struct tfm_pm_suspend_args_t args;
	size_t suspend_args_sz;
	uint32_t bytes_read;
	int err;

	suspend_args_sz = msg->in_size[0];

	/* Check input parameters. */
	if (suspend_args_sz != sizeof(args))
		return PSA_ERROR_INVALID_ARGUMENT;

	bytes_read = psa_read(msg->handle, 0, &args, suspend_args_sz);
	if (bytes_read != suspend_args_sz)
		return PSA_ERROR_GENERIC_ERROR;

	err = _pm_suspend(args.mode);
	if (err)
		return PSA_ERROR_GENERIC_ERROR;

	return PSA_SUCCESS;
}

psa_status_t tfm_pm_power_off(void)
{
	/* call suspend of each device */
	pm_suspend_devices(PM_HINT_CLOCK_STATE | PM_HINT_PLATFORM_STATE_MASK);

	stm32mp2_lp_fw_set_lpmode(STM32MP2_LP_FW_LPMODE_OFF);
	stm32mp2_lp_fw_mark_data_valid();

	/* Clean cache in order to prevent unsynchronized shared data */
	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		if (stm32_dcache_clean(0x0, 0xFFFFFFFF))
			return PSA_ERROR_GENERIC_ERROR;

		if (stm32_dcache_disable())
			return PSA_ERROR_GENERIC_ERROR;
	}

	if (stm32mp2_lp_fw_exec())
		return PSA_ERROR_GENERIC_ERROR;

	return PSA_SUCCESS;
}

psa_status_t tfm_pm_fw_init(void)
{
	const uintptr_t uart_addr = DT_REG_ADDR(DT_CHOSEN(stdout_device));

#if (TFM_PARTITION_LOG_LEVEL < TFM_PARTITION_LOG_LEVEL_INFO)
	/* Deactivate LP firmware trace */
	uart_addr = 0x0;
#endif

	/* Initialize the shared memory */
	stm32mp2_lp_fw_clear_data();

	/* Display LP version in INIT phase when uart is defined */
	stm32mp2_lp_fw_set_uart_addr(uart_addr);

	if (jump_low_power_fw(STM32MP2_LP_FW_LPMODE_INIT)) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	/* Debug trace in LP firmware only if UART device is not suspended */
	if (!IS_ENABLED(STM32_CONSOLE_NO_SUSPEND))
		stm32mp2_lp_fw_set_uart_addr(0x0);

	return PSA_SUCCESS;
}
