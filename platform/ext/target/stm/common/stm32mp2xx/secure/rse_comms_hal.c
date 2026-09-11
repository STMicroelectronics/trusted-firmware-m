/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#define DT_DRV_COMPAT st_psa_mbox

#include <device.h>
#include <mbox.h>
#include <psa/service.h>
#include "config_impl.h"
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
#include <service_api.h>
#include "ns_evt.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,"only one st,psa-mbox node may be present");

#if CONFIG_TFM_SPM_BACKEND_IPC != 1
/*
 * SFN backend: mailbox agent service handler.
 *
 * In the SFN model the mailbox agent has no blocking entry loop. The queued
 * request is pumped synchronously from Thread mode when the (local) non-secure
 * caller invokes the TFM_MBOX_SERVICE service. The generic pump does the
 * dispatch and reply.
 */
psa_status_t tfm_mbox_service_sfn(const psa_msg_t *msg)
{
	(void)msg;

	return ns_agent_mailbox_process_request();
}
#endif /* CONFIG_TFM_SPM_BACKEND_IPC != 1 */

/* Secure RSS SHMEM */
#define S_RSS_SHMEM_ADDR		DT_INST_MEMORY_REGION_ADDR(0)
#define S_RSS_SHMEM_SIZE		DT_INST_MEMORY_REGION_SIZE(0)

static const struct mbox_dt_spec channel =  { MBOX_DT_SPEC_GET_IDX(DT_DRV_INST(0),0)};
/* Declared statically to avoid using huge amounts of stack space. Maybe revisit
 * if functions not being reentrant becomes a problem.
 */
static __ALIGNED(4) struct serialized_psa_msg_t msg;
static __ALIGNED(4) struct serialized_psa_reply_t reply;

#if CONFIG_TFM_SPM_BACKEND_IPC == 1
TFM_POOL_DECLARE(req_pool, sizeof(struct client_request_t),
                 RSE_COMMS_MAX_CONCURRENT_REQ);
#else
/*
 * SFN backend: the mailbox interrupt only records that a message is pending;
 * the actual reception/deserialization and dispatch happen synchronously from
 * Thread mode in the mailbox agent service. As the protocol is
 * single-outstanding, a plain flag is enough and no request pool is needed.
 */
static volatile bool mailbox_msg_pending;
#endif

static void rx_psa(const struct device *dev,
		   mbox_channel_id_t channel_id, void *user_data,
		   struct mbox_msg *data)
{
#if CONFIG_TFM_SPM_BACKEND_IPC != 1
	mailbox_msg_pending = true;
#endif
#if IPCC_LEGACY
	psa_call(TFM_MBOX_SERVICE_HANDLE, TFM_MBOX_SERVICE_SID, NULL, 0, NULL, 0);
#else
	tfm_ns_notif_flih(TFM_SP_IPCC_RSE_NS_EVT);
#endif
}

void tfm_mailbox_hal_pending_receive(void)
{
	tfm_multi_core_hal_receive(CLIENT_ID_OWNER_MAGIC, CLIENT_ID_OWNER_MAGIC, 0);
}

static inline bool rse_shmem_send(void *m, size_t m_len)
{
        bool notify = *(rse_shmem_size_t *)(S_RSS_SHMEM_ADDR) & RSE_SHMEM_FLAGS_SIZE_NO_NOTIFICATION
		? false : true;

	memcpy((uint8_t *)(S_RSS_SHMEM_ADDR + RSE_SHMEM_PAYLOAD_OFFSET), m, m_len);
	(*(rse_shmem_size_t *)(S_RSS_SHMEM_ADDR + RSE_SHMEM_SIZE_OFFSET)) = m_len;

	return notify;
}

static inline void rse_shmem_recv(void *m, size_t *m_len)
{
	rse_shmem_size_t len = (*(rse_shmem_size_t *)(S_RSS_SHMEM_ADDR + RSE_SHMEM_PAYLOAD_OFFSET))
		& ~RSE_SHMEM_FLAGS_SIZE_NO_NOTIFICATION;

	*m_len = *m_len >= len ? len : *m_len;
	memcpy((uint8_t *)m, (uint8_t *)(S_RSS_SHMEM_ADDR + RSE_SHMEM_PAYLOAD_OFFSET), *m_len);
}

