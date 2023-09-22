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
#include <tfm_plat_otp.h>

#define DBG_FULL 0xFFF
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

void stm32_bsec_write_debug_conf(uint32_t val);

int stm32_bsec_otp_size(enum tfm_otp_element_id_t id, size_t *size);
int stm32_bsec_otp_read(enum tfm_otp_element_id_t id,
			size_t out_len, uint8_t *out);
int stm32_bsec_otp_write(enum tfm_otp_element_id_t id,
			 size_t in_len, const uint8_t *in);
int stm32_bsec_dummy_switch(void);
bool stm32_bsec_is_valid(void);
#endif /* STM32_BSEC3_H */
