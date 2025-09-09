/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <lib/utils_def.h>
#include <debug.h>
#include <errno.h>
#include <sk_cipher.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <stm32_keyshare.h>

#include <psa/crypto.h>
#include <psa/service.h>
#include <tfm_plat_otp.h>

#define SAES_NODE_NAME	saes

#define ENC_MAX_KEY_SIZE	32

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(SAES_NODE_NAME), okay), "saes node is mandatory");

enum tfm_platform_err_t stm32_keyshare_enable(const psa_invec *in_vec, const psa_outvec *out_vec)
{
	const struct device *dev = SK_CIPHER_GET_BY_NODE_LABEL(SAES_NODE_NAME);
	enum tfm_otp_element_id_t id;
	enum tfm_platform_err_t status = TFM_PLATFORM_ERR_SYSTEM_ERROR;
	struct sk_cipher_config_t config;
	uint8_t key[ENC_MAX_KEY_SIZE] = {0};
	size_t key_size;
	uint32_t key_id;
	int err;

	if (in_vec->len != sizeof(uint32_t))
		return TFM_PLATFORM_ERR_INVALID_PARAM;

	key_id = *(uint32_t *)in_vec->base;

	switch (key_id) {
	case 0: /* FIP EDMK*/
		id = PLAT_OTP_ID_FIP_EDMK;
		break;
	default:
		return TFM_PLATFORM_ERR_INVALID_PARAM;
	}

	err = tfm_plat_otp_get_size(id, &key_size);
	if (err || (key_size > sizeof(key)))
		return TFM_PLATFORM_ERR_SYSTEM_ERROR;

	err = tfm_plat_otp_read(id, key_size, key);
	if (err) {
		status = TFM_PLATFORM_ERR_SYSTEM_ERROR;
		goto end;
	}

	/* wrap key */
	config = (struct sk_cipher_config_t){
		.is_dec = false,
		.ch_mode = SK_CIPHER_MODE_ECB,
		.key_select = STM32_SAES_KEY_DHU,
		.key = NULL,
		.key_size = key_size,
		.iv = NULL,
		.iv_size = 0,
	};

	err = sk_cipher_ctx_init(dev, &config);
	if (err) {
		status = TFM_PLATFORM_ERR_SYSTEM_ERROR;
		goto end;
	}

	err = sk_cipher_wrap(dev, true, 1, key, key);
	if (err) {
		status = TFM_PLATFORM_ERR_SYSTEM_ERROR;
		goto end;
	}

	err = sk_cipher_ctx_init(dev, &config);
	if (err) {
		status = TFM_PLATFORM_ERR_SYSTEM_ERROR;
		goto end;
	}

	/* unwrap key on the fly to start key sharing */
	err = sk_cipher_wrap(dev, false, 1, key, NULL);
	if (err) {
		status = TFM_PLATFORM_ERR_SYSTEM_ERROR;
		goto end;
	}

	status = TFM_PLATFORM_ERR_SUCCESS;

end:
	memset(key, 0, sizeof(key));

	return status;
}

enum tfm_platform_err_t stm32_keyshare_disable(const psa_invec *in_vec, const psa_outvec *out_vec)
{
	const struct device *dev = SK_CIPHER_GET_BY_NODE_LABEL(SAES_NODE_NAME);

	/* reset SAES disable key sharing. */
	if (sk_cipher_reset(dev))
		return PSA_ERROR_HARDWARE_FAILURE;

	return TFM_PLATFORM_ERR_SUCCESS;
}
