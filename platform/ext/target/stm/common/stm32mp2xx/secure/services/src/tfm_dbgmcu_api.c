/*
 * Copyright (c) 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <psa/client.h>
#include <psa_manifest/sid.h>
#include <uapi/tfm_dbgmcu_api.h>

psa_status_t tfm_dbgmcu_service(void)
{
	return psa_call(TFM_DBGMCU_SERVICE_HANDLE,
			PSA_IPC_CALL,
			NULL,
			0,
			NULL,
			0);
}
