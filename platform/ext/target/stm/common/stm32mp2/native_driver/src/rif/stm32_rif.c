/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <cmsis.h>
#include <clk.h>

#include <stm32_rif.h>
#include <target_cfg.h>

/* Device Tree related definitions */
#define DT_RISAF_COMPAT			"st,stm32-risaf"
#define RISAF_REG_ID_MASK		0xFU
#define RISAF_EN_SHIFT			5
#define RISAF_EN_MASK			GENMASK(1, 0)
#define RISAF_SEC_SHIFT			6
#define RISAF_SEC_MASK			GENMASK(1, 0)
#define RISAF_ENC_SHIFT			7
#define RISAF_ENC_MASK			GENMASK(1, 0)
#define RISAF_PRIVL_SHIFT		8
#define RISAF_PRIVL_MASK		GENMASK(8, 0)
#define RISAF_READL_SHIFT		16
#define RISAF_READL_MASK		GENMASK(8, 0)
#define RISAF_WRITEL_SHIFT		24
#define RISAF_WRITEL_MASK		GENMASK(8, 0)

/* ID Registers */
#define _RISAF_REG_CFGR			0x40U
#define _RISAF_REG_STARTR		0x44U
#define _RISAF_REG_ENDR			0x48U
#define _RISAF_REG_CIDCFGR		0x4CU
#define _RISAF_REGX_OFFSET(x)		(0x40 * (x - 1))

#define _RISAF_HWCFGR			0xFF0U
#define _RISAF_VERR			0xFF4U

/* _RISAF_REGx_CFGR register fields */
#define _RISAF_REGx_CFGR_BREN			BIT(0)
#define _RISAF_REGx_CFGR_BREN_SHIFT		0
#define _RISAF_REGx_CFGR_SEC			BIT(8)
#define _RISAF_REGx_CFGR_SEC_SHIFT		8
#define _RISAF_REGx_CFGR_ENC			BIT(15)
#define _RISAF_REGx_CFGR_ENC_SHIFT		15
#define _RISAF_REGx_CFGR_PRIVC_MASK		GENMASK(23, 16)
#define _RISAF_REGx_CFGR_PRIVC_SHIFT		16
#define _RISAF_REGx_CFGR_PRIVC0			BIT(16)
#define _RISAF_REGx_CFGR_PRIVC1			BIT(17)
#define _RISAF_REGx_CFGR_PRIVC2			BIT(18)
#define _RISAF_REGx_CFGR_PRIVC3			BIT(19)
#define _RISAF_REGx_CFGR_PRIVC4			BIT(20)
#define _RISAF_REGx_CFGR_PRIVC5			BIT(21)
#define _RISAF_REGx_CFGR_PRIVC6			BIT(22)
#define _RISAF_REGx_CFGR_PRIVC7			BIT(23)

/* _RISAF_REGx_STARTR register fields */
#define _RISAF_REGx_STARTR_BADDSTART_MASK	GENMASK(31, 12)
#define _RISAF_REGx_STARTR_BADDSTART_SHIFT	12

/* _RISAF_REGx_ENDR register fields */
#define _RISAF_REGx_ENDR_BADDEND_MASK		GENMASK(31, 12)
#define _RISAF_REGx_ENDR_BADDEND_SHIFT		12

/* _RISAF_REGx_CIDCFGR register fields */
#define _RISAF_REGx_CIDCFGR_RDENC_MASK		GENMASK(7, 0)
#define _RISAF_REGx_CIDCFGR_RDENC_SHIFT		0
#define _RISAF_REGx_CIDCFGR_RDENC0		BIT(0)
#define _RISAF_REGx_CIDCFGR_RDENC1		BIT(1)
#define _RISAF_REGx_CIDCFGR_RDENC2		BIT(2)
#define _RISAF_REGx_CIDCFGR_RDENC3		BIT(3)
#define _RISAF_REGx_CIDCFGR_RDENC4		BIT(4)
#define _RISAF_REGx_CIDCFGR_RDENC5		BIT(5)
#define _RISAF_REGx_CIDCFGR_RDENC6		BIT(6)
#define _RISAF_REGx_CIDCFGR_RDENC7		BIT(7)
#define _RISAF_REGx_CIDCFGR_WRENC_MASK		GENMASK(23, 16)
#define _RISAF_REGx_CIDCFGR_WRENC_SHIFT		16
#define _RISAF_REGx_CIDCFGR_WRENC0		BIT(16)
#define _RISAF_REGx_CIDCFGR_WRENC1		BIT(17)
#define _RISAF_REGx_CIDCFGR_WRENC2		BIT(18)
#define _RISAF_REGx_CIDCFGR_WRENC3		BIT(19)
#define _RISAF_REGx_CIDCFGR_WRENC4		BIT(20)
#define _RISAF_REGx_CIDCFGR_WRENC5		BIT(21)
#define _RISAF_REGx_CIDCFGR_WRENC6		BIT(22)
#define _RISAF_REGx_CIDCFGR_WRENC7		BIT(23)

