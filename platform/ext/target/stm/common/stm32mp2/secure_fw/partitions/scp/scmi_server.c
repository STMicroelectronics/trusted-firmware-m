/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019-2022, Linaro Limited
 * Copyright (c) 2023-2024, STMicroelectronics
 *
 */

#include <arch_main.h>
#include <scmi_agent_configuration.h>
#include <scmi_server.h>
#include "tfm_scmi.h"
#include <assert.h>
#include "tfm_sp_log.h"
#include "psa/service.h"

/*
 * SCMI server APIs exported to TF-M core
 */
int scmi_server_get_channels_count(void)
{
	return scmi_get_devices_count();
}

int32_t scmi_server_get_channel(unsigned int channel_id, int *handle)
{
	int fwk_id = 0;

	fwk_id = scmi_get_device(channel_id);
	if (fwk_id < 0)
		return TFM_SCMI_INVAL_PARAM;

	if (handle)
		*handle = fwk_id;

	return TFM_SCMI_SUCCESS;
}

int32_t scmi_server_smt_process_thread(unsigned int channel_id)
{
	int32_t res = TFM_SCMI_INVAL_PARAM;
	int fwk_id = 0;

	res = scmi_server_get_channel(channel_id, &fwk_id);
	if (!res)
		scmi_process_mbx_smt(fwk_id);

	return res;
}

int32_t scmi_server_msg_process_thread(unsigned int channel_id,
					  void *in_buf, size_t in_sz,
					  void *out_buf, size_t *out_sz)
{
	int32_t res = TFM_SCMI_INVAL_PARAM;
	int fwk_id = 0;

	res = scmi_server_get_channel(channel_id, &fwk_id);
	if (!res)
		scmi_process_mbx_msg(fwk_id, in_buf, in_sz, out_buf, out_sz);

	return TFM_SCMI_SUCCESS;
}

int32_t scmi_server_initialize(void)
{
	struct scpfw_config *cfg = NULL;
	int rc = 0;

	cfg = scmi_scpfw_get_configuration();
	assert(cfg);
	scpfw_configure(cfg);
	scmi_scpfw_release_configuration();

	rc = scmi_arch_init();
	if (rc < 0) {
		LOG_ERRFMT("SCMI server init failed: %d", rc);
		psa_panic();
	}

	return TFM_SCMI_SUCCESS;
}
