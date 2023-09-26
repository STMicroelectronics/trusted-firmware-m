/*
 * Copyright (c) 2018-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_rcc_rctl

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <debug.h>

#include <device.h>
#include <reset.h>

#include <dt-bindings/reset/stm32mp25-resets.h>

#define RESET_ID_MASK		GENMASK_32(31, 5)
#define RESET_ID_SHIFT		5
#define RESET_BIT_POS_MASK	GENMASK_32(4, 0)
#define RESET_OFFSET_MAX	1024

#define RESET_OFFSET(__id)	(((__id & RESET_ID_MASK) >> RESET_ID_SHIFT) \
				 * sizeof(uint32_t))
#define RESET_BIT(__id)		BIT((__id & RESET_BIT_POS_MASK))

/* registers offset */
#define _RCC_BDCR		U(0X400)
#define _RCC_C1RSTCSETR		U(0x404)
#define _RCC_CPUBOOTCR		U(0x434)

#define _RCC_BDCR_RTCSRC_MASK	GENMASK(17, 16)

#define CPUBOOT_BIT(__offset)	__offset == _RCC_C1RSTCSETR ? BIT(1) : BIT(0)

#define TIMEOUT_RESET_US	USEC_PER_MSEC

struct stm32_reset_config {
	uintptr_t base;
};

static int _assert(const struct device *dev, uint32_t id, unsigned int to_us)
{
	const struct stm32_reset_config *drv_cfg = dev_get_config(dev);
	uintptr_t addr = drv_cfg->base + RESET_OFFSET(id);
	uint32_t rst_mask = RESET_BIT(id);
	uint32_t cfgr;

	io_setbits32(addr, rst_mask);

	if (!to_us)
		return 0;

	return  mmio_read32_poll_timeout(addr, cfgr, (cfgr & rst_mask), to_us);
}

static int _deassert(const struct device *dev, uint32_t id, unsigned int to_us)
{
	const struct stm32_reset_config *drv_cfg = dev_get_config(dev);
	uintptr_t addr = drv_cfg->base + RESET_OFFSET(id);
	uint32_t rst_mask = RESET_BIT(id);
	uint32_t cfgr;

	io_clrbits32(addr, rst_mask);

	if (!to_us)
		return 0;

	return mmio_read32_poll_timeout(addr, cfgr, (~cfgr & rst_mask), to_us);
}

int _stm32_reset_status(const struct device *dev, uint32_t id)
{
	const struct stm32_reset_config *drv_cfg = dev_get_config(dev);
	uintptr_t addr = drv_cfg->base + RESET_OFFSET(id);
	uint32_t reg;

	reg = io_read32(addr);

	return !!(reg & RESET_BIT(id));
}

int _stm32_reset_assert(const struct device *dev, uint32_t id)
{
	return _assert(dev, id, 0);
}

int _stm32_reset_deassert(const struct device *dev, uint32_t id)
{
	return _deassert(dev, id, 0);
}

int _stm32_reset_reset(const struct device *dev, uint32_t id)
{
	int err;

	err = _assert(dev, id, TIMEOUT_RESET_US);
	if (err)
		return err;

	return _deassert(dev, id, TIMEOUT_RESET_US);
}

static const struct reset_driver_api stm32_reset_ops = {
	.status = _stm32_reset_status,
	.assert_level = _stm32_reset_assert,
	.deassert_level = _stm32_reset_deassert,
	.reset = _stm32_reset_reset,
};

static int _cpu_assert(const struct device *dev, uint32_t id,
		       unsigned int to_us)
{
	const struct stm32_reset_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;
	uint32_t rst_offset = RESET_OFFSET(id);
	uint32_t rst_mask = RESET_BIT(id);
	uint32_t cpu_mask = CPUBOOT_BIT(rst_offset);
	uint32_t cfgr;
	int err = 0;

	/* put in HOLD: enable HOLD boot & reset */
	io_clrbits32(base + _RCC_CPUBOOTCR, cpu_mask);
	err = mmio_read32_poll_timeout(base + _RCC_CPUBOOTCR,
				       cfgr, (~cfgr & cpu_mask), to_us);
	if (err)
		return err;

	io_setbits32(base + rst_offset, rst_mask);
	if (!to_us)
		return 0;

	return mmio_read32_poll_timeout(base + rst_offset, cfgr,
					(~cfgr & rst_mask), to_us);
}

