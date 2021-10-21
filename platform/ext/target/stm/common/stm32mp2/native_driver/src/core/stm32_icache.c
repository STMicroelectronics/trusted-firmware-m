// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#include <cmsis.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <strings.h>
#include <lib/utils_def.h>
#include <stm32_icache.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <inttypes.h>
#include <debug.h>

/* ICACHE offset register */
#define _ICACHE_CR		U(0x000)
#define _ICACHE_SR		U(0x004)
#define _ICACHE_IER		U(0x008)
#define _ICACHE_FCR		U(0x00C)
#define _ICACHE_HMONR		U(0x010)
#define _ICACHE_MMONR		U(0x014)
#define _ICACHE_CRR0		U(0x020)
#define _ICACHE_HWCFGR		U(0x3F0)
#define _ICACHE_VERR		U(0x3F4)
#define _ICACHE_IPIDR		U(0x3F8)
#define _ICACHE_SIDR		U(0x3FC)

/* ICACHE_CR register fields */
#define _ICACHE_CR_EN		BIT(0)
#define _ICACHE_CR_CACHEINV	BIT(1)
#define _ICACHE_CR_WAYSEL	BIT(2)
#define _ICACHE_CR_HITMEN	BIT(16)
#define _ICACHE_CR_MISSMEN	BIT(17)
#define _ICACHE_CR_HITMRST	BIT(18)
#define _ICACHE_CR_MISSMRST	BIT(19)

/* ICACHE_SR register fields */
#define _ICACHE_SR_BUSYF	BIT(0)
#define _ICACHE_SR_BUSYENDF	BIT(1)
#define _ICACHE_SR_ERRF		BIT(2)

/* ICACHE_FCR register fields */
#define _ICACHE_FCR_CBUSYENDF	BIT(1)
#define _ICACHE_FCR_CERRF	BIT(2)

#define _ICACHE_CRR_BASEADDR_MASK	GENMASK_32(7, 0)
#define _ICACHE_CRR_BASEADDR_SHIFT	0
#define _ICACHE_CRR_RSIZE_MASK		GENMASK_32(11, 9)
#define _ICACHE_CRR_RSIZE_SHIFT		9
#define _ICACHE_CRR_REN			BIT(15)
#define _ICACHE_CRR_REMAPADDR_MASK	GENMASK_32(26, 16)
#define _ICACHE_CRR_REMAPADDR_SHIFT	16
#define _ICACHE_CRR_MSTSEL		BIT(28)
#define _ICACHE_CRR_HBURST		BIT(31)

/* ICACHE_HWCFGR register fields */
#define _ICACHE_HWCFGR_WAYS_MASK	GENMASK_32(1, 0)
#define _ICACHE_HWCFGR_WAYS_SHIFT	0
#define _ICACHE_HWCFGR_SIZE_MASK	GENMASK_32(6, 4)
#define _ICACHE_HWCFGR_SIZE_SHIFT	4
#define _ICACHE_HWCFGR_LWIDTH_MASK	GENMASK_32(10, 9)
#define _ICACHE_HWCFGR_LWIDTH_SHIFT	9
#define _ICACHE_HWCFGR_RANGE_MASK	GENMASK_32(15, 12)
#define _ICACHE_HWCFGR_RANGE_SHIFT	12
#define _ICACHE_HWCFGR_REGION_MASK	GENMASK_32(18, 16)
#define _ICACHE_HWCFGR_REGION_SHIFT	16
#define _ICACHE_HWCFGR_FMASTER_MASK	GENMASK_32(22, 21)
#define _ICACHE_HWCFGR_FMASTER_SHIFT	21
#define _ICACHE_HWCFGR_SMASTER_MASK	GENMASK_32(24, 23)
#define _ICACHE_HWCFGR_SMASTER_SHIFT	23

/* _ICACHE_VERR register fields */
#define _ICACHE_VERR_MINREV_MASK	GENMASK_32(3, 0)
#define _ICACHE_VERR_MINREV_SHIFT	0
#define _ICACHE_VERR_MAJREV_MASK	GENMASK_32(7, 4)
#define _ICACHE_VERR_MAJREV_SHIFT	4

#define _ICACHE_FLD_PREP(field, value)	(((uint32_t)(value) << (field ## _SHIFT)) & (field ## _MASK))
#define _ICACHE_FLD_GET(field, value)	(((uint32_t)(value) & (field ## _MASK)) >> (field ## _SHIFT))

