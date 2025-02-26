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

psa_status_t tfm_scp_service_ns_sfn(const psa_msg_t *msg)
{
	size_t in_sz = msg->in_size[0];
	size_t out_sz = msg->out_size[0];
	uint32_t in_buf[32] = {0};
	uint32_t out_buf[32] = {0};
	int ret = 0;

	if (in_sz >= sizeof(in_buf) || out_sz> sizeof(out_buf))
		return PSA_ERROR_NOT_SUPPORTED;
	if (psa_read(msg->handle, 0,in_buf, in_sz) != in_sz)
		return PSA_ERROR_GENERIC_ERROR;

	ret  =	scmi_server_msg_process_thread(0,
					       in_buf, in_sz,
					       out_buf, &out_sz);
	if (ret)
		return PSA_ERROR_GENERIC_ERROR;

	psa_write(msg->handle, 0, out_buf, out_sz);
	return ret;
}
