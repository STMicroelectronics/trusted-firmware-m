/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef __CONFIG_TFM_TARGET_H__
#define __CONFIG_TFM_TARGET_H__

/* Include optional claims in initial attestation token */
#undef ATTEST_INCLUDE_OPTIONAL_CLAIMS
#define ATTEST_INCLUDE_OPTIONAL_CLAIMS	0

/* For size optimization, set CLK_MINIMAL_SZ (no clock name defined e.g.) */
#ifndef CLK_MINIMAL_SZ
#define CLK_MINIMAL_SZ	1

#endif
/* Set ddr4 type */
#undef STM32MP_DDR4_TYPE
#define STM32MP_DDR4_TYPE 1

#define SCP_STACK_SIZE 0x2000

#endif /* __CONFIG_TFM_TARGET_H__ */
