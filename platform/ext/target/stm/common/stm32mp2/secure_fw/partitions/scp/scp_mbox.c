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
#include <psa/service.h>
#include "spm_ipc.h"
#include "tfm_hal_interrupt.h"
#include "tfm_peripherals_def.h"
#include "load/interrupt_defs.h"
#include "psa_manifest/tfm_scp.h"
#include "scmi_server.h"

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
const struct mbox_dt_spec channel = MBOX_DT_SPEC_GET(DT_DRV_INST(0));

void rx_scp(const struct device *dev,
	    mbox_channel_id_t channel_id, void *user_data,
	    struct mbox_msg *data)
{
	scmi_server_smt_process_thread(DT_PROP(DT_NODELABEL(scmi_ca35),agent_id) - 1);
	mbox_send_dt(&channel, NULL);
}

extern struct irq_t mailbox_irq;

void scp_com_handle(void)
{
}

psa_flih_result_t mailbox_flih(void)
{
	IPCC_HANDLE_0();
	return PSA_FLIH_NO_SIGNAL;
}

void scp_com_init(void)
{
	if (mbox_register_callback_dt(&channel, rx_scp, NULL)) {
		return 0;
	}

	if (mbox_set_enabled_dt(&channel, true)) {
		return 0;
	}
	psa_irq_enable(mailbox_irq.p_ildi->signal);
}
#endif
