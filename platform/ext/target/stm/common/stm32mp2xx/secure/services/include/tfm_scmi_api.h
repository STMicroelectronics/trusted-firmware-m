// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 */
#ifndef TFM_SCMI_API_H
#define TFM_SCMI_API_H
#include "psa/client.h"
/*  psa ret */
enum psa_scmi_err_t {
	/* a scmi notif received */
	PSA_SCMI_ERR_SUCCESS = 0,
	/* no scmi notif received*/
	PSA_SCMI_ERR_BUFFER_EMPTY,
	/* a scmi notif has been received, and at least on scmi notif has been lost */
	PSA_SCMI_ERR_BUFFER_OVERFLOW,
};

psa_status_t tfm_scmi_req(void *req, size_t req_len, void *rsp, size_t rsp_len);
psa_status_t tfm_secure_scmi_req(uint32_t agent_id);
psa_status_t tfm_secure_scmi_get_notif(void *rsp, size_t rsp_len);
#endif

