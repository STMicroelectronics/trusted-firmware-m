/*
 * Copyright (C) 2022-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>

#include <stm32mp_ddr_debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/st/stm32mp_ram.h>
#include <libfdt.h>

#include <platform_def.h>

int stm32mp_ddr_dt_get_info(void *fdt, int node, struct stm32mp_ddr_info *info)
{
	int ret;

	ret = fdt_read_uint32(fdt, node, "st,mem-speed", &info->speed);
	if (ret < 0) {
		DDR_VERBOSE("%s: no st,mem-speed\n", __func__);
		return -EINVAL;
	}
	info->size = dt_get_ddr_size();
	if (info->size == 0U) {
		DDR_VERBOSE("%s: no st,mem-size\n", __func__);
		return -EINVAL;
	}
	info->name = fdt_getprop(fdt, node, "st,mem-name", NULL);
	if (info->name == NULL) {
		DDR_VERBOSE("%s: no st,mem-name\n", __func__);
		return -EINVAL;
	}

	DDR_INFO("RAM: %s\n", info->name);

	return 0;
}

int stm32mp_ddr_dt_get_param(void *fdt, int node, const struct stm32mp_ddr_param *param,
			     uint32_t param_size, uintptr_t config)
{
	int ret;
	uint32_t idx;

	for (idx = 0U; idx < param_size; idx++) {
		ret = fdt_read_uint32_array(fdt, node, param[idx].name, param[idx].size,
					    (void *)(config + param[idx].offset));

		DDR_VERBOSE("%s: %s[0x%x] = %d\n", __func__, param[idx].name, param[idx].size, ret);
		if (ret != 0) {
			DDR_ERROR("%s: Cannot read %s, error=%d\n", __func__, param[idx].name, ret);
			return -EINVAL;
		}
	}

	return 0;
}