#define _ICACHE_BUSY_TIMEOUT_US		1000U

#define SZ_1M				0x00100000
/*
 * RI defines the number of significant bits to consider.
 * RI = log2(region size) with a minimum value of 21 (for a 2-Mbyte region)
 */
#define RI_MIN				21
#define _ICACHE_REMAP_MIN_SZ		2 * SZ_1M
#define _ICACHE_REMAP_MAX_SZ		128 * SZ_1M
#define RANGE_2SZ(range)		((1 << range) * SZ_1M)
#define SZ_2RANGE(size)			(ffs(size) - RI_MIN)
#define MS_ADDR(addr, size)		((addr & ~(size - 1)) >> 21)

static struct icache_driver_data icache_drvdata;
static struct stm32_icache_platdata icache_pdata;

static void stm32_icache_get_driverdata(struct stm32_icache_platdata *pdata)
{
	uint32_t regval = 0;

	regval = io_read32(pdata->base + _ICACHE_HWCFGR);
	icache_drvdata.nb_region = _ICACHE_FLD_GET(_ICACHE_HWCFGR_REGION, regval);
	icache_drvdata.ways = _ICACHE_FLD_GET(_ICACHE_HWCFGR_WAYS, regval);
	icache_drvdata.range = RANGE_2SZ(_ICACHE_FLD_GET(_ICACHE_HWCFGR_RANGE, regval));

	pdata->drv_data = &icache_drvdata;

	regval = io_read32(pdata->base + _ICACHE_VERR);

	DMSG("ICACHE version %"PRIu32".%"PRIu32,
	     _ICACHE_FLD_GET(_ICACHE_VERR_MAJREV, regval),
	     _ICACHE_FLD_GET(_ICACHE_VERR_MINREV, regval));

	DMSG("HW cap: nb region:%"PRIu8" ways:%"PRIu8" range:%"PRIu8,
	     icache_drvdata.nb_region,
	     icache_drvdata.ways,
	     icache_drvdata.range);
}

static int stm32_icache_parse_fdt(struct stm32_icache_platdata *pdata)
{
	return -ENOTSUP;
}

static int stm32_icache_waitforready(void)
{
	uint32_t sr;
	int err;

	err = mmio_read32_poll_timeout(icache_pdata.base + _ICACHE_SR,
				       sr, (sr & _ICACHE_SR_BUSYENDF),
				       _ICACHE_BUSY_TIMEOUT_US);

	if (err)
		EMSG("%s: busy timeout\n", __func__);

	io_write32(icache_pdata.base + _ICACHE_FCR, _ICACHE_FCR_CBUSYENDF);

	return err;
}

/*
 * This function could be overridden by platform to define
 * pdata of serc driver
 */
__weak int stm32_icache_get_platdata(struct stm32_icache_platdata *pdata)
{
	return -ENODEV;
}

void ICACHE_IRQHandler(void)
{
}

int stm32_icache_enable_irq(void)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	NVIC_EnableIRQ(icache_pdata.irq);
}

int stm32_icache_monitor_reset(void)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	io_setbits32(icache_pdata.base + _ICACHE_CR,
		     _ICACHE_CR_MISSMRST | _ICACHE_CR_HITMRST);

	io_clrbits32(icache_pdata.base + _ICACHE_CR,
		     _ICACHE_CR_MISSMRST | _ICACHE_CR_HITMRST);

	return 0;
}

int stm32_icache_monitor_start(void)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	io_setbits32(icache_pdata.base + _ICACHE_CR,
		     _ICACHE_CR_MISSMEN | _ICACHE_CR_HITMEN);

	return 0;
}

int stm32_icache_monitor_stop(void)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	io_clrbits32(icache_pdata.base + _ICACHE_CR,
		     _ICACHE_CR_MISSMEN | _ICACHE_CR_HITMEN);

	return 0;
}

int stm32_icache_monitor_get(struct stm32_icache_mon *mon)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	mon->miss = io_read32(icache_pdata.base + _ICACHE_MMONR);
	mon->hit = io_read32(icache_pdata.base + _ICACHE_HMONR);

	return 0;
}

