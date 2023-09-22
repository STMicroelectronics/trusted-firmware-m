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
#include <tfm_spm_log.h>

#ifdef TFM_DUMMY_PROVISIONING
#define PSA_ROT_PROV_DATA_MAGIC           0xBEEFFEED

__PACKED_STRUCT tfm_psa_rot_provisioning_data_t {
	uint32_t magic;
	uint8_t iak[32];
	uint8_t implementation_id[12];
	uint8_t entropy_seed[64];
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

    SPMLOG_INFMSG("[INF] Beginning TF-M provisioning\r\n");

    SPMLOG_ERRMSG("[WRN]\033[1;31m ");
    SPMLOG_ERRMSG("TFM_DUMMY_PROVISIONING is not suitable for production! ");
    SPMLOG_ERRMSG("This device is \033[1;1mNOT SECURE");
    SPMLOG_ERRMSG("\033[0m\r\n");

    if (lcs == PLAT_OTP_LCS_ASSEMBLY_AND_TEST) {
        if (psa_rot_prov_data.magic != PSA_ROT_PROV_DATA_MAGIC) {
            SPMLOG_ERRMSG("No valid PSA_ROT provisioning data found\r\n");
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
        SPMLOG_ERRMSG("[WRN]\033[1;31m ");
        SPMLOG_ERRMSG("This device was provisioned with dummy keys. ");
        SPMLOG_ERRMSG("This device is \033[1;1mNOT SECURE");
        SPMLOG_ERRMSG("\033[0m\r\n");
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
