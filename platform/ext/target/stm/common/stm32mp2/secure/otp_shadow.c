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

static struct stm32_otp_shadow_platdata pdata;

__PACKED_STRUCT otp_shadow_layout_t {
	uint32_t reserved1[5];
	uint32_t implementation_id[3];		/* otp 5..7 */
	uint32_t reserved2[10];
	uint32_t bootrom_config_9[1];		/* otp 18  */
	uint32_t reserved3[105];
	uint32_t hconf1[1];			/* otp 124 */
	uint32_t reserved4[207];
	uint32_t entropy_seed[16];		/* otp 332 */
	uint32_t iak[8];			/* otp 348 */
};

#define OTP_OFFSET(x)		(offsetof(struct otp_shadow_layout_t, x))
#define OTP_SIZE(x)		(sizeof(((struct otp_shadow_layout_t *)0)->x))

#define BOOTROM_CFG_9_SEC_BOOT	GENMASK(3,0)
#define BOOTROM_CFG_9_PROV_DONE	GENMASK(7,4)
#define HCONF1_DISABLE_SCAN	BIT(20)

__attribute__((weak))
int stm32_otp_shadow_get_platdata(struct stm32_otp_shadow_platdata *pdata)
{
	return -ENODEV;
}

static inline int _otp_is_valid(uint32_t status){
	return !(status & (STATUS_SECURE | LOCK_ERROR));
}

static enum tfm_plat_err_t otp_shadow_read(uint32_t offset, size_t len,
					   size_t out_len, uint8_t *out)
{
	struct bsec_shadow *shadow = (struct bsec_shadow *)pdata.base;
	size_t copy_size = len < out_len ? len : out_len;
	uint32_t *p_out_w = (uint32_t *)out;
	uint32_t *p_value, *p_status;
	uint32_t idx;

	if (offset % (sizeof(uint32_t)))
		return TFM_PLAT_ERR_INVALID_INPUT;

	if (copy_size % (sizeof(uint32_t)))
		return TFM_PLAT_ERR_INVALID_INPUT;

	offset /= sizeof(uint32_t);
	p_value = &(shadow->value[offset]);
	p_status = &(shadow->status[offset]);

	for (idx = 0; idx < (copy_size / sizeof(uint32_t)); idx++) {
		if (!_otp_is_valid(p_status[idx]))
			return TFM_PLAT_ERR_NOT_PERMITTED;

		p_out_w[idx] = p_value[idx];
	}

	return TFM_PLAT_ERR_SUCCESS;
}

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

