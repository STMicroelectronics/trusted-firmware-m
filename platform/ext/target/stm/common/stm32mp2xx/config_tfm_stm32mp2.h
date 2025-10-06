/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef __CONFIG_TFM_STM32MP2_H__
#define __CONFIG_TFM_STM32MP2_H__

/* Include optional claims in initial attestation token */
#undef ATTEST_INCLUDE_OPTIONAL_CLAIMS
#define ATTEST_INCLUDE_OPTIONAL_CLAIMS	0

/* For size optimization, set CLK_MINIMAL_SZ (no clock name defined e.g.) */
#ifndef CLK_MINIMAL_SZ
#define CLK_MINIMAL_SZ	1
#endif

#define SCP_STACK_SIZE 0x2000
#define IPCC_STACK_SIZE 0x2000
#define STL_STACK_SIZE 0x2000

#define CONFIG_TFM_SCHEDULE_WHEN_NS_INTERRUPTED 1
#define CONFIG_TFM_SECURE_SLIH_MASK_NS_INTERRUPT 1

#define RSE_COMMS_PAYLOAD_MAX_SIZE (0x40 + 0x800)

#ifdef MAILBOX_ENABLE_INTERRUPTS
#undef MAILBOX_ENABLE_INTERRUPTS
#undef MAILBOX_SIGNAL_IS_ACTIVE
#undef MAILBOX_SIGNAL_GET_ACTIVE
#undef MAILBOX_CLEAR_SIGNAL
#endif /* MAILBOX_ENABLE_INTERRUPTS */
#define MAILBOX_ENABLE_INTERRUPTS()
#define MAILBOX_SIGNAL_IS_ACTIVE(signals) (signals & TFM_MBOX_SERVICE_SIGNAL) ? true : false
#define MAILBOX_SIGNAL_GET_ACTIVE(signals) (signals & TFM_MBOX_SERVICE_SIGNAL)
#define MAILBOX_CLEAR_SIGNAL(signals) { psa_msg_t msg; \
	     psa_get(MAILBOX_SIGNAL_GET_ACTIVE(signals), &msg); \
	     psa_reply(msg.handle, PSA_SUCCESS); }

/* Use stored NV seed to provide entropy */
#undef CRYPTO_NV_SEED
#define CRYPTO_NV_SEED 0
#define CRYPTO_EXT_RNG 1

/* TLV minor used for platform boot data with TLV_MAJOR_PLATFORM */
#define TLV_PLAT_DDRENCKEY 0

#endif /* __CONFIG_TFM_STM32MP2_H__ */