enum tfm_plat_err_t tfm_multi_core_hal_receive(void *mhu_receiver_dev,
                                               void *mhu_sender_dev,
                                               uint32_t source)
{
#if CONFIG_TFM_SPM_BACKEND_IPC == 1
	/*    enum mhu_error_t mhu_err;*/
	enum tfm_plat_err_t err;
	size_t msg_len = sizeof(msg);
	size_t reply_size;
	struct client_request_t *req;

	memset(&msg, 0, sizeof(msg));
	memset(&reply, 0, sizeof(reply));

	msg_len = sizeof(msg);
	/* Receive complete message */
	rse_shmem_recv(&msg, &msg_len);

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
		if (rse_shmem_send((uint8_t *)&reply, reply_size))
			mbox_send_dt(&channel, NULL);
	}

	if (req) {
		tfm_pool_free(req_pool, req);
	}

	return err;
#else /* CONFIG_TFM_SPM_BACKEND_IPC == 1 */
	(void)mhu_receiver_dev;
	(void)mhu_sender_dev;
	(void)source;
	return TFM_PLAT_ERR_SYSTEM_ERR;
#endif /* CONFIG_TFM_SPM_BACKEND_IPC == 1 */
}

#if CONFIG_TFM_SPM_BACKEND_IPC != 1
enum tfm_plat_err_t tfm_multi_core_hal_receive_sfn(struct client_request_t *req)
{
	enum tfm_plat_err_t err;
	size_t msg_len = sizeof(msg);
	size_t reply_size;

	/*
	 * Drain-loop terminator: process at most the single outstanding request
	 * flagged by the interrupt. Once consumed, report "empty" so the caller's
	 * loop stops instead of re-dispatching stale shared-memory content.
	 */
	if (!mailbox_msg_pending) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
	mailbox_msg_pending = false;

	memset(&msg, 0, sizeof(msg));
	memset(&reply, 0, sizeof(reply));
	memset(req, 0, sizeof(*req));

	/* Receive complete message from shared memory */
	rse_shmem_recv(&msg, &msg_len);

	/* Record the sender device to be used for the reply */
	req->mhu_sender_dev = (void *)CLIENT_ID_OWNER_MAGIC;

	err = rse_protocol_deserialize_msg(req, &msg, msg_len);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		/* Deserialisation failed: reply with an error and stop the loop */
		if (rse_protocol_serialize_error(req, &msg.header,
						 PSA_ERROR_CONNECTION_BUSY,
						 &reply, &reply_size)
		    == TFM_PLAT_ERR_SUCCESS) {
			if (rse_shmem_send((uint8_t *)&reply, reply_size))
				mbox_send_dt(&channel, NULL);
		}
		return err;
	}

	/* Message ready: caller dispatches and replies via the single req */
	return TFM_PLAT_ERR_SUCCESS;
}
#endif /* CONFIG_TFM_SPM_BACKEND_IPC != 1 */

enum tfm_plat_err_t tfm_multi_core_hal_reply(struct client_request_t *req)
{
	enum tfm_plat_err_t err;
	size_t reply_size;

#if CONFIG_TFM_SPM_BACKEND_IPC == 1
	if (!is_valid_chunk_data_in_pool(req_pool, (uint8_t *)req)) {
		err = TFM_PLAT_ERR_SYSTEM_ERR;
		goto out;
	}
#endif

	err = rse_protocol_serialize_reply(req, &reply, &reply_size);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		SPMLOG_DBGMSGVAL("[COMMS] Serialize reply failed: ", err);
		goto out_free_req;
	}
	if (rse_shmem_send(&reply, reply_size))
		if (mbox_send_dt(&channel, NULL))
			err = TFM_PLAT_ERR_SYSTEM_ERR;

	SPMLOG_DBGMSG("[COMMS] Sent reply\r\n");

out_free_req:
#if CONFIG_TFM_SPM_BACKEND_IPC == 1
	tfm_pool_free(req_pool, req);
out:
#endif
	return err;
}

enum tfm_plat_err_t tfm_multi_core_hal_init(void)
{
#if CONFIG_TFM_SPM_BACKEND_IPC == 1
	int32_t spm_err;

	spm_err = tfm_pool_init(req_pool, POOL_BUFFER_SIZE(req_pool),
				sizeof(struct client_request_t),
				RSE_COMMS_MAX_CONCURRENT_REQ);
	if (spm_err) {
		return TFM_PLAT_ERR_SYSTEM_ERR;
	}
#endif
	if (mbox_register_callback_dt(&channel, rx_psa, NULL)) {
		return PSA_ERROR_BAD_STATE;
	}

	if (mbox_set_enabled_dt(&channel, true)) {
		return PSA_ERROR_BAD_STATE;
	}
	tfm_multi_core_register_client_id_range(CLIENT_ID_OWNER_MAGIC,-0x0400ffff, -0x04000000);

	return TFM_PLAT_ERR_SUCCESS;
}
