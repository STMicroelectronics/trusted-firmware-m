/*
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stm32_bsec3.h>
#include "crypto_hw.h"
#include "entropy.h"
#include "stm32mp2.h"
#include "tfm_plat_hw_keys.h"

#define ENTROPY_SEED_SIZE	128 /* MBEDTLS_ENTROPY_MAX_GATHER */

/*  interface for mbed-crypto */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
	(void)data;

	*olen = 0;

	if (IS_ENABLED(STM32_M33TDCID)) {
		if (entropy_get_entropy(NULL, output, len))
			return -1;
	} else if (IS_ENABLED(TFM_DUMMY_PROVISIONING)) {
		static const uint8_t dummy_entropy_seed[ENTROPY_SEED_SIZE] = {
			0xa3, 0x1b, 0x8b, 0x12, 0xa0, 0xa2, 0x43, 0x53,
			0xa5, 0x35, 0x25, 0x34, 0x90, 0xcd, 0x5f, 0xa2,
			0x08, 0x4a, 0xe9, 0x20, 0xde, 0xd9, 0x94, 0xc1,
			0x9a, 0x78, 0x0f, 0x6b, 0xfb, 0xf6, 0xf8, 0xd7,
			0x33, 0xc7, 0x1b, 0xa1, 0x48, 0x10, 0x0b, 0xdb,
			0x7c, 0x37, 0xff, 0x07, 0x32, 0x9c, 0x35, 0xaf,
			0x26, 0x87, 0x45, 0x66, 0x0c, 0xfa, 0xde, 0xec,
			0x0d, 0x32, 0xca, 0xc9, 0xf4, 0x49, 0xf8, 0xa4,
			0x6f, 0x5f, 0xf7, 0xc2, 0x54, 0xe2, 0xb1, 0x6b,
			0xa3, 0x86, 0x13, 0x53, 0x76, 0xa1, 0x1e, 0x9b,
			0x25, 0xe9, 0x69, 0x82, 0xc0, 0x0c, 0x04, 0x0a,
			0xb9, 0x52, 0xb5, 0x63, 0x6f, 0x09, 0x74, 0xcb,
			0xb2, 0xcc, 0xa8, 0x00, 0xc9, 0x95, 0x1c, 0x84,
			0x27, 0x46, 0x88, 0xc4, 0xc6, 0x9f, 0x11, 0x80,
			0x23, 0xa0, 0xad, 0x18, 0x4b, 0x7a, 0xc9, 0x96,
			0x30, 0x9f, 0x72, 0xb9, 0x99, 0xa3, 0xe7, 0x47,
		};

		if (len > ENTROPY_SEED_SIZE)
			return -1;

		memcpy(output, dummy_entropy_seed, len);
	} else {
		/* Not yet supported */
		return -1;
	}

	*olen = len;

	return 0;
}

/*
 * \brief Initialize the stm crypto accelerator
 */

int crypto_hw_accelerator_init(void)
{
	return 0;
}

/*
 * \brief Deallocate the stm crypto accelerator
 */
int crypto_hw_accelerator_finish(void)
{
	return 0;
}

/*
 * \brief Check that crypto key is ready
 */
bool crypto_hw_is_key_ready(enum tfm_plat_hw_key_t key)
{
	switch (key) {
	case TFM_PLAT_HUK:
		return stm32_bsec_is_huk_ready();
	case TFM_PLAT_BHK:
		return false;
	default:
		return false;
	}
}

/**
 * \brief Apply permissions on debug signals
 *
 * \param[in]   permissions_mask   permission vector for debug signals
 *                                 vector bits interpretation is specific
 *                                 to a target and depends on the architecture
 * \param[in]   len                length of permission vector
 *
 * \return 0 on success, non-zero otherwise
 */
int crypto_hw_apply_debug_permissions(uint8_t *permissions_mask, uint32_t len __unused)
{
	uint32_t perm_mask;

	/*
	 * permissions_mask is given by psa-adac library, with len == 16,
	 * but permission mask on this platform is defined on 32 bits
	 */
	memcpy(&perm_mask, permissions_mask, sizeof(uint32_t));

	return stm32_bsec_write_debug_conf(perm_mask);
}