#define _RIF_MAX_RISAF			5
#define _RISAF_MAX_REGION		15
#define _RISAF_MAX_REGION_PARAM		3

static struct stm32_rif_platdata pdata;

__attribute__((weak))
int stm32_rif_get_platdata(struct stm32_rif_platdata *pdata)
{
	return -ENODEV;
}

static void stm32_risaf_write_cfg(uintptr_t base,
		const struct risaf_region_cfg *region)
{
	mmio_write_32(base + _RISAF_REG_CFGR, 0);
	__DSB();
	__ISB();
	mmio_write_32(base + _RISAF_REG_STARTR, region->start_addr);
	mmio_write_32(base + _RISAF_REG_ENDR, region->end_addr);
	mmio_write_32(base + _RISAF_REG_CIDCFGR, region->cid_cfg);
	mmio_write_32(base + _RISAF_REG_CFGR, region->cfg);
	__DSB();
	__ISB();
}

static void stm32_risaf_read_cfg(uintptr_t base, struct risaf_region_cfg *region)
{
	region->start_addr = mmio_read_32(base + _RISAF_REG_STARTR);
	region->end_addr = mmio_read_32(base + _RISAF_REG_ENDR);
	region->cid_cfg = mmio_read_32(base + _RISAF_REG_CIDCFGR);
	region->cfg = mmio_read_32(base + _RISAF_REG_CFGR);
}

/* The copy is done in max_region */
static void stm32_risaf_tmp_copy(const struct risaf_cfg *risaf,
		const struct risaf_region_cfg *region)
{
	struct risaf_region_cfg tmp_region;
	uintptr_t base;

	base = risaf->base + _RISAF_REGX_OFFSET(region->id);
	stm32_risaf_read_cfg(base, &tmp_region);
	base = risaf->base + _RISAF_REGX_OFFSET(risaf->max_region);
	stm32_risaf_write_cfg(base, &tmp_region);
}

/*
 * To allow On-the-fly update of region, the region max_region
 * is reserved like temporary region.
 */
static int stm32_risaf_reg_cfg(const struct risaf_cfg *risaf,
			       const struct risaf_region_cfg *region, bool update)
{
	uintptr_t base;
	uint32_t enabled;

	if (region->id < 1 || region->id >= risaf->max_region)
		return -ENOTSUP;

	base = risaf->base + _RISAF_REGX_OFFSET(region->id);
	enabled = mmio_read_32(base + _RISAF_REG_CFGR) & _RISAF_CFGR_BREN ? true : false;

	/* create a temporary region before disabling and updating region */
	if (enabled)
		stm32_risaf_tmp_copy(risaf, region);

	stm32_risaf_write_cfg(base, region);

	if (enabled) {
		base = risaf->base + _RISAF_REGX_OFFSET(risaf->max_region);
		mmio_write_32(base + _RISAF_REG_CFGR, 0);
		__DSB();
		__ISB();
	}

	return 0;
}

static int stm32_rif_risafx_init(const struct risaf_cfg *risaf)
{
	struct clk *clk;
	int i, err = 0;
	bool clk_disabled = false;

	if (!risaf)
		return -ENOTSUP;

	clk = clk_get(STM32_DEV_RCC, (clk_subsys_t) risaf->clock_id);

	if (clk && !clk_is_enabled(clk)) {
		clk_disabled = true;
		clk_enable(clk);
	}

	for(i = 0; i < risaf->nregions; i++) {
		const struct risaf_region_cfg *region = risaf->regions + i;

		err = stm32_risaf_reg_cfg(risaf, region, true);
		if (err)
			break;
	}

	if (clk && clk_disabled)
		clk_disable(clk);

	return err;
}

static int stm32_rif_risaf_init(struct stm32_rif_platdata *pdata)
{
	int err, i;

	for(i = 0; i < pdata->nrisafs && i < _RIF_MAX_RISAF; i++) {
		const struct risaf_cfg * risaf = pdata->risafs + i;

		err = stm32_rif_risafx_init(risaf);
		if (err)
			return err;
	}

	return 0;
}

#ifdef LIBFDT
static int stm32_rif_parse_fdt(struct stm32_rif_platdata *pdata)
{
	/* parse fdt to fill platform data struct */
	return 0;
}
#else
static int stm32_rif_parse_fdt(struct stm32_rif_platdata *pdata)
{
	return -ENOTSUP;
}
#endif

int stm32_rif_init(void)
{
	int err;

	err = stm32_rif_get_platdata(&pdata);
	if (err)
		return err;

	err = stm32_rif_parse_fdt(&pdata);
	if (err && err != -ENOTSUP)
		return err;

	err = stm32_rif_risaf_init(&pdata);
	if (err)
		return err;

	return 0;
}
