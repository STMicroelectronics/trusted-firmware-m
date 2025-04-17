/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define DT_DRV_COMPAT st_stm32mp2_otp

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmsis_compiler.h>

#include <lib/utils_def.h>
#include <tfm_plat_otp.h>
#include <psa/crypto.h>
#include <config_tfm.h>
#include <tfm_plat_provisioning.h>

#include <devicetree.h>
#include <devicetree/nvmem.h>
#include <nvmem.h>

#ifdef TFM_DUMMY_PROVISIONING
__PACKED_STRUCT tfm_psa_rot_provisioning_data_t {
	uint8_t iak[32];
	uint8_t implementation_id[12];
	uint8_t entropy_seed[64];
#if defined(STM32_BL2)
	uint8_t bl2_rotpk_0[32];
	uint8_t bl2_rotpk_1[32];
	uint8_t bl2_rotpk_2[32];
#endif
};

static const struct tfm_psa_rot_provisioning_data_t psa_rot_prov_data = {
	.iak = {
		0xA9, 0xB4, 0x54, 0xB2, 0x6D, 0x6F, 0x90, 0xA4,
		0xEA, 0x31, 0x19, 0x35, 0x64, 0xCB, 0xA9, 0x1F,
		0xEC, 0x6F, 0x9A, 0x00, 0x2A, 0x7D, 0xC0, 0x50,
		0x4B, 0x92, 0xA1, 0x93, 0x71, 0x34, 0x58, 0x5F,
	},
	.implementation_id = {
		0xAA, 0xAA, 0xAA, 0xAA,
		0xBB, 0xBB, 0xBB, 0xBB,
		0xCC, 0xCC, 0xCC, 0xCC,
	},
	.entropy_seed = {
		0x12, 0x13, 0x23, 0x34, 0x0a, 0x05, 0x89, 0x78,
		0xa3, 0x66, 0x8c, 0x0d, 0x97, 0x55, 0x53, 0xca,
		0xb5, 0x76, 0x18, 0x62, 0x29, 0xc6, 0xb6, 0x79,
		0x75, 0xc8, 0x5a, 0x8d, 0x9e, 0x11, 0x8f, 0x85,
		0xde, 0xc4, 0x5f, 0x66, 0x21, 0x52, 0xf9, 0x39,
		0xd9, 0x77, 0x93, 0x28, 0xb0, 0x5e, 0x02, 0xfa,
		0x58, 0xb4, 0x16, 0xc8, 0x0f, 0x38, 0x91, 0xbb,
		0x28, 0x17, 0xcd, 0x8a, 0xc9, 0x53, 0x72, 0x66,
	},
#if defined(STM32_BL2)
#if defined(MCUBOOT_SIGN_RSA) && (MCUBOOT_SIGN_RSA_LEN == 3072)
	.bl2_rotpk_0 = {
		0xbf, 0xe6, 0xd8, 0x6f, 0x88, 0x26, 0xf4, 0xff,
		0x97, 0xfb, 0x96, 0xc4, 0xe6, 0xfb, 0xc4, 0x99,
		0x3e, 0x46, 0x19, 0xfc, 0x56, 0x5d, 0xa2, 0x6a,
		0xdf, 0x34, 0xc3, 0x29, 0x48, 0x9a, 0xdc, 0x38,
	},
	.bl2_rotpk_1 = {
		0xb3, 0x60, 0xca, 0xf5, 0xc9, 0x8c, 0x6b, 0x94,
		0x2a, 0x48, 0x82, 0xfa, 0x9d, 0x48, 0x23, 0xef,
		0xb1, 0x66, 0xa9, 0xef, 0x6a, 0x6e, 0x4a, 0xa3,
		0x7c, 0x19, 0x19, 0xed, 0x1f, 0xcc, 0xc0, 0x49,
	},
	.bl2_rotpk_2 = {
		0xbf, 0xe6, 0xd8, 0x6f, 0x88, 0x26, 0xf4, 0xff,
		0x97, 0xfb, 0x96, 0xc4, 0xe6, 0xfb, 0xc4, 0x99,
		0x3e, 0x46, 0x19, 0xfc, 0x56, 0x5d, 0xa2, 0x6a,
		0xdf, 0x34, 0xc3, 0x29, 0x48, 0x9a, 0xdc, 0x38,
	},
#elif defined(MCUBOOT_SIGN_EC256)
	.bl2_rotpk_0 = {
		0xe3, 0x04, 0x66, 0xf6, 0xb8, 0x47, 0x0c, 0x1f, \
		0x29, 0x07, 0x0b, 0x17, 0xf1, 0xe2, 0xd3, 0xe9, \
		0x4d, 0x44, 0x5e, 0x3f, 0x60, 0x80, 0x87, 0xfd, \
		0xc7, 0x11, 0xe4, 0x38, 0x2b, 0xb5, 0x38, 0xb6, \
	},
	.bl2_rotpk_1 = {
		0x82, 0xa5, 0xb4, 0x43, 0x59, 0x48, 0x53, 0xd4, \
		0xbf, 0x0f, 0xdd, 0x89, 0xa9, 0x14, 0xa5, 0xdc, \
		0x16, 0xf8, 0x67, 0x54, 0x82, 0x07, 0xd7, 0x07, \
		0x7e, 0x74, 0xd8, 0x0c, 0x06, 0x3e, 0xfd, 0xa9, \
	},
	.bl2_rotpk_2 = {
		0xe3, 0x04, 0x66, 0xf6, 0xb8, 0x47, 0x0c, 0x1f, \
		0x29, 0x07, 0x0b, 0x17, 0xf1, 0xe2, 0xd3, 0xe9, \
		0x4d, 0x44, 0x5e, 0x3f, 0x60, 0x80, 0x87, 0xfd, \
		0xc7, 0x11, 0xe4, 0x38, 0x2b, 0xb5, 0x38, 0xb6, \
	},
#else
#error "TFM_DUMMY_PROVISIONING: Please choose between EC-P256 or RSA-3072 for image signatures."
#endif
#endif
};
#endif /* TFM_DUMMY_PROVISIONING */

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

