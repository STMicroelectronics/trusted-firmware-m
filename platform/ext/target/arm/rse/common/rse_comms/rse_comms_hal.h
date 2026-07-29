/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __RSE_COMMS_HAL_H__
#define __RSE_COMMS_HAL_H__

#include "rse_comms.h"
#include "tfm_plat_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Platform specific initialization of SPE multi-core.
 *
 * \retval TFM_PLAT_ERR_SUCCESS  Operation succeeded.
 * \retval Other return code     Operation failed with an error code.
 */
enum tfm_plat_err_t tfm_multi_core_hal_init(void);

/**
 * \brief Receive PSA client call request from NSPE.
 *        Implemented by platform specific inter-processor communication driver.
 *
 * \param[in] mhu_receiver_dev  Pointer to MHU receiver device on which to read
 *                              the message.
 * \param[in] mhu_sender_dev    Pointer to MHU sender device on which to write
 *                              the reply.
 * \param[in] source            The number of the IRQ source for this MHU.
 *
 * \retval TFM_PLAT_ERR_SUCCESS  Operation succeeded.
 * \retval Other return code     Operation failed with an error code.
 */
enum tfm_plat_err_t tfm_multi_core_hal_receive(void *mhu_receiver_dev,
                                               void *mhu_sender_dev,
                                               uint32_t source);

/**
 * \brief SFN backend: receive and deserialize the next pending PSA client call
 *        request from NSPE directly into the caller-provided request buffer.
 *
 * This is the queue-free / allocation-free counterpart of
 * tfm_multi_core_hal_receive() used by the SFN model. The RSE comms protocol
 * is single-outstanding and handled synchronously, so no request pool or ring
 * queue is needed: the message is deserialized in place into \p req.
 *
 * \param[out] req  Request buffer to deserialize the incoming message into.
 *
 * \retval TFM_PLAT_ERR_SUCCESS  A message was pending and deserialized into
 *                               \p req; the caller should dispatch and reply.
 * \retval Other return code     No message was pending (drain loop should
 *                               stop), or the message was rejected and an
 *                               error reply was already sent.
 */
enum tfm_plat_err_t tfm_multi_core_hal_receive_sfn(struct client_request_t *req);

/**
 * \brief Notify NSPE that a PSA client call return result is replied.
 *        Implemented by platform specific inter-processor communication driver.
 *
 * \retval TFM_PLAT_ERR_SUCCESS  The notification is successfully sent out.
 * \retval Other return code     Operation failed with an error code.
 */
enum tfm_plat_err_t tfm_multi_core_hal_reply(struct client_request_t *req);

#ifdef __cplusplus
}
#endif

#endif /* __RSE_COMMS_HAL_H__ */
