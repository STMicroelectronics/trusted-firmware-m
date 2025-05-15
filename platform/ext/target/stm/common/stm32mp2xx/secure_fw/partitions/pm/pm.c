/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <errno.h>
#include <pm/device.h>
#include <pm/pm.h>
#include <psa/error.h>
#include <psa/service.h>
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>

#define STM32_PM_HINT PM_HINT_POWER_STATE | PM_HINT_CLOCK_STATE | PM_HINT_IO_STATE

static int _pm_suspend(enum pm_suspend_mode_t mode)
{
	/* prepare suspend context*/

	/* call suspend of each device */
	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		if (!pm_suspend_devices(STM32_PM_HINT)) {
			pm_resume_devices(STM32_PM_HINT);
			return -EINVAL;
		}
	}

	/* core suspend */

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
