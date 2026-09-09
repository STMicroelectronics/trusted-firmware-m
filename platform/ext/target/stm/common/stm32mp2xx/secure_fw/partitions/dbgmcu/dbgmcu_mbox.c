/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025, STMicroelectronics
 *
 */

#include <debug.h>
#include <device.h>
#include <psa/error.h>
#include <psa/service.h>
#include <load/interrupt_defs.h>
#include "psa_manifest/tfm_dbgmcu.h"

extern void DBG_AUTH_HOST_HANDLE(void);
extern struct irq_t dbg_auth_host_irq;

/*
 * Run the debug authentication sequence for a pending DBG_AUTH_HOST interrupt.
 *
 * This MUST be called from Thread mode: DBG_AUTH_HOST_HANDLE() polls the
 * DBGMCU mailbox (up to 1s) and ends up issuing a psa_call() towards the ADAC
 * service, which is rejected from Handler mode.
 *
 * SLIH model: the ISR has masked the interrupt source and asserted the signal,
 * psa_eoi() acknowledges it and re-arms the source.
 */
static void dbgmcu_process_pending(void)
{
	DBG_AUTH_HOST_HANDLE();
	psa_eoi(dbg_auth_host_irq.p_ildi->signal);
}

#if TFM_SP_DBGMCU_MODEL_IPC == 1

void tfm_dbgmcu_entry(void)
{
	psa_signal_t irq_signal = dbg_auth_host_irq.p_ildi->signal;
	psa_signal_t signals;
	psa_msg_t msg;

	psa_irq_enable(irq_signal);

	while (1) {
		signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);

		if (signals & irq_signal)
			dbgmcu_process_pending();

		if (signals & TFM_DBGMCU_SERVICE_SIGNAL) {
			/*
			 * This partition owns a thread which already services
			 * the interrupt, so there is nothing to pump here.
			 * Only acknowledge the caller so that the service
			 * behaves identically on both backends.
			 */
			psa_get(TFM_DBGMCU_SERVICE_SIGNAL, &msg);
			psa_reply(msg.handle, PSA_SUCCESS);
		}
	}
}

#elif TFM_SP_DBGMCU_MODEL_SFN == 1

psa_status_t tfm_dbgmcu_init(void)
{
	/*
	 * The SFN backend runs every partition on a single shared context:
	 * entry_init MUST return, otherwise spm_init_function() never
	 * completes and the boot sequence hangs before the NS agent starts.
	 *
	 * Only arm the interrupt here. The deferred processing is performed
	 * in Thread mode by tfm_dbgmcu_service_sfn().
	 */
	psa_irq_enable(dbg_auth_host_irq.p_ildi->signal);

	return PSA_SUCCESS;
}

psa_status_t tfm_dbgmcu_service_sfn(const psa_msg_t *msg)
{
	psa_signal_t irq_signal = dbg_auth_host_irq.p_ildi->signal;

	(void)msg;

	/*
	 * Under SFN there is no partition thread to block on psa_wait(), so a
	 * client drives the deferred handling by calling this service. Poll
	 * the interrupt signal and, when a debug authentication request is
	 * pending, service it from this Thread mode context.
	 */
	if (psa_wait(irq_signal, PSA_POLL) & irq_signal)
		dbgmcu_process_pending();

	return PSA_SUCCESS;
}

#endif /* TFM_SP_DBGMCU_MODEL_IPC == 1 */
