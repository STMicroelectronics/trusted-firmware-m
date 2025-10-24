/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <errno.h>
#include <pm/device.h>
#include <pm/pm.h>
#include <psa/error.h>
#include <psa/service.h>
#include <stm32_dcache.h>
#include <stm32mp2_lp_fw_api.h>
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>

#include "critical_section.h"

#define STM32_PM_HINT PM_HINT_POWER_STATE | PM_HINT_CLOCK_STATE | PM_HINT_IO_STATE

typedef struct context {
	uint32_t VTOR;
	uint32_t MSPLIM;
	uint32_t PSPLIM;
	uint32_t CONTROL;
	uint32_t FPSCR;
	uint32_t MSP;
	uint32_t PSP;
	uint32_t primask_s;
} cm33_context_t;

cm33_context_t tfm_context;

void save_it_status(void)
{
	tfm_context.primask_s = __get_PRIMASK();
	tfm_context.VTOR = SCB->VTOR;
	__set_PRIMASK(1);
}

void restore_it_status(void)
{
	SCB->VTOR = tfm_context.VTOR;
	__DSB();
	__ISB();

	__set_PRIMASK(tfm_context.primask_s);
}

void jump_low_power_fw(enum pm_suspend_mode_t lpmode)
{
	stm32mp2_lp_fw_suspend_mode_t lpfwmode;

	switch(lpmode)
	{
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
		break;
	default:
		/* Unexpected value */
		return;
	}

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
		if (stm32_dcache_clean(0x0, 0xFFFFFFFF))
			return;

		if (stm32_dcache_disable())
			return;
	}

	stm32mp2_lp_fw_exec();

	/* restore tfm execution context */
	__set_MSPLIM(tfm_context.MSPLIM);
	__set_PSPLIM(tfm_context.PSPLIM);
	__set_PSP(tfm_context.PSP);
	__set_MSP(tfm_context.MSP);
	__set_CONTROL(tfm_context.CONTROL);
	__set_FPSCR(tfm_context.FPSCR);

	restore_it_status();
}

static int _pm_suspend(enum pm_suspend_mode_t mode)
{
	struct critical_section_t cs_assert = CRITICAL_SECTION_STATIC_INIT;
	int err = 0;

	/* prepare suspend context*/

	/* IT: mask, wakeup, fault ?*/
	/* cache ? */
	/* mpu ? */

	CRITICAL_SECTION_ENTER(cs_assert);

	/* call suspend of each device */
	if (!pm_suspend_devices(STM32_PM_HINT)) {
		pm_resume_devices(STM32_PM_HINT);
		err = -EINVAL;
		goto out;
	}

	jump_low_power_fw(mode);

	pm_resume_devices(STM32_PM_HINT);

	if (IS_ENABLED(STM32_CACHE_ENABLED))
		err = stm32_dcache_enable(true, true);

out:
	CRITICAL_SECTION_LEAVE(cs_assert);
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
	stm32mp2_lp_fw_set_lpmode(STM32MP2_LP_FW_LPMODE_OFF);
	stm32mp2_lp_fw_mark_data_valid();

	/* Clean cache in order to prevent unsynchronized shared data */
	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		if (stm32_dcache_clean(0x0, 0xFFFFFFFF))
			return PSA_ERROR_GENERIC_ERROR;

		if (stm32_dcache_disable())
			return PSA_ERROR_GENERIC_ERROR;
	}

	stm32mp2_lp_fw_exec();

	return PSA_SUCCESS;
}
