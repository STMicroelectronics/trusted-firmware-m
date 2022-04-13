/*
 * Copyright (c) 2018-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <debug.h>

#include <rstctrl.h>
#include <stm32_reset.h>

#include <dt-bindings/reset/stm32mp25-resets.h>

#define RESET_ID_MASK		GENMASK_32(31, 5)
#define RESET_ID_SHIFT		5
#define RESET_BIT_POS_MASK	GENMASK_32(4, 0)
#define RESET_OFFSET_MAX	1024

#define RESET_OFFSET(__id)	(((__id & RESET_ID_MASK) >> RESET_ID_SHIFT) \
				 * sizeof(uint32_t))
#define RESET_BIT(__id)		BIT((__id & RESET_BIT_POS_MASK))

#define _RCC_C1RSTCSETR		U(0x404)
#define _RCC_CPUBOOTCR		U(0x434)

#define CPUX(__offset)		__offset - _RCC_C1RSTCSETR

/*
 * no common errno between component
 * define iac internal errno
 */
#define	RESET_ERR_NOMEM		12	/* Out of memory */
#define RESET_ERR_NODEV		19	/* No such device */
#define RESET_ERR_INVAL		22	/* Invalid argument */
#define RESET_ERR_NOTSUP	45	/* Operation not supported */

static struct stm32_reset_platdata rst_pdata;

static struct stm32_rstline *to_rstline(struct rstctrl *rstctrl)
{
	assert(rstctrl);

	return container_of(rstctrl, struct stm32_rstline, rstctrl);
}

static int stm32_reset_assert(struct rstctrl *rstctrl, unsigned int to_us)
{
	unsigned int id = to_rstline(rstctrl)->id;
	uintptr_t addr = rst_pdata.base + RESET_OFFSET(id);
	uint32_t rst_mask = RESET_BIT(id);
	uint32_t cfgr;

	io_setbits32(addr, rst_mask);

	if (!to_us)
		return 0;

	return  mmio_read32_poll_timeout(addr, cfgr, (cfgr & rst_mask), to_us);
}

static int stm32_reset_deassert(struct rstctrl *rstctrl, unsigned int to_us)
{
	unsigned int id = to_rstline(rstctrl)->id;
	uintptr_t addr = rst_pdata.base + RESET_OFFSET(id);
	uint32_t rst_mask = RESET_BIT(id);
	uint32_t cfgr;

	io_clrbits32(addr, rst_mask);

	if (!to_us)
		return 0;

	return mmio_read32_poll_timeout(addr, cfgr, (~cfgr & rst_mask), to_us);
}

#define CPUBOOT_BIT(__offset)	__offset == _RCC_C1RSTCSETR ? BIT(1) : BIT(0)

static int stm32_reset_cpu_assert(struct rstctrl *rstctrl, unsigned int to_us)
{
	unsigned int id = to_rstline(rstctrl)->id;
	uintptr_t base = rst_pdata.base;
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

static int stm32_reset_cpu_deassert(struct rstctrl *rstctrl, unsigned int to_us)
{
	unsigned int id = to_rstline(rstctrl)->id;
	uintptr_t base = rst_pdata.base;
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

static struct rstctrl_ops stm32_rstctrl_ops = {
	.assert_level = stm32_reset_assert,
	.deassert_level = stm32_reset_deassert,
};

static struct rstctrl_ops stm32_rstctrl_cpu_ops = {
	.assert_level = stm32_reset_cpu_assert,
	.deassert_level = stm32_reset_cpu_deassert,
};

/*
 * This function could be overridden by platform to define
 * pdata of iac driver
 */
__weak int stm32_reset_get_platdata(struct stm32_reset_platdata *pdata)
{
	return -RESET_ERR_NODEV;
}

int stm32_reset_init(void)
{
	return stm32_reset_get_platdata(&rst_pdata);
}

DEFINE_STM32_RSTLINE(OSPI1DLL_R, &stm32_rstctrl_ops);
DEFINE_STM32_RSTLINE(OSPI1_R, &stm32_rstctrl_ops);
DEFINE_STM32_RSTLINE(CPU1_R, &stm32_rstctrl_cpu_ops);
