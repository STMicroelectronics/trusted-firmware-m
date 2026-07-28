/*
 * Copyright (c) 2021-2024, Arm Limited. All rights reserved.
 * Copyright (c) 2021-2023 Cypress Semiconductor Corporation (an Infineon company)
 * or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "async.h"
#include "config_tfm.h"
#include "psa/service.h"
#include "psa_manifest/ns_agent_mailbox.h"
#include "tfm_hal_mailbox.h"
#include "tfm_hal_multi_core.h"
#include "tfm_hal_platform.h"
#include "tfm_multi_core.h"
#include "tfm_rpc.h"
#include "tfm_sp_log.h"

#include "compiler_ext_defs.h" /* Keep last. */

static void boot_ns_core(void)
{
    /* Boot up non-secure core */
    LOG_DBGFMT("Enabling non-secure core...\r\n");

    tfm_hal_boot_ns_cpu(tfm_hal_get_ns_VTOR());
    tfm_hal_wait_for_ns_cpu_ready();
}

/*
 * Boot the non-secure core and set up the inter-core communication.
 * Shared by the IPC entry loop and the SFN partition init below.
 */
static void ns_agent_mailbox_common_init(void)
{
    boot_ns_core();

    if (tfm_inter_core_comm_init()) {
        LOG_ERRFMT("Inter-core communication init failed\r\n");
        psa_panic();
    }
}

/*
 * Optional deferred-receive hook. Default is empty: platforms that receive and
 * enqueue the inter-core request directly in their interrupt handler keep
 * their existing behavior. A platform can override this (strong definition) to
 * move the reception/deserialization out of the interrupt context and perform
 * it here, from Thread mode, just before the request is dispatched.
 */
__attribute__((weak)) void tfm_mailbox_hal_pending_receive(void)
{
}

#if CONFIG_TFM_SPM_BACKEND_IPC == 1

void ns_agent_mailbox_entry(void)
{
    psa_signal_t signals = 0;

    ns_agent_mailbox_common_init();

    MAILBOX_ENABLE_INTERRUPTS();

    while (1) {
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
        if (MAILBOX_SIGNAL_IS_ACTIVE(signals)) {
            MAILBOX_CLEAR_SIGNAL(MAILBOX_SIGNAL_GET_ACTIVE(signals));
            tfm_mailbox_hal_pending_receive();
            tfm_rpc_client_call_handler();
        } else if (signals & ASYNC_MSG_REPLY) {
            tfm_rpc_client_call_reply();
        } else {
            psa_panic();
        }
    }
}

#else /* CONFIG_TFM_SPM_BACKEND_IPC == 1 */

/*
 * SFN backend variant.
 *
 * In the SFN model there is no dedicated agent thread that can block on
 * psa_wait(): the single running context is the NS agent launched by the SPM
 * (e.g. the TrustZone agent). The mailbox agent therefore cannot rely on its
 * own entry loop. Instead:
 *   - the non-secure/remote request is received and queued by the platform
 *     mailbox interrupt (SLIH/FLIH), which does NOT dispatch anything itself;
 *   - the non-secure caller then invokes the mailbox agent secure service,
 *     whose SFN handler runs in Thread mode and pumps the request queue
 *     synchronously (dispatch to the target service + reply).
 *
 * This mirrors the IPC path but replaces the blocking loop with a
 * service-driven, fully synchronous handling.
 */

/* SFN partition init: boot the NS core and set up inter-core communication. */
psa_status_t ns_agent_mailbox_init(void *param)
{
    (void)param;

    ns_agent_mailbox_common_init();

    return PSA_SUCCESS;
}

/*
 * Generic mailbox request pump for the SFN backend.
 *
 * A platform's mailbox agent service SFN handler must call this function to
 * process the queued mailbox request(s) from Thread mode. The actual dispatch
 * and reply are handled synchronously by the registered RPC handler
 * (rse_comms or tfm_spe_mailbox), which already replies directly in SFN mode.
 */
psa_status_t ns_agent_mailbox_process_request(void)
{
    tfm_mailbox_hal_pending_receive();
    tfm_rpc_client_call_handler();

    return PSA_SUCCESS;
}

#endif /* CONFIG_TFM_SPM_BACKEND_IPC == 1 */
