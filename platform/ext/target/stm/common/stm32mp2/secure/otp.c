/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmsis_compiler.h>

#include <tfm_plat_otp.h>
#include <psa/crypto.h>
#include <config_attest.h>
#include <tfm_plat_provisioning.h>

#include <stm32_bsec3.h>

#ifdef STM32_PROV_FAKE
/* waiting study on:
 *  - huk => wait SAES driver to derive HUK
 *  - where to get profile definition
 *  - where to get iak_id for symetric key (needed for small profile)
 *  - where is boot_seed (in production mode)
 */
__PACKED_STRUCT otp_fake_layout_t {
    uint8_t huk[32];
    uint8_t profile_definition[32];
    uint8_t iak_id[32];
    uint32_t iak_type;
    uint8_t boot_seed[32];
};

#define FAKE_OFFSET(x)       (offsetof(struct otp_fake_layout_t, x))
#define FAKE_SIZE(x)         (sizeof(((struct otp_fake_layout_t *)0)->x))

static const struct otp_fake_layout_t otp_fake_data = {
	.huk = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	},
	.profile_definition = {
#if ATTEST_TOKEN_PROFILE_PSA_IOT_1
		"PSA_IOT_PROFILE_1",
#elif ATTEST_TOKEN_PROFILE_PSA_2_0_0
		"http://arm.com/psa/2.0.0",
#elif ATTEST_TOKEN_PROFILE_ARM_CCA
		"http://arm.com/CCA-SSD/1.0.0",
#else
		"UNDEFINED",
#endif
	},
	.iak_id = {
		"kid@trustedfirmware.example",
	},
#ifdef SYMMETRIC_INITIAL_ATTESTATION
	.iak_type = PSA_ALG_HMAC(PSA_ALG_SHA_256),
#else
	.iak_type = PSA_ECC_FAMILY_SECP_R1,
#endif
	.boot_seed = {
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
		0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
		0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
		0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	},
};

static enum tfm_plat_err_t __maybe_unused
otp_fake_read(uint32_t offset, uint32_t len, uint32_t out_len, uint8_t *out)
{
	size_t copy_size = len < out_len ? len : out_len;

	memcpy(out, ((void*)&otp_fake_data) + offset, copy_size);
	return TFM_PLAT_ERR_SUCCESS;
}

static enum tfm_plat_err_t __maybe_unused
otp_fake_write(uint32_t offset, uint32_t len, uint32_t in_len, uint8_t *in)
{
	if (in_len > len)
		return TFM_PLAT_ERR_INVALID_INPUT;

	memcpy(((void*)&otp_fake_data) + offset, in, len);
	return TFM_PLAT_ERR_SUCCESS;
}
#else
static enum tfm_plat_err_t __maybe_unused
otp_fake_read(uint32_t offset, uint32_t len, uint32_t out_len, uint8_t *out)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}

static enum tfm_plat_err_t __maybe_unused
otp_fake_write(uint32_t offset, uint32_t len, uint32_t in_len, uint8_t *in)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}
#endif

enum tfm_plat_err_t tfm_plat_otp_read(enum tfm_otp_element_id_t id,
                                      size_t out_len, uint8_t *out)
{
	int err;

	switch (id) {
	case PLAT_OTP_ID_LCS:
	case PLAT_OTP_ID_IAK:
	case PLAT_OTP_ID_IAK_LEN:
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
	case PLAT_OTP_ID_ENTROPY_SEED:
		err = stm32_bsec_otp_read(id, out_len, out);
		break;
	case PLAT_OTP_ID_IAK_TYPE:
		err = otp_fake_read(FAKE_OFFSET(iak_type),
				    FAKE_SIZE(iak_type), out_len, out);
		break;
	case PLAT_OTP_ID_IAK_ID:
		err = otp_fake_read(FAKE_OFFSET(iak_id),
				    FAKE_SIZE(iak_id), out_len, out);
		break;
	case PLAT_OTP_ID_BOOT_SEED:
		err = otp_fake_read(FAKE_OFFSET(boot_seed),
				    FAKE_SIZE(boot_seed), out_len, out);
		break;
	case PLAT_OTP_ID_HUK:
		err = otp_fake_read(FAKE_OFFSET(huk),
				    FAKE_SIZE(huk), out_len, out);
		break;
	case PLAT_OTP_ID_PROFILE_DEFINITION:
		err = otp_fake_read(FAKE_OFFSET(profile_definition),
				    FAKE_SIZE(profile_definition),
				    out_len, out);
		break;
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (err)
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_get_size(enum tfm_otp_element_id_t id,
                                          size_t *size)
{
	int err = 0;

	switch (id) {
	case PLAT_OTP_ID_LCS:
	case PLAT_OTP_ID_IAK:
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
	case PLAT_OTP_ID_ENTROPY_SEED:
		err = stm32_bsec_otp_size(id, size);
		break;
	case PLAT_OTP_ID_IAK_LEN:
		*size = sizeof(uint32_t);
		break;
	case PLAT_OTP_ID_IAK_TYPE:
		*size = FAKE_SIZE(iak_type);
		break;
	case PLAT_OTP_ID_IAK_ID:
		*size = FAKE_SIZE(iak_id);
		break;
	case PLAT_OTP_ID_BOOT_SEED:
		*size = FAKE_SIZE(boot_seed);
		break;
	case PLAT_OTP_ID_HUK:
		*size = FAKE_SIZE(huk);
		break;
	case PLAT_OTP_ID_PROFILE_DEFINITION:
		*size = FAKE_SIZE(profile_definition);
		break;
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (err)
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}

/*
 * the shadow otp area is read only (protected by TDCID).
 * So for dummy provisioning, we must:
 *  - copy the otp area in tfm data.
 *  - move shadow otp base on it.
 */
#ifdef TFM_DUMMY_PROVISIONING
enum tfm_plat_err_t stm32_otp_dummy_prep(void)
{
	if (stm32_bsec_dummy_switch())
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id,
                                       size_t in_len, const uint8_t *in)
{
	int err = TFM_PLAT_ERR_UNSUPPORTED;

	switch (id) {
	case PLAT_OTP_ID_LCS:
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
	case PLAT_OTP_ID_ENTROPY_SEED:
	case PLAT_OTP_ID_IAK:
		err = stm32_bsec_otp_write(id, in_len, in);
		break;
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	if (err)
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}
#else
enum tfm_plat_err_t stm32_otp_dummy_prep(void)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}

enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id,
                                       size_t in_len, const uint8_t *in)
{
	return TFM_PLAT_ERR_UNSUPPORTED;
}
#endif

enum tfm_plat_err_t tfm_plat_otp_init(void)
{
	if (!stm32_bsec_is_valid())
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}