static const struct device *nvmem_dev_from_otp_id(enum tfm_otp_element_id_t id)
{
	const struct device *dev;

	switch (id) {
#if defined(STM32_BL2)
	case PLAT_OTP_ID_BL2_ROTPK_0:
		dev = DT_INST_DEV_NVMEM(0, bl2_rotpk_0);
		break;
	case PLAT_OTP_ID_BL2_ROTPK_1:
		dev = DT_INST_DEV_NVMEM(0, bl2_rotpk_1);
		break;
	case PLAT_OTP_ID_BL2_ROTPK_2:
		dev = DT_INST_DEV_NVMEM(0, bl2_rotpk_2);
		break;
	case PLAT_OTP_ID_BL2_ROTPK_3:
		dev = DT_INST_DEV_NVMEM(0, bl2_rotpk_3);
		break;
#endif
	case PLAT_OTP_ID_IAK:
		dev = DT_INST_DEV_NVMEM(0, iak);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		dev = DT_INST_DEV_NVMEM(0, implementation_id);
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		dev = DT_INST_DEV_NVMEM(0, entropy_seed);
		break;
	case PLAT_OTP_ID_RPN:
		dev = DT_INST_DEV_NVMEM(0, rpn_otp);
		break;
	case PLAT_OTP_ID_REV_ID:
		dev = DT_INST_DEV_NVMEM(0, id_otp);
		break;
	case PLAT_OTP_ID_PACKAGE:
		dev = DT_INST_DEV_NVMEM(0, package_otp);
		break;
	case PLAT_OTP_ID_BOARD_ID:
		dev = DT_INST_DEV_NVMEM(0, board_id);
		break;
	default:
		dev = NULL;
		break;
	}

	return dev;
}

#if TFM_DUMMY_PROVISIONING
static enum tfm_plat_err_t stm32_check_otp_check_value(size_t out_len, uint8_t *out)
{
	for (int i = 0; i < out_len; i++) {
		if (out[i] != 0)
			return TFM_PLAT_ERR_SUCCESS;
	}

