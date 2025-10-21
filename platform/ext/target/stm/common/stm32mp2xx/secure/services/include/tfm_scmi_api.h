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

enum scmi_sys_power {
	SYS_POWER_SHUTDOWN = 0,
	SYS_POWER_COLD_RESET = 1,
	SYS_POWER_WARM_RESET = 2,
	SYS_POWER_SUSPEND = 4
};

psa_status_t tfm_scmi_req(void *req, size_t req_len, void *rsp, size_t rsp_len);
psa_status_t tfm_secure_scmi_req(uint32_t agent_id);
psa_status_t tfm_secure_scmi_get_notif(void *rsp, size_t rsp_len);
void tfm_sys_power_state_notifier(uint32_t agent_id, bool graceful, enum scmi_sys_power event);

#endif

