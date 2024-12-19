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

void __panic(void)
{
	psa_panic();
}

extern void scp_com_init(void);
extern void scp_com_handle(void);

psa_status_t tfm_scp_entry(void)
{
	int ret = 0;
	psa_signal_t signals = 0;

	scp_com_init();
	ret = scmi_scpfw_cfg_early_init();
	assert(ret == TFM_SCMI_SUCCESS);
	ret = scmi_scpfw_cfg_init();
	assert(ret == TFM_SCMI_SUCCESS);
	LOG_DBGFMT("\r\nlaunch scmi_server_initialize\r\n");
	ret = scmi_server_initialize();
	assert(ret == TFM_SCMI_SUCCESS);

	while (1) {
		LOG_DBGFMT("scp wait loop..\r\n");
		signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
		LOG_DBGFMT("receive notification..\r\n");
		if (signals & MAILBOX_SIGNAL) scp_com_handle();
	}

	return PSA_SUCCESS;
}
