/*
 * Copyright (c) 2026, STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <tfm_plat_bl2_fwu.h>

#include <errno.h>
#include <string.h>
#include <devicetree.h>
#include <devicetree/nvmem.h>
#include <nvmem.h>

#include <debug.h>

#define		STM32_M33_FWU_FAIL	BIT(31)

#if STM32_BL2
#define		_NVMEM_FWU_INFO_FAIL DEVICE_GET(DEVICE_DT_DEV_ID(DT_NODELABEL(cm_fwu_info)))

void tfm_plat_bl2_notify_init(void)
{
	size_t cell_size = 0;
	int err = 0;
	int reg = 0;

	err = nvmem_get_cell_size(_NVMEM_FWU_INFO_FAIL, &cell_size);
	if (err)
		goto fail;

	if (cell_size != sizeof(uint32_t))
		goto fail;

	err = nvmem_write_cell(_NVMEM_FWU_INFO_FAIL, cell_size, (uint8_t *)&reg);
fail:
	if (err)
		WMSG("%s failed", __func__);
}

void tfm_plat_bl2_notify_erase(void)
{
	size_t cell_size = 0;
	size_t read_len = 0;
	int err = 0;
	uint32_t reg;

	err = nvmem_get_cell_size(_NVMEM_FWU_INFO_FAIL, &cell_size);
	if (err)
		goto fail;

	if (cell_size != sizeof(uint32_t))
		goto fail;

	err = nvmem_read_cell(_NVMEM_FWU_INFO_FAIL, cell_size, (uint8_t *)&reg, &read_len);
	if (err)
		goto fail;

	reg |= STM32_M33_FWU_FAIL;
	err = nvmem_write_cell(_NVMEM_FWU_INFO_FAIL, cell_size, (uint8_t *)&reg);
fail:
	if (err)
		WMSG("%s failed", __func__);
}
#else /* STM32_BL2 */
void tfm_plat_bl2_notify_init(void)
{
}

void tfm_plat_bl2_notify_erase(void)
{
}
#endif
