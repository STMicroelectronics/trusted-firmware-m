/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_BSEC3_H
#define STM32_BSEC3_H

#include <stddef.h>
#include <stdbool.h>
#include <lib/utils_def.h>

#define DBG_FULL 0xFFF

struct stm32_bsec_platdata {
	uintptr_t base;
};

void stm32_bsec_write_debug_conf(uint32_t val);
int stm32_bsec_init(void);

/*
 * otp shadow depend of TDCID loader
 * which copies bsec otp to shadow memory.
 * must be aligned with [TDCID loader]stm32_bsec3 driver
 */
#define STM32MP2_OTP_MAX_ID		367
#define OTP_MAX_SIZE			(STM32MP2_OTP_MAX_ID + 1U)

/* Magic use to indicated valid SHADOW = 'B' 'S' 'E' 'C' */
#define BSEC_MAGIC			0x42534543

/* state bitfield */
#define BSEC_STATE_SEC_OPEN		U(0x0)
#define BSEC_STATE_SEC_CLOSED		U(0x1)
#define BSEC_STATE_INVALID		U(0x3)
#define BSEC_STATE_MASK			GENMASK_32(1, 0)
#define BSEC_HARDWARE_KEY		BIT(8)

/* status bitfield */
#define LOCK_PERM			BIT(30)
#define LOCK_SHADOW_R			BIT(29)
#define LOCK_SHADOW_W			BIT(28)
#define LOCK_SHADOW_P			BIT(27)
#define LOCK_ERROR			BIT(26)
#define STATUS_PROVISIONING		BIT(1)
#define STATUS_SECURE			BIT(0)

struct bsec_shadow {
	uint32_t magic;
	uint32_t state;
	uint32_t value[OTP_MAX_SIZE];
	uint32_t status[OTP_MAX_SIZE];
};

struct stm32_otp_shadow_platdata {
	uintptr_t base;
	size_t size;
	bool hw_key_valid;
};

int stm32_otp_shadow_init(void);
enum tfm_plat_err_t stm32_otp_dummy_prep(void);

#endif /* STM32_BSEC3_H */
