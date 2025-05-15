/* Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <psa/error.h>
#include <psa/client.h>
#include <psa/service.h>
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>

#include "tfm_pm.h"

static psa_status_t tfm_pm_load_fw()
{
	return PSA_SUCCESS;
}

psa_status_t tfm_pm_service_sfn(const psa_msg_t *msg)
{
    switch (msg->type) {
    case TFM_PM_SUSPEND:
        return tfm_pm_suspend(msg);
    case TFM_PM_POWER_OFF:
        return tfm_pm_power_off();
    default:
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_ERROR_GENERIC_ERROR;
}

psa_status_t tfm_pm_init(void)
{
	return tfm_pm_load_fw();
}
