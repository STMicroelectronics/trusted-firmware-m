/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 * Copyright (c) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <psa/client.h>
#include <psa_manifest/sid.h>
#include <uapi/tfm_adac_api.h>

psa_status_t tfm_adac_service(void)
{
	return psa_call(TFM_ADAC_SERVICE_HANDLE,
			PSA_IPC_CALL,
			NULL,
			0,
			NULL,
			0);
}