int stm32_icache_enable(bool monitor, bool inv)
{
	uint32_t reg;

	if (icache_pdata.base == 0)
		return -ENODEV;

	reg = io_read32(icache_pdata.base + _ICACHE_SR);
	if (reg & (_ICACHE_SR_BUSYF))
		return -EBUSY;

	if (monitor) {
		stm32_icache_monitor_reset();
		stm32_icache_monitor_start();
	}

	/*
	 * cacheinv when ICACHE_CR.EN=0 is not allow.
	 * The cache inv is done:
	 *  - on reset
	 *  - after ICACHE_CR.EN=1->0
	 *  - when cacheinv=1 with ICACHE_CR.EN=1
	 * warning: the mpu must be disabled to avoid unpredictable behavior
	 * before the busyendf
	 */
	reg = io_read32(icache_pdata.base + _ICACHE_CR);
	if (reg & (_ICACHE_CR_EN)) {
		if (!inv)
			return 0;

		return stm32_icache_full_inv();
	} else {
		/* if en=0 cache is clean (by reset or 1->0) */
		io_setbits32(icache_pdata.base + _ICACHE_CR, _ICACHE_CR_EN);
		__ISB();
	}

	return 0;
}

int stm32_icache_disable(void)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	io_clrbits32(icache_pdata.base + _ICACHE_CR, _ICACHE_CR_EN);
	__ISB();

	return stm32_icache_waitforready();
}

static int stm32_icache_valid_region(struct stm32_icache_region* region)
{
	struct icache_driver_data *ddata = icache_pdata.drv_data;

	if (region->n_region >= ddata->nb_region)
		return -ENOTSUP;

	if (region->size < ddata->range)
		return -ENOTSUP;

	if (region->size > _ICACHE_REMAP_MAX_SZ)
		return -ENOTSUP;

	if (!IS_POWER_OF_TWO(region->size))
		return -ENOTSUP;

	/*
	 * TODO
	 * check remap region + size include in
	 * exmaple stm32mp257, 3 areas:
	 *  - 0x00000000-0x07ffffff
	 *  - 0x10000000-0x17ffffff
	 *  - 0x18000000-0x1fffffff
	 */

	return 0;
}

int stm32_icache_region_enable(struct stm32_icache_region* region)
{
	uintptr_t base = icache_pdata.base;
	uint32_t ccr, offset;
	int ret;

	if (icache_pdata.base == 0)
		return -ENODEV;

	if (io_read32(base + _ICACHE_CR) & _ICACHE_CR_EN)
		return -EACCES;

	ret = stm32_icache_valid_region(region);
	if (ret)
		return ret;

	ccr = _ICACHE_FLD_PREP(_ICACHE_CRR_RSIZE, SZ_2RANGE(region->size));
	ccr |= _ICACHE_FLD_PREP(_ICACHE_CRR_BASEADDR,
				MS_ADDR(region->icache_addr, region->size));
	ccr |= _ICACHE_FLD_PREP(_ICACHE_CRR_REMAPADDR,
				MS_ADDR(region->device_addr, region->size));

	if (region->slow_c_bus)
		ccr |= _ICACHE_CRR_MSTSEL;

	offset = sizeof(uint32_t) * region->n_region;
	io_write32(base + _ICACHE_CRR0 + offset, ccr | _ICACHE_CRR_REN);

	return 0;
}

int stm32_icache_region_disable(struct stm32_icache_region* region)
{
	if (icache_pdata.base == 0)
		return -ENODEV;

	return 0;
}

int stm32_icache_full_inv(void)
{
	uintptr_t base = icache_pdata.base;
	uint32_t sr;

	if (!base)
		return -ENODEV;

	sr = io_read32(base + _ICACHE_SR);
	if (sr & _ICACHE_SR_BUSYF)
		return -EBUSY;

	io_write32(base + _ICACHE_FCR, _ICACHE_FCR_CBUSYENDF);

	io_setbits32(base + _ICACHE_CR, _ICACHE_CR_CACHEINV);

	return stm32_icache_waitforready();
}

int stm32_icache_init(void)
{
	int err = 0;

	err = stm32_icache_get_platdata(&icache_pdata);
	if (err)
		return err;

	err = stm32_icache_parse_fdt(&icache_pdata);
	if (err && err != -ENOTSUP)
		return err;

	if (!icache_pdata.drv_data)
		stm32_icache_get_driverdata(&icache_pdata);

	return 0;
}
