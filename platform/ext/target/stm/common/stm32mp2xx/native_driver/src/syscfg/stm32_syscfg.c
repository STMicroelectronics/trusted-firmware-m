/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define DT_DRV_COMPAT st_stm32mp25_syscfg

#include <errno.h>
#include <stdint.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <device.h>
#include <syscon.h>

#define _SYSCFG_SIDR			U(0x7FFC)

struct stm32_syscfg_config {
	uintptr_t base;
};

static int stm32_syscfg_read(const struct device *dev, uint16_t off,
			     uint32_t *val)
{
	const struct stm32_syscfg_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;

	if (off > _SYSCFG_SIDR) {
		return -EINVAL;
	}

	*val = mmio_read_32(base + off);

	return 0;
}

static int stm32_syscfg_write(const struct device *dev, uint16_t off,
			      uint32_t val)
{
	const struct stm32_syscfg_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;

	if (off > _SYSCFG_SIDR) {
		return -EINVAL;
	}

	mmio_write_32(base + off, val);

	return 0;
}

static int stm32_syscfg_clrsetbits(const struct device *dev, uint16_t off,
				   uint32_t clr, uint32_t set)
{
	const struct stm32_syscfg_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;

	if (off > _SYSCFG_SIDR) {
		return -EINVAL;
	}

	mmio_clrsetbits_32(base + off, clr, set);

	return 0;
}

static int stm32_syscfg_clrbits(const struct device *dev, uint16_t off,
				uint32_t clr)
{
	const struct stm32_syscfg_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;

	if (off > _SYSCFG_SIDR) {
		return -EINVAL;
	}

	mmio_clrbits_32(base + off, clr);

	return 0;
}

static int stm32_syscfg_setbits(const struct device *dev, uint16_t off,
				uint32_t set)
{
	const struct stm32_syscfg_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;

	if (off > _SYSCFG_SIDR) {
		return -EINVAL;
	}

	mmio_setbits_32(base + off, set);

	return 0;
}

static const struct syscon_driver_api stm32_syscfg_com_api = {
	.read = stm32_syscfg_read,
	.write = stm32_syscfg_write,
	.clrsetbits = stm32_syscfg_clrsetbits,
	.clrbits = stm32_syscfg_clrbits,
	.setbits = stm32_syscfg_setbits,
};

static const struct stm32_syscfg_config stm32_syscfg_cfg = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0,
		      NULL,
		      NULL, &stm32_syscfg_cfg,
		      PRE_CORE, 0,
		      &stm32_syscfg_com_api);
