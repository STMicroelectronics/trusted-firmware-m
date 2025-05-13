/*
 * Copyright (C) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>

#include "psa/client.h"
#include "psa_manifest/sid.h"
#include "tfm_scmi_api.h"

psa_status_t tfm_scmi_req(void *req, size_t req_len, void *rsp, size_t rsp_len )
{
	psa_status_t status;
	psa_outvec out_vec;
	psa_invec in_vec;

	in_vec.base = (const void *)req;
	in_vec.len = req_len;

	out_vec.base = (void *)rsp;
	out_vec.len = rsp_len;

	status = psa_call(TFM_SCP_SERVICE_NS_HANDLE, PSA_IPC_CALL, &in_vec, 1,
			  &out_vec, 1);
	return status;
}

psa_status_t tfm_secure_scmi_req(uint32_t agent_id)
{

	return psa_call(TFM_SCP_SERVICE_HANDLE, agent_id, NULL, 0,
			  NULL, 0);
}
