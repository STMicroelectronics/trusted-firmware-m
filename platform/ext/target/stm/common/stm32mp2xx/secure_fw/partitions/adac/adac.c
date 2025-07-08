/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 * Copyright (c) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <psa/error.h>
#include <psa_adac_platform.h>
#include <tfm_plat_otp.h>
#include <util_macro.h>

#define ROTPK_SIZE 32

static uint8_t secure_debug_rotpk[ROTPK_SIZE];

psa_status_t adac_service_request(void)
{
	int rc;

	/* Authenticate incoming debug request */
	rc = psa_adac_start_secure_debug(secure_debug_rotpk, ROTPK_SIZE);
	if (rc != 0)
		/* Authentication failure */
		return PSA_ERROR_NOT_PERMITTED;

	return PSA_SUCCESS;
}

psa_status_t adac_sp_init(bool *is_service_enabled)
{
	enum tfm_plat_err_t err;

	*is_service_enabled = false;

	if (!IS_ENABLED(TFM_DUMMY_PROVISIONING)) { /* Skip LCS check if dummy provisioning */
		enum plat_otp_lcs_t lcs = PLAT_OTP_LCS_UNKNOWN;

		/* Read LCS from OTP */
		err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(lcs), (uint8_t *)&lcs);
		if (err != TFM_PLAT_ERR_SUCCESS)
			return PSA_ERROR_SERVICE_FAILURE;

		/* ADAC service is only enabled if device is in secure state */
		if (lcs != PLAT_OTP_LCS_SECURED)
			return PSA_ERROR_SERVICE_FAILURE;
	}

	err = tfm_plat_otp_read(PLAT_OTP_ID_SECURE_DEBUG_PK, ROTPK_SIZE, secure_debug_rotpk);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return PSA_ERROR_SERVICE_FAILURE;

	*is_service_enabled = true;

	return PSA_SUCCESS;
}