static int _cpu_deassert(const struct device *dev, uint32_t id,
		       unsigned int to_us)
{
	const struct stm32_reset_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;
	uint32_t rst_offset = RESET_OFFSET(id);
	uint32_t rst_mask = RESET_BIT(id);
	uint32_t cpu_mask = CPUBOOT_BIT(rst_offset);
	uint32_t cfgr;
	int err = 0;

	/* release HOLD: disable HOLD boot & reset */
	io_setbits32(base + _RCC_CPUBOOTCR, cpu_mask);
	err = mmio_read32_poll_timeout(base + _RCC_CPUBOOTCR,
				       cfgr, (cfgr & rst_mask), to_us);
	if (err)
		return err;

	io_setbits32(base + rst_offset, rst_mask);
	if (!to_us)
		return 0;

	return mmio_read32_poll_timeout(base + rst_offset, cfgr,
					(~cfgr & rst_mask), to_us);
}

int _stm32_reset_cpu_assert(const struct device *dev, uint32_t id)
{
	return _cpu_assert(dev, id, 0);
}

int _stm32_reset_cpu_deassert(const struct device *dev, uint32_t id)
{
	return _cpu_deassert(dev, id, 0);
}

int _stm32_reset_cpu_reset(const struct device *dev, uint32_t id)
{
	int err;

	err = _cpu_assert(dev, id, TIMEOUT_RESET_US);
	if (err)
		return err;

	return _cpu_deassert(dev, id, TIMEOUT_RESET_US);
}

static const struct reset_driver_api stm32_reset_cpu_ops = {
	.assert_level = _stm32_reset_cpu_assert,
	.deassert_level = _stm32_reset_cpu_deassert,
	.reset = _stm32_reset_cpu_reset
};

int _stm32_reset_vsw_assert(const struct device *dev, uint32_t id)
{
	const struct stm32_reset_config *drv_cfg = dev_get_config(dev);
	uintptr_t base = drv_cfg->base;

	if ((io_read32(base + _RCC_BDCR) & _RCC_BDCR_RTCSRC_MASK))
		return 0;

	/* Reset backup domain on cold boot cases */
	return _assert(dev, id, 0);
}

static const struct reset_driver_api stm32_reset_vsw_ops = {
	.assert_level = _stm32_reset_vsw_assert,
	.deassert_level = _stm32_reset_deassert,
};

#define stm32_reset_op(_op, _dev, _id)			\
({							\
	const struct reset_driver_api *ops;		\
							\
	if (id == C1_R)					\
		ops = &stm32_reset_cpu_ops;		\
	else if (id == VSW_R)				\
		ops = &stm32_reset_vsw_ops;		\
	else						\
		ops = &stm32_reset_ops;			\
							\
	(!ops->_op) ? -ENOSYS : ops->_op(_dev, _id);	\
 })

int stm32_reset_com_status(const struct device *dev, uint32_t id)
{
	return stm32_reset_op(status, dev, id);
}

int stm32_reset_com_assert(const struct device *dev, uint32_t id)
{
	return stm32_reset_op(assert_level, dev, id);
}

int stm32_reset_com_deassert(const struct device *dev, uint32_t id)
{
	return stm32_reset_op(deassert_level, dev, id);
}

int stm32_reset_com_reset(const struct device *dev, uint32_t id)
{
	return stm32_reset_op(reset, dev, id);
}

static const struct reset_driver_api stm32_reset_com_api = {
	.status = stm32_reset_com_status,
	.assert_level = stm32_reset_com_assert,
	.deassert_level = stm32_reset_com_deassert,
	.reset = stm32_reset_com_reset,
};

static const struct stm32_reset_config stm32_reset_cfg = {
	.base = DT_REG_ADDR(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0,
		      NULL,
		      NULL, &stm32_reset_cfg,
		      PRE_CORE, 6,
		      &stm32_reset_com_api);
