/*
 * Copyright (C) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <stdbool.h>
#include "psa/client.h"
#include "psa_manifest/sid.h"
#include "tfm_scmi_api.h"

/*
 * Fifo buffer number is pow of 2 so that BUF_NUM-1 mask is
 * used for incrementing fifo read and write index
 */
#define BUF_NUM 2

struct notif_fifo_t {
	uint32_t buf[BUF_NUM][32];
	/*
	 * read == write means fifo empty, write = read-1 means fifo is full.
	 * if a buffer at index read-1 is written, a fifo overflow is detected,
	 * and write index is not incremented.
	 */
	uint32_t read;
	uint32_t write;
	bool overflow;
};

static struct notif_fifo_t notif = {0};

static inline void handle_fifo(uint32_t write)
{
	if (notif.buf[write][0]) {
		write = (write + 1) & (BUF_NUM - 1);
		if (write == notif.read) {
			notif.overflow = true;
		} else
			notif.write = write;
	}
}

psa_status_t tfm_scmi_req(void *req, size_t req_len, void *rsp, size_t rsp_len)
{
	psa_status_t status;
	psa_outvec out_vec[2];
	psa_invec in_vec;
	uint32_t write = notif.write;

	in_vec.base = (const void *)req;
	in_vec.len = req_len;

	out_vec[0].base = (void *)rsp;
	out_vec[0].len = rsp_len;
	out_vec[1].base = (void *)notif.buf[write];
	out_vec[1].len = sizeof(notif.buf[write]);


	status = psa_call(TFM_SCP_SERVICE_NS_HANDLE, PSA_IPC_CALL, &in_vec, 1,
			  out_vec, 2);

	handle_fifo(write);

	return status;
}

psa_status_t tfm_secure_scmi_req(uint32_t agent_id)
{

	psa_status_t status;
	psa_outvec out_vec[1];
	uint32_t write = notif.write;

	out_vec[0].base = (void *)notif.buf[write];
	out_vec[0].len = sizeof(notif.buf[write]);
	status = psa_call(TFM_SCP_SERVICE_HANDLE, agent_id, NULL, 0,
			  out_vec, 1);

	handle_fifo(write);

	return status;
}

/* Retrieve scmi notification from scp firmware */
psa_status_t tfm_secure_scmi_get_notif(void *rsp, size_t rsp_len)
{
	uint32_t read = notif.read;

	if (read == notif.write)
		return  PSA_SCMI_ERR_BUFFER_EMPTY;

	memcpy(rsp, notif.buf[read], rsp_len);

	/* Clear buffer header */
	notif.buf[read][0] = 0;
	/* Increment read index */
	notif.read = (read + 1) & (BUF_NUM - 1);

	if (notif.overflow) {
		notif.overflow = false;
		return PSA_SCMI_ERR_BUFFER_OVERFLOW;
	}

	return PSA_SCMI_ERR_SUCCESS;
}