	return TFM_PLAT_ERR_INVALID_INPUT;
}

static enum tfm_plat_err_t stm32_set_default_value(enum tfm_otp_element_id_t id,
						   size_t out_len, uint8_t *out)
{
	switch (id) {
	case PLAT_OTP_ID_IAK_LEN:
		if (out_len < sizeof(size_t))
			return TFM_PLAT_ERR_INVALID_INPUT;

		*out = sizeof(psa_rot_prov_data.iak);
		break;
	case PLAT_OTP_ID_IAK:
		if (out_len > sizeof(psa_rot_prov_data.iak))
			return TFM_PLAT_ERR_INVALID_INPUT;

		memcpy((void *)out, psa_rot_prov_data.iak, out_len);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		if (out_len > sizeof(psa_rot_prov_data.implementation_id))
			return TFM_PLAT_ERR_INVALID_INPUT;

		memcpy((void *)out, psa_rot_prov_data.implementation_id, out_len);
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		if (out_len > sizeof(psa_rot_prov_data.entropy_seed))
			return TFM_PLAT_ERR_INVALID_INPUT;

		memcpy((void *)out, psa_rot_prov_data.entropy_seed, out_len);
		break;
#if defined(STM32_BL2)
	case PLAT_OTP_ID_BL2_ROTPK_0:
		if (out_len > sizeof(psa_rot_prov_data.bl2_rotpk_0))
			return TFM_PLAT_ERR_INVALID_INPUT;

		memcpy((void*)out, psa_rot_prov_data.bl2_rotpk_0, out_len);
		break;
	case PLAT_OTP_ID_BL2_ROTPK_1:
		if (out_len > sizeof(psa_rot_prov_data.bl2_rotpk_1))
			return TFM_PLAT_ERR_INVALID_INPUT;

		memcpy((void*)out, psa_rot_prov_data.bl2_rotpk_1, out_len);
		break;
	case PLAT_OTP_ID_BL2_ROTPK_2:
		if (out_len > sizeof(psa_rot_prov_data.bl2_rotpk_2))
			return TFM_PLAT_ERR_INVALID_INPUT;

		memcpy((void *)out, psa_rot_prov_data.bl2_rotpk_2, out_len);
		break;
#endif
	default:
		return TFM_PLAT_ERR_UNSUPPORTED;
	}

	return TFM_PLAT_ERR_SUCCESS;
}
#endif

static enum tfm_plat_err_t otp_read_by_nvmem(const struct device *dev_nvmem,
					     size_t out_len, uint8_t *out)
{
	size_t read_len = 0;
	int err;

	if (!dev_nvmem)
		return TFM_PLAT_ERR_UNSUPPORTED;

	err = nvmem_read_cell(dev_nvmem, out_len, out, &read_len);
	if (read_len != out_len) {
		memset(out, 0, out_len);
		return TFM_PLAT_ERR_NOT_PERMITTED;
	}

	return TFM_PLAT_ERR_SUCCESS;
}

#define BOOTROM_CFG_9_SEC_BOOT	GENMASK(3, 0)
#define BOOTROM_CFG_9_PROV_DONE	GENMASK(7, 4)
#define HCONF1_DISABLE_SCAN	BIT(20)
static int otp_read_lcs(uint32_t out_len, uint8_t *out)
{
	uint32_t secure_boot, disable_scan, prov_done;
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t *)out;
	uint32_t bootrom_cfg_9, hconf1;
	int err;

	*lcs = PLAT_OTP_LCS_ASSEMBLY_AND_TEST;

	err = otp_read_by_nvmem(DT_INST_DEV_NVMEM(0, bootrom_config_9),
				sizeof(uint32_t), (uint8_t *)&bootrom_cfg_9);
	if (!err)
		return err;

	err = otp_read_by_nvmem(DT_INST_DEV_NVMEM(0, hconf1_otp),
				sizeof(uint32_t), (uint8_t *)&hconf1);
	if (!err)
		return err;

	/* true if all bit of field are set */
	secure_boot = !!(bootrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT);
	prov_done = !!(bootrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE);
	disable_scan = !!(hconf1 & HCONF1_DISABLE_SCAN);

	if (secure_boot && prov_done && disable_scan)
		*lcs = PLAT_OTP_LCS_SECURED;

	return 0;
}

