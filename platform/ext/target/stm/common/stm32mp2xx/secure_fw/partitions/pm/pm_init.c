/* Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <device.h>
#include <lib/utils_def.h>
#include <psa/error.h>
#include <psa/client.h>
#include <psa/service.h>
#include <service_api.h>
#include <stm32_dcache.h>
#include <stm32mp2_ramcfg.h>
#include <strings.h>
#include <tfm_boot_status.h>
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>
#include <stm32mp2_lp_fw_api.h>
#include "tfm_pm.h"

extern const uint8_t __tfm_lp_fw_start[];
extern const uint8_t __tfm_lp_fw_end[];

#define DDR_ENCRYPT_KEY_SZ	32 /* in bytes (256 bits) */
#define MAX_PLAT_BOOTDATA	(DDR_ENCRYPT_KEY_SZ + \
				 SHARED_DATA_ENTRY_HEADER_SIZE)

/*!
 * \struct platform_boot_data
 *
 * \brief Contains the received boot status information from bootloader
 *
 * \details This is a redefinition of \ref tfm_boot_data to allocate the
 *          appropriate, service dependent size of \ref boot_data.
 */
struct platform_boot_data {
	struct shared_data_tlv_header header;
	uint8_t data[MAX_PLAT_BOOTDATA];
};

/*!
 * \var boot_data
 *
 * \brief Store the boot status in service's memory.
 *
 * \details Boot status comes from the secure bootloader and primarily stored
 *          on a memory area which is shared between bootloader and SPM.
 *          SPM provides the \ref tfm_core_get_boot_data() API to retrieve
 *          the service related data from shared area.
 */
__aligned(4)
static struct platform_boot_data boot_data;

/*
 * Iterates over the TLV section in boot data and returns the address and
 * size of TLV with requested tlv_type
 */
static psa_status_t tlv_extract_data(struct tfm_boot_data *tlv,
				     uint16_t tlv_type,
				     uint8_t **dst,
				     uint8_t *size)
{
	uint32_t data_length = tlv->header.tlv_tot_len - sizeof(*tlv);
	uint8_t *data_end = tlv->data + data_length;
	uint8_t *data = tlv->data;
	struct shared_data_tlv_entry *tlv_entry;

	*size = 0;
	while (data < data_end) {
		tlv_entry = (struct shared_data_tlv_entry *)data;
		data += sizeof(*tlv_entry);
		if (tlv_entry->tlv_type == tlv_type) {
			*dst = data;
			*size = tlv_entry->tlv_len;

			return PSA_SUCCESS;
		}
		//Next_tlv
		data += tlv_entry->tlv_len;
	}

	return PSA_ERROR_INVALID_ARGUMENT;
}

static psa_status_t tfm_pm_load_fw()
{
	size_t fw_size = __tfm_lp_fw_end - __tfm_lp_fw_start;
	uintptr_t dst_addr = DT_REG_ADDR(DT_NODELABEL(cm33_retram));
	size_t dst_sz = DT_REG_SIZE(DT_NODELABEL(cm33_retram));
	size_t crc_buffer_sz;
	psa_status_t tfm_res;
	int ret;
	uint8_t *key;
	uint8_t size;
	bool done;

	if (!fw_size)
		return PSA_ERROR_DOES_NOT_EXIST;

	crc_buffer_sz = div_round_up(fw_size, RETRAM_BUF_SZ) * RETRAM_BUF_SZ;

	if (crc_buffer_sz > dst_sz)
		return PSA_ERROR_GENERIC_ERROR;

	/* set crc_buffer_sz at 0 for computation */
	bzero((void *)dst_addr, crc_buffer_sz);
	memcpy((void *)dst_addr, __tfm_lp_fw_start, fw_size);

	__DMB();
	stm32_dcache_inv((uintptr_t) __tfm_lp_fw_start,
			 (uintptr_t) (__tfm_lp_fw_start + fw_size));

	/* Compute retram CRC for signature */
	ret = stm32_ramcfg_crc_compute(DT_RAMCFG_DEVICE(retram), crc_buffer_sz);
	if (ret)
		return PSA_ERROR_GENERIC_ERROR;

	tfm_res = tfm_core_get_boot_data(TLV_MAJOR_PLATFORM,
					 (struct tfm_boot_data *)&boot_data,
					 sizeof(boot_data));

	/* Set DDR Encryption key when present in shared data */
	if (tfm_res == PSA_SUCCESS &&
	    tlv_extract_data((struct tfm_boot_data *)&boot_data,
			     SET_TLV_TYPE(TLV_MAJOR_PLATFORM, TLV_PLAT_DDRENCKEY),
			     &key, &size) == PSA_SUCCESS) {
		done = stm32mp2_lp_fw_set_mkey(key, size);
	} else {
		done = stm32mp2_lp_fw_set_mkey(NULL, 0);
	}

	return done ? PSA_SUCCESS : PSA_ERROR_GENERIC_ERROR;
}

psa_status_t tfm_pm_service_sfn(const psa_msg_t *msg)
{
    switch (msg->type) {
    case TFM_PM_SUSPEND:
        return tfm_pm_suspend(msg);
    case TFM_PM_POWER_OFF:
        return tfm_pm_power_off();
    default:
        return PSA_ERROR_NOT_SUPPORTED;
    }

    return PSA_ERROR_GENERIC_ERROR;
}

psa_status_t tfm_pm_init(void)
{
	psa_status_t ret;

	ret = tfm_pm_load_fw();
	if (ret) {
		LOG_ERRFMT("[ERR][PM] load failed\r\n");
		return ret;
	}

	ret = tfm_pm_fw_init();
	if (ret) {
		LOG_ERRFMT("[ERR][PM] init failed\r\n");
	}

	return ret;
}