static enum tfm_plat_err_t otp_shadow_read_lcs(uint32_t out_len, uint8_t *out)
{
	uint32_t secure_boot, disable_scan, prov_done;
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t*) out;
	uint32_t btrom_cfg_9, hconf1;
	enum tfm_plat_err_t err;

	*lcs = PLAT_OTP_LCS_ASSEMBLY_AND_TEST;

	err = otp_shadow_read(OTP_OFFSET(bootrom_config_9),
			      OTP_SIZE(bootrom_config_9),
			      sizeof(btrom_cfg_9), (uint8_t*)&btrom_cfg_9);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return err;

	err = otp_shadow_read(OTP_OFFSET(hconf1), OTP_SIZE(hconf1),
			      sizeof(hconf1), (uint8_t*)&hconf1);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return err;

	/* true if any bit of field is set */
	secure_boot = !!(btrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT);
	prov_done = !!(btrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE);
	disable_scan = !!(hconf1 & HCONF1_DISABLE_SCAN);

	if (secure_boot && prov_done && disable_scan)
		*lcs = PLAT_OTP_LCS_SECURED;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_read(enum tfm_otp_element_id_t id,
                                      size_t out_len, uint8_t *out)
{
	switch (id) {
	case PLAT_OTP_ID_LCS:
		return otp_shadow_read_lcs(out_len, out);
	case PLAT_OTP_ID_IAK:
		return otp_shadow_read(OTP_OFFSET(iak), OTP_SIZE(iak),
				       out_len, out);
	case PLAT_OTP_ID_IAK_LEN:
		*out = OTP_SIZE(iak);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		return otp_shadow_read(OTP_OFFSET(implementation_id),
				       OTP_SIZE(implementation_id),
				       out_len, out);
	case PLAT_OTP_ID_ENTROPY_SEED:
		return otp_shadow_read(OTP_OFFSET(entropy_seed),
				       OTP_SIZE(entropy_seed), out_len, out);
	case PLAT_OTP_ID_IAK_TYPE:
		return otp_fake_read(FAKE_OFFSET(iak_type),
				     FAKE_SIZE(iak_type), out_len, out);
	case PLAT_OTP_ID_IAK_ID:
		return otp_fake_read(FAKE_OFFSET(iak_id),
				     FAKE_SIZE(iak_id), out_len, out);
	case PLAT_OTP_ID_BOOT_SEED:
		return otp_fake_read(FAKE_OFFSET(boot_seed),
				     FAKE_SIZE(boot_seed), out_len, out);
	case PLAT_OTP_ID_HUK:
		return otp_fake_read(FAKE_OFFSET(huk),
				     FAKE_SIZE(huk), out_len, out);
	case PLAT_OTP_ID_PROFILE_DEFINITION:
		return otp_fake_read(FAKE_OFFSET(profile_definition),
				     FAKE_SIZE(profile_definition),
				     out_len, out);
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_get_size(enum tfm_otp_element_id_t id,
                                          size_t *size)
{
	switch (id) {
	case PLAT_OTP_ID_LCS:
		*size = sizeof(uint32_t);
		break;
	case PLAT_OTP_ID_IAK_LEN:
		*size = sizeof(uint32_t);
		break;
	case PLAT_OTP_ID_IAK:
		*size = OTP_SIZE(iak);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		*size = OTP_SIZE(implementation_id);
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		*size = OTP_SIZE(entropy_seed);
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

	return TFM_PLAT_ERR_SUCCESS;
}

/*
 * the shadow otp area is read only (protected by TDCID).
 * So for dummy provisioning, we must:
 *  - copy the otp area in tfm data.
 *  - move shadow otp base on it.
 */
#ifdef TFM_DUMMY_PROVISIONING
__PACKED_STRUCT bsec_shadow bsec_shadow_dummy;

enum tfm_plat_err_t stm32_otp_dummy_prep(void)
{
	memcpy(&bsec_shadow_dummy, (struct bsec_shadow *)pdata.base,
	       sizeof(bsec_shadow_dummy));
	pdata.base = (uintptr_t)&bsec_shadow_dummy;

	return TFM_PLAT_ERR_SUCCESS;
}

static enum tfm_plat_err_t otp_shadow_write(uint32_t offset, size_t len,
					    size_t in_len, const uint8_t *in)
{
	struct bsec_shadow *shadow = (struct bsec_shadow *)pdata.base;
	uint32_t *p_in_w = (uint32_t *)in;
	uint32_t *p_value, *p_status;
	uint32_t idx;

	if (offset % (sizeof(uint32_t)))
		return TFM_PLAT_ERR_INVALID_INPUT;

	if (len != in_len)
		return TFM_PLAT_ERR_INVALID_INPUT;

	if (len % (sizeof(uint32_t)))
		return TFM_PLAT_ERR_INVALID_INPUT;

	offset /= sizeof(uint32_t);
	p_value = &(shadow->value[offset]);
	p_status = &(shadow->status[offset]);

	for (idx = 0; idx < (len / sizeof(uint32_t)); idx++) {
		if (!_otp_is_valid(p_status[idx]))
			return TFM_PLAT_ERR_NOT_PERMITTED;

		p_value[idx] = p_in_w[idx];
		p_status[idx] = LOCK_SHADOW_R;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

static enum tfm_plat_err_t otp_shadow_write_lcs(uint32_t in_len, const uint8_t *in)
{
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t*) in;
	uint32_t btrom_cfg_9, hconf1;
	enum tfm_plat_err_t err;

	err = otp_shadow_read(OTP_OFFSET(bootrom_config_9),
			      OTP_SIZE(bootrom_config_9),
			      sizeof(btrom_cfg_9), (uint8_t*)&btrom_cfg_9);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return err;

	err = otp_shadow_read(OTP_OFFSET(hconf1), OTP_SIZE(hconf1),
			      sizeof(hconf1), (uint8_t*)&hconf1);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return err;

	if (*lcs == PLAT_OTP_LCS_SECURED) {
		if (!(btrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT))
			btrom_cfg_9 |= BOOTROM_CFG_9_SEC_BOOT;

		if (!(btrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE))
			btrom_cfg_9 |= BOOTROM_CFG_9_PROV_DONE;

		if (!(hconf1 & HCONF1_DISABLE_SCAN))
			hconf1 |= HCONF1_DISABLE_SCAN;
	}
	else {
		btrom_cfg_9 &= ~BOOTROM_CFG_9_PROV_DONE;
	}

	err = otp_shadow_write(OTP_OFFSET(bootrom_config_9),
			       OTP_SIZE(bootrom_config_9),
			       sizeof(btrom_cfg_9), (uint8_t*)&btrom_cfg_9);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return err;

	err = otp_shadow_write(OTP_OFFSET(hconf1), OTP_SIZE(hconf1),
			       sizeof(hconf1), (uint8_t*)&hconf1);
	if (err != TFM_PLAT_ERR_SUCCESS)
		return err;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id,
                                       size_t in_len, const uint8_t *in)
{
	switch (id) {
	case PLAT_OTP_ID_LCS:
		return otp_shadow_write_lcs(in_len, in);
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		return otp_shadow_write(OTP_OFFSET(implementation_id),
					OTP_SIZE(implementation_id),
					in_len, in);
	case PLAT_OTP_ID_ENTROPY_SEED:
		return otp_shadow_write(OTP_OFFSET(entropy_seed),
					OTP_SIZE(entropy_seed), in_len, in);
	case PLAT_OTP_ID_IAK:
		return otp_shadow_write(OTP_OFFSET(iak), OTP_SIZE(iak),
					in_len, in);
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}
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
	struct bsec_shadow *b_shadow = (struct bsec_shadow *) pdata.base;

	if (b_shadow->magic != BSEC_MAGIC)
		return TFM_PLAT_ERR_SYSTEM_ERR;

	pdata.hw_key_valid = false;
	if (b_shadow->state & BSEC_HARDWARE_KEY)
		pdata.hw_key_valid = true;

	return TFM_PLAT_ERR_SUCCESS;
}

int stm32_otp_shadow_init(void)
{
	int err;

	err = stm32_otp_shadow_get_platdata(&pdata);
	if (err)
		return err;

	return 0;
}
