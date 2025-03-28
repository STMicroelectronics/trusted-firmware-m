/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */
#define DT_DRV_COMPAT st_scp_mbox

#include <assert.h>
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


/* function called by mailbox SLIH handler */
void rx_scp(const struct device *dev,
	    mbox_channel_id_t channel_id, void *user_data,
	    struct mbox_msg *data)
{
	psa_call(TFM_SCP_SERVICE_HANDLE, (int32_t)user_data, NULL, 0, NULL, 0);
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

int scp_com_init(const struct mbox_dt_spec *chan, void *user_data)
{
        /*  Initialize scmi channel for ca35 ns */
	if (mbox_register_callback_dt(chan, rx_scp, user_data)) {
		return -1;
	}

	if (mbox_set_enabled_dt(chan, true)) {
		return -2;
	}

	return 0;
}

