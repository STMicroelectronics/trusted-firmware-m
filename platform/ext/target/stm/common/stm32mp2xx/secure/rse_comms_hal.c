/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#define DT_DRV_COMPAT st_psa_mbox

#include <device.h>
#include <mbox.h>
#include "tfm_multi_core.h"
#include "rse_comms_hal.h"
#include "rse_comms.h"
#include "rse_comms_queue.h"
#include "tfm_hal_device_header.h"
#include "tfm_peripherals_def.h"
#include "tfm_spm_log.h"
#include "tfm_pools.h"
#include "rse_comms_protocol.h"
#include "rse_shmem.h"
#include <string.h>
#include "psa_manifest/pid.h"
#include "psa_manifest/sid.h"
#include "region_defs.h"
#include "tfm_ns_notif.h"
#include "ns_evt.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,"only one st,psa-mbox node may be present");
/* Secure RSS SHMEM */
#define S_RSS_SHMEM_ADDR		DT_REG_ADDR(DT_INST_PHANDLE(0, memory_region))
#define S_RSS_SHMEM_SIZE		DT_REG_SIZE(DT_INST_PHANDLE(0, memory_region))

static const struct mbox_dt_spec channel =  { MBOX_DT_SPEC_GET_IDX(DT_DRV_INST(0),0)};
/* Declared statically to avoid using huge amounts of stack space. Maybe revisit
 * if functions not being reentrant becomes a problem.
 */
static __ALIGNED(4) struct serialized_psa_msg_t msg;
static __ALIGNED(4) struct serialized_psa_reply_t reply;

TFM_POOL_DECLARE(req_pool, sizeof(struct client_request_t),
                 RSE_COMMS_MAX_CONCURRENT_REQ);

static void rx_psa(const struct device *dev,
		   mbox_channel_id_t channel_id, void *user_data,
		   struct mbox_msg *data)
{
	tfm_multi_core_hal_receive(CLIENT_ID_OWNER_MAGIC, CLIENT_ID_OWNER_MAGIC, 0);
#if IPCC_LEGACY
	psa_call(TFM_MBOX_SERVICE_HANDLE, TFM_MBOX_SERVICE_SID, NULL, 0, NULL, 0);
#else
	tfm_ns_notif_flih(TFM_SP_IPCC_RSE_NS_EVT);
#endif
}

static inline bool sendmsg(void *msg, size_t msg_len)
{
        bool notify = *( rse_shmem_size_t*)(S_RSS_SHMEM_ADDR) & RSE_SHMEM_FLAGS_SIZE_NO_NOTIFICATION
		? false : true;

	memcpy((uint8_t*)(S_RSS_SHMEM_ADDR + RSE_SHMEM_PAYLOAD_OFFSET), msg, msg_len);
	(*(rse_shmem_size_t*)(S_RSS_SHMEM_ADDR + RSE_SHMEM_SIZE_OFFSET))= msg_len;

	return notify;
}

static inline void recvmsg(void *msg, size_t * msg_len)
{
	rse_shmem_size_t len = (*(rse_shmem_size_t *)(S_RSS_SHMEM_ADDR + RSE_SHMEM_PAYLOAD_OFFSET))
		& ~RSE_SHMEM_FLAGS_SIZE_NO_NOTIFICATION;
	*msg_len = *msg_len >= len ? len : *msg_len;
	memcpy((uint8_t *)msg, (uint8_t*)(S_RSS_SHMEM_ADDR + RSE_SHMEM_PAYLOAD_OFFSET)
	       , *msg_len);
}

enum tfm_plat_err_t tfm_multi_core_hal_receive(void *mhu_receiver_dev,
                                               void *mhu_sender_dev,
                                               uint32_t source)
{
	/*    enum mhu_error_t mhu_err;*/
	enum tfm_plat_err_t err;
	size_t msg_len = sizeof(msg);
	size_t reply_size;
	struct client_request_t *req;

	memset(&msg, 0, sizeof(msg));
	memset(&reply, 0, sizeof(reply));

	msg_len = sizeof(msg);
	/* Receive complete message */
	recvmsg(&msg, &msg_len);

	req = tfm_pool_alloc(req_pool);
	if (!req) {
		/* No free capacity, drop message */
		err = TFM_PLAT_ERR_SYSTEM_ERR;
		goto out_return_err;
	}
	memset(req, 0, sizeof(struct client_request_t));

	/* Record the sender device to be used for the reply */
	req->mhu_sender_dev = mhu_sender_dev;

	err = rse_protocol_deserialize_msg(req, &msg, msg_len);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		/* Deserialisation failed, drop message */
		goto out_return_err;
	}

	if (queue_enqueue(req) != 0) {
		/* No queue capacity, drop message */
		err = TFM_PLAT_ERR_SYSTEM_ERR;
		goto out_return_err;
	}

	/* Message successfully received */
	return TFM_PLAT_ERR_SUCCESS;

out_return_err:
	/*  Attempt to respond with a failure message */
	if (rse_protocol_serialize_error(req, &msg.header,
					 PSA_ERROR_CONNECTION_BUSY,
					 &reply, &reply_size)
	    == TFM_PLAT_ERR_SUCCESS) {
		if (sendmsg((uint8_t *)&reply, reply_size))
			mbox_send_dt(&channel, NULL);
	}

	if (req) {
		tfm_pool_free(req_pool, req);
	}

	return err;
}

enum tfm_plat_err_t tfm_multi_core_hal_reply(struct client_request_t *req)
{
	enum tfm_plat_err_t err;
	size_t reply_size;

	if (!is_valid_chunk_data_in_pool(req_pool, (uint8_t *)req)) {
		err = TFM_PLAT_ERR_SYSTEM_ERR;
		goto out;
	}

	err = rse_protocol_serialize_reply(req, &reply, &reply_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		SPMLOG_DBGMSGVAL("[COMMS] Serialize reply failed: ", err);
		goto out_free_req;
	}
	if (sendmsg(&reply, reply_size))
		if (mbox_send_dt(&channel, NULL))
			err = TFM_PLAT_ERR_SYSTEM_ERR;

	SPMLOG_DBGMSG("[COMMS] Sent reply\r\n");

out_free_req:
	tfm_pool_free(req_pool, req);
out:
	return err;
}

enum tfm_plat_err_t tfm_multi_core_hal_init(void)
{
	int32_t spm_err;

	spm_err = tfm_pool_init(req_pool, POOL_BUFFER_SIZE(req_pool),
				sizeof(struct client_request_t),
				RSE_COMMS_MAX_CONCURRENT_REQ);
	if (spm_err) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	if (mbox_register_callback_dt(&channel, rx_psa, NULL)) {
		return PSA_ERROR_BAD_STATE;
	}

	if (mbox_set_enabled_dt(&channel, true)) {
		return PSA_ERROR_BAD_STATE;
	}
	tfm_multi_core_register_client_id_range(CLIENT_ID_OWNER_MAGIC,-0x0400ffff, -0x04000000);

	return TFM_PLAT_ERR_SUCCESS;
}
