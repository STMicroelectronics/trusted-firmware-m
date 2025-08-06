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
#include <stm32_dcache.h>
#include <strings.h>
#include <tfm_sp_log.h>
#include <uapi/tfm_pm_api.h>

#include "tfm_pm.h"

extern const uint8_t __tfm_lp_fw_start[];
extern const uint8_t __tfm_lp_fw_end[];

/*
 * the CRC computation is calculation follow:
 * nb RETRAM_BUF_SZ (16K) Starting from RETRAM start address
 */
#define RETRAM_BUF_SZ	(16 * 1024)

uint32_t retram_crc_enable(size_t buf_size)
{
	uint32_t crcbs = div_round_up(buf_size, RETRAM_BUF_SZ) - 1;

	/* deactivate the CRC */
	MODIFY_REG(RAMCFG_RETRAM->CCR1, RAMCFG_CCR1_CRCC, 0);

	/* disable hardware erase */
	SET_BIT(RAMCFG_RETRAM->CR, RAMCFG_CR_SRAMHWERDIS);

	/* Select the buffer size to take into account during the CRC computation */
	RAMCFG_RETRAM->CCR1 = (crcbs << RAMCFG_CCR1_CRCBS_Pos) |
		((0x1) << RAMCFG_CCR1_CRCC_Pos);

	/* start the CRC computation */
	SET_BIT(RAMCFG_RETRAM->CCR2, RAMCFG_CCR2_CRCCS);

	/* wait RETRAM computation is OK */
	while (RAMCFG_RETRAM->CSR != RAMCFG_CSR_CRCEOC) {}

	/* Clear the CRC status */
	SET_BIT(RAMCFG_RETRAM->CCR2, RAMCFG_CCR2_CRCFC);

	/* Write 0x0 in CRCC[1:0] bits of the RAMCFG_xCCR1 to deactivate the CRC */
	MODIFY_REG(RAMCFG_RETRAM->CCR1, RAMCFG_CCR1_CRCC, 0);

	/* Save calculated signature in reference signature */
	RAMCFG_RETRAM->CRSR = RAMCFG_RETRAM->CCSR;

	/* Enable CRC + Signature check */
	RAMCFG_RETRAM->CCR1 = (crcbs << RAMCFG_CCR1_CRCBS_Pos) |
		((0x2) << RAMCFG_CCR1_CRCC_Pos);
}

static psa_status_t tfm_pm_load_fw()
{
	size_t fw_size = __tfm_lp_fw_end - __tfm_lp_fw_start;
	uintptr_t dst_addr = DT_REG_ADDR(DT_NODELABEL(cm33_retram));
	size_t dst_sz = DT_REG_SIZE(DT_NODELABEL(cm33_retram));
	size_t crc_buffer_sz;

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

	/* enable retram crc computation */
	retram_crc_enable(crc_buffer_sz);

	return PSA_SUCCESS;
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
	return tfm_pm_load_fw();
}
