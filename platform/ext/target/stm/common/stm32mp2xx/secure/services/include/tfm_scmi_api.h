// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 */
#ifndef TFM_SCMI_API_H
#define TFM_SCMI_API_H
#include "psa/client.h"
psa_status_t tfm_scmi_req(void *req, size_t req_len, void *rsp, size_t rsp_len);
#endif /* TFM_SCMI_API_H  */
