/*
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <psa/client.h>
#include <psa_manifest/sid.h>
#include <uapi/tfm_pm_api.h>

psa_status_t psa_pm_suspend(enum pm_suspend_mode_t mode)
{
	struct tfm_pm_suspend_args_t args;
	psa_status_t status;
	psa_invec in_vec;

	in_vec.base = (const void *)&args;
	in_vec.len = sizeof(args);

	args.mode = mode;

	status = psa_call(TFM_PM_SERVICE_HANDLE, TFM_PM_SUSPEND,
			  &in_vec, 1, NULL, 0);

	return status;
}

psa_status_t psa_pm_power_off(void)
{
	return psa_call(TFM_PM_SERVICE_HANDLE, TFM_PM_POWER_OFF, NULL, 0, NULL, 0);
}

