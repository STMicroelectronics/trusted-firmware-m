/*
 * Copyright (c) 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_DBGMCU_API_H__
#define __TFM_DBGMCU_API_H__

#include <stdint.h>
#include <psa/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Process a pending debug authentication request.
 *
 * The DBGMCU partition cannot run the debug authentication sequence from its
 * DBG_AUTH_HOST interrupt handler: the sequence polls the DBGMCU mailbox and
 * issues a psa_call() to the ADAC service, both of which require Thread mode.
 *
 * With the IPC backend the partition owns a thread which services the
 * interrupt on its own, and this call is a no-op acknowledgment.
 *
 * With the SFN backend there is no partition thread, so this call is the
 * mechanism which lets the pending request be handled: it must be issued
 * periodically, or whenever a debugger connection is expected, for the secure
 * debug authentication to complete.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval #PSA_SUCCESS   The pending request, if any, has been processed
 */
psa_status_t tfm_dbgmcu_service(void);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_DBGMCU_API_H__ */
