/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */

#include <assert.h>
#include <device.h>
#include <mbox.h>
#include <psa/service.h>
#include "psa_manifest/pid.h"
#include "psa_manifest/sid.h"
#include <spm.h>
#include "tfm_hal_interrupt.h"
#include "load/interrupt_defs.h"
#include "scmi_server.h"
#include "tfm_ns_notif.h"
#include "ns_evt.h"
#include <dt-bindings/scmi/stm32mp2-agents.h>

/* function called by mailbox SLIH handler */
void rx_scp(const struct device *dev,
	    mbox_channel_id_t channel_id, void *user_data,
	    struct mbox_msg *data)
{
#if IPCC_LEGACY
	psa_call(TFM_SCP_SERVICE_HANDLE, (int32_t)user_data, NULL, 0, NULL, 0);
#else
	(void)tfm_ns_notif_flih((int)user_data);
#endif
}


int scp_mbox_raise(void *chan_mbx)
{
	const struct mbox_dt_spec *chan = (const struct mbox_dt_spec *)chan_mbx;
	return mbox_send_dt(chan, NULL);
}

void scp_com_handle(int type)
{
	scmi_server_smt_process_thread(type - 1);
}

int scp_com_init(const struct mbox_dt_spec *chan, const int agent_id)
{
        /*  Initialize scmi channel for ca35 ns */
	void *user_data;

#if IPCC_LEGACY
	user_data = (void *)agent_id;
#else
	switch (agent_id) {
	case STM32MP25_AGENT_ID_CA35:
		user_data =(void *)TFM_SP_IPCC_SCMI_CA35_NS_EVT;
		break;
	case STM32MP25_AGENT_ID_CA35_BL31:
		user_data=(void *)TFM_SP_IPCC_SCMI_CA35_BL31_NS_EVT;
		break;
	default:
		return -1;
	}
#endif

	if (mbox_register_callback_dt(chan, rx_scp, user_data)) {
		return -1;
	}

	if (mbox_set_enabled_dt(chan, true)) {
		return -2;
	}

	return 0;
}

