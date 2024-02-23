/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <cmsis_compiler.h>

#include <config_attest.h>
#include <tfm_plat_provisioning.h>
#include <tfm_plat_otp.h>
#include <tfm_attest_hal.h>
#include <psa/crypto.h>
#include <debug.h>

#ifdef TFM_DUMMY_PROVISIONING
#define PSA_ROT_PROV_DATA_MAGIC           0xBEEFFEED

__PACKED_STRUCT tfm_psa_rot_provisioning_data_t {
	uint32_t magic;
	uint8_t iak[32];
	uint8_t implementation_id[12];
	uint8_t entropy_seed[64];
#if defined(STM32_BL2)
	uint8_t bl2_rotpk_0[32];
	uint8_t bl2_rotpk_1[32];
#endif
};

static const struct tfm_psa_rot_provisioning_data_t psa_rot_prov_data = {
	.magic = PSA_ROT_PROV_DATA_MAGIC,
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
#endif
};

enum tfm_plat_err_t provision_psa_rot(void)
{
	enum tfm_plat_err_t err;
	uint32_t new_lcs;

	err = tfm_plat_otp_write(PLAT_OTP_ID_IAK,
				 sizeof(psa_rot_prov_data.iak),
				 psa_rot_prov_data.iak);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_write(PLAT_OTP_ID_IMPLEMENTATION_ID,
				 sizeof(psa_rot_prov_data.implementation_id),
				 psa_rot_prov_data.implementation_id);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	err = tfm_plat_otp_write(PLAT_OTP_ID_ENTROPY_SEED,
				 sizeof(psa_rot_prov_data.entropy_seed),
				 psa_rot_prov_data.entropy_seed);
	if (err != TFM_PLAT_ERR_SUCCESS && err != TFM_PLAT_ERR_UNSUPPORTED) {
		return err;
	}

	new_lcs = PLAT_OTP_LCS_SECURED;
	err = tfm_plat_otp_write(PLAT_OTP_ID_LCS,
				 sizeof(new_lcs),
				 (uint8_t*)&new_lcs);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

#if defined(STM32_BL2)
	err = tfm_plat_otp_write(PLAT_OTP_ID_BL2_ROTPK_0,
				 sizeof(psa_rot_prov_data.bl2_rotpk_0),
				 psa_rot_prov_data.bl2_rotpk_0);
	if (err != TFM_PLAT_ERR_SUCCESS && err != TFM_PLAT_ERR_UNSUPPORTED)
		return err;

	err = tfm_plat_otp_write(PLAT_OTP_ID_BL2_ROTPK_1,
				 sizeof(psa_rot_prov_data.bl2_rotpk_1),
				 psa_rot_prov_data.bl2_rotpk_1);
	if (err != TFM_PLAT_ERR_SUCCESS && err != TFM_PLAT_ERR_UNSUPPORTED)
		return err;
#endif

	return err;
}

enum tfm_plat_err_t tfm_plat_provisioning_perform(void)
{
	enum tfm_plat_err_t err;
	enum plat_otp_lcs_t lcs;

	err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(lcs), (uint8_t*)&lcs);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	IMSG("Beginning provisioning\033[1;31m");
	IMSG("DUMMY_PROVISIONING is not suitable for production! ");
	IMSG("This device is \033[1;1mNOT SECURE\033[0m");

	if (lcs == PLAT_OTP_LCS_ASSEMBLY_AND_TEST) {
		if (psa_rot_prov_data.magic != PSA_ROT_PROV_DATA_MAGIC) {
			EMSG("No valid PSA_ROT provisioning data found\r\n");
			return TFM_PLAT_ERR_INVALID_INPUT;
		}

		err = stm32_otp_dummy_prep();
		if (err != TFM_PLAT_ERR_SUCCESS) {
			return err;
		}

		err = provision_psa_rot();
		if (err != TFM_PLAT_ERR_SUCCESS) {
			return err;
		}
	}

	return TFM_PLAT_ERR_SUCCESS;
}
#else
enum tfm_plat_err_t tfm_plat_provisioning_perform(void)
{
	return TFM_PLAT_ERR_NOT_PERMITTED;
}
#endif /* TFM_DUMMY_PROVISIONING */

void tfm_plat_provisioning_check_for_dummy_keys(void)
{
	uint64_t iak_start;

	tfm_plat_otp_read(PLAT_OTP_ID_IAK, sizeof(iak_start), (uint8_t*)&iak_start);

	if(iak_start == 0xA4906F6DB254B4A9) {
		EMSG("[WRN]\033[1;31m ");
		EMSG("This device was provisioned with dummy keys. ");
		EMSG("This device is \033[1;1mNOT SECURE");
		EMSG("\033[0m\r\n");
	}

	memset(&iak_start, 0, sizeof(iak_start));
}

int tfm_plat_provisioning_is_required(void)
{
	enum tfm_plat_err_t err;
	enum plat_otp_lcs_t lcs;

	err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(lcs), (uint8_t*)&lcs);
	if (err != TFM_PLAT_ERR_SUCCESS) {
		return err;
	}

	return lcs == PLAT_OTP_LCS_ASSEMBLY_AND_TEST
		|| lcs == PLAT_OTP_LCS_PSA_ROT_PROVISIONING;
}
