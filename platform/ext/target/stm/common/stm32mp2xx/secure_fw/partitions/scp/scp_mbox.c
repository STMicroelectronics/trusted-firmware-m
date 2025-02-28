/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */
#define DT_DRV_COMPAT st_scp_mbox

#include <device.h>
#include <mbox.h>
#include "region_defs.h"
#include "psa_manifest/pid.h"
#include "psa_manifest/sid.h"

#include <psa/service.h>
#include <spm.h>
#include "tfm_hal_interrupt.h"
#include "load/interrupt_defs.h"
#include "psa_manifest/tfm_scp.h"
#include "scmi_server.h"

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
const struct mbox_dt_spec channel = MBOX_DT_SPEC_INST_GET(0, ns);
/* function called by mailbox SLIH handler */
void rx_scp(const struct device *dev,
	    mbox_channel_id_t channel_id, void *user_data,
	    struct mbox_msg *data)
{
	psa_call(TFM_SCP_SERVICE_HANDLE, TFM_SCP_SERVICE_SID, NULL, 0, NULL, 0);
}


void scp_com_handle(void)
{
	scmi_server_smt_process_thread(DT_PROP(DT_NODELABEL(scmi_ca35),agent_id) - 1);
	mbox_send_dt(&channel, NULL);
}

void scp_com_init(void)
{
	if (mbox_register_callback_dt(&channel, rx_scp, NULL)) {
		return;
	}

	if (mbox_set_enabled_dt(&channel, true)) {
		return;
	}
}
#endif
