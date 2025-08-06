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
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>

#define STM32_PM_HINT PM_HINT_POWER_STATE | PM_HINT_CLOCK_STATE | PM_HINT_IO_STATE

extern const uint8_t __tfm_lp_fw_start[];

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

typedef void (*p_fw_fn)(void);

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
	p_fw_fn lp_fw_entry;

	save_it_status();

	/* save tfm execution context */
	tfm_context.MSPLIM = __get_MSPLIM();
	tfm_context.PSPLIM = __get_PSPLIM();
	tfm_context.PSP = __get_PSP();
	tfm_context.MSP = __get_MSP();
	tfm_context.CONTROL = __get_CONTROL();
	tfm_context.FPSCR = __get_FPSCR();

	lp_fw_entry = (p_fw_fn) *((uint32_t *)(__tfm_lp_fw_start + 4));
	lp_fw_entry();

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
	/* prepare suspend context*/

	/* IT: mask, wakeup, fault ?*/
	/* cache ? */
	/* mpu ? */

	/* call suspend of each device */
	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		if (!pm_suspend_devices(STM32_PM_HINT)) {
			pm_resume_devices(STM32_PM_HINT);
			return -EINVAL;
		}
	}

	/* jump_lp_fw */
	jump_low_power_fw(mode);

	pm_resume_devices(STM32_PM_HINT);

	return 0;
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
	return PSA_SUCCESS;
}