enum tfm_plat_err_t tfm_plat_otp_read(enum tfm_otp_element_id_t id,
                                      size_t out_len, uint8_t *out)
{
	const struct device *dev_nvmem;
	size_t read_len = 0;
	size_t cell_size = 0;
	int err;

	switch (id) {
	case PLAT_OTP_ID_LCS:
		err = otp_read_lcs(out_len, out);
		break;
	case PLAT_OTP_ID_IAK_LEN:
		dev_nvmem = DT_INST_DEV_NVMEM(0, iak);
		if (!dev_nvmem)
			return TFM_PLAT_ERR_UNSUPPORTED;

		err = nvmem_get_cell_size(dev_nvmem, &read_len);
		if (!err)
			memcpy(out, &read_len, sizeof(size_t));

		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		dev_nvmem = DT_INST_DEV_NVMEM(0, implementation_id);
		if (!dev_nvmem)
			return TFM_PLAT_ERR_UNSUPPORTED;

		/*
		 * On STM32MP2 platform, the Implementation ID is lower than the max size
		 * (currently 12 bytes vs. 32 bytes for the max).
		 */
		err = nvmem_get_cell_size(dev_nvmem, &cell_size);
		if (!err) {
			err = nvmem_read_cell(dev_nvmem, cell_size, out, &read_len);
#if TFM_DUMMY_PROVISIONING
			if (!err && stm32_check_otp_check_value(cell_size, out))
				err = stm32_set_default_value(id, cell_size, out);
#endif
		}
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
		dev_nvmem = nvmem_dev_from_otp_id(id);
		if (!dev_nvmem)
			return TFM_PLAT_ERR_UNSUPPORTED;

		err = nvmem_read_cell(dev_nvmem, out_len, out, &read_len);
		if (read_len != out_len) {
			memset(out, 0, out_len);
			return TFM_PLAT_ERR_NOT_PERMITTED;
		}
#if TFM_DUMMY_PROVISIONING
#if !STM32_OVERRIDE_OTP
		if (!err && stm32_check_otp_check_value(out_len, out))
#endif
			err = stm32_set_default_value(id, out_len, out);
#endif
		break;
	}

	if (err)
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_get_size(enum tfm_otp_element_id_t id, size_t *size)
{
	const struct device *dev;
	int err = 0;

	switch (id) {
	case PLAT_OTP_ID_LCS:
		*size = sizeof(uint32_t);
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
		dev = nvmem_dev_from_otp_id(id);
		if (!dev)
			return TFM_PLAT_ERR_UNSUPPORTED;

		err = nvmem_get_cell_size(dev, size);
		break;
	}

	if (err)
		return TFM_PLAT_ERR_SYSTEM_ERR;

	return TFM_PLAT_ERR_SUCCESS;
}

enum tfm_plat_err_t tfm_plat_otp_write(enum tfm_otp_element_id_t id, size_t in_len,
				       const uint8_t *in)
{
	return TFM_PLAT_ERR_NOT_PERMITTED;
}

enum tfm_plat_err_t tfm_plat_otp_init(void)
{
	return TFM_PLAT_ERR_SUCCESS;
}

#define NVMEM_ELEM(node_id, prop, idx)								\
	DT_NVMEM_SPEC_GET_BY_IDX(node_id, idx)

#define STM32_OTP_CONFIG(n)									\
												\
static const struct nvmem_dt_spec stm32_otp_cfg_##n[DT_INST_PROP_LEN(n, nvmem_cells)] = {	\
	DT_INST_FOREACH_PROP_ELEM_SEP(n, nvmem_cells, NVMEM_ELEM, (,))				\
};												\
												\
DEVICE_DT_INST_DEFINE(n, NULL, NULL,								\
		      NULL,									\
		      &stm32_otp_cfg_##n,							\
		      CORE, 7,									\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_OTP_CONFIG)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "only one otp compatible node is supported");
