/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */
#include <stdbool.h>
#include <arch_main.h>
#include "tfm_sp_log.h"
#include "tfm_hal_defs.h"
#include "uart_stdout.h"
#include <string.h>
#include "psa/framework_feature.h"
#include "psa/service.h"
#include "tfm_scmi.h"
#include <assert.h>
#include "scmi_server.h"
#include "psa_manifest/tfm_scp.h"
#include "psa/service.h"
void __panic(void)
{
	psa_panic();
}

extern void scp_com_handle(int type);

psa_status_t tfm_scp_entry(void)
{
	int ret = 0;

	ret = scmi_scpfw_cfg_early_init();
	assert(ret == TFM_SCMI_SUCCESS);
	ret = scmi_scpfw_cfg_init();
	assert(ret == TFM_SCMI_SUCCESS);
	LOG_DBGFMT("\r\nlaunch scmi_server_initialize\r\n");
	ret = scmi_server_initialize();
	assert(ret == TFM_SCMI_SUCCESS);

	return PSA_SUCCESS;
}

psa_status_t tfm_scp_service_sfn(const psa_msg_t *msg)
{
	int type = msg->type;
	scp_com_handle(type);
	return PSA_SUCCESS;
}
