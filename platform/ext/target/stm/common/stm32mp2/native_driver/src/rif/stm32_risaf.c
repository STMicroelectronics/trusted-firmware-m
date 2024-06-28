/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_risaf

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <cmsis.h>
#include <device.h>
#include <clk.h>

#include <dt-bindings/rif/stm32mp25-risaf.h>

/* ID Registers */
#define _RISAF_REG_CFGR			0x40U
#define _RISAF_REG_STARTR		0x44U
#define _RISAF_REG_ENDR			0x48U
#define _RISAF_REG_CIDCFGR		0x4CU
#define _RISAF_REGX_OFFSET(x)		(0x40 * (x - 1))

#define _RISAF_HWCFGR			0xFF0U
#define _RISAF_VERR			0xFF4U

/* _RISAF_REG_CFGR(n) register fields */
#define _RISAF_REG_CFGR_BREN_SHIFT	U(0)
#define _RISAF_REG_CFGR_BREN		BIT(_RISAF_REG_CFGR_BREN_SHIFT)
#define _RISAF_REG_CFGR_SEC_SHIFT	U(8)
#define _RISAF_REG_CFGR_SEC		BIT(_RISAF_REG_CFGR_SEC_SHIFT)
#define _RISAF_REG_CFGR_ENC_SHIFT	U(15)
#define _RISAF_REG_CFGR_ENC		BIT(_RISAF_REG_CFGR_ENC_SHIFT)
#define _RISAF_REG_CFGR_PRIVC_SHIFT	U(16)
#define _RISAF_REG_CFGR_PRIVC_MASK	GENMASK_32(23, 16)
#define _RISAF_REG_CFGR_ALL_MASK	(_RISAF_REG_CFGR_BREN | \
					 _RISAF_REG_CFGR_SEC | \
					 _RISAF_REG_CFGR_ENC | \
					 _RISAF_REG_CFGR_PRIVC_MASK)

/* _RISAF_REGx_STARTR register fields */
#define _RISAF_REGx_STARTR_BADDSTART_MASK	GENMASK(31, 12)
#define _RISAF_REGx_STARTR_BADDSTART_SHIFT	12

/* _RISAF_REGx_ENDR register fields */
#define _RISAF_REGx_ENDR_BADDEND_MASK		GENMASK(31, 12)
#define _RISAF_REGx_ENDR_BADDEND_SHIFT		12

/* _RISAF_REG_CIDCFGR(n) register fields */
#define _RISAF_REG_CIDCFGR_RDENC_SHIFT	U(0)
#define _RISAF_REG_CIDCFGR_RDENC_MASK	GENMASK_32(7, 0)
#define _RISAF_REG_CIDCFGR_WRENC_SHIFT	U(16)
#define _RISAF_REG_CIDCFGR_WRENC_MASK	GENMASK_32(23, 16)
#define _RISAF_REG_CIDCFGR_ALL_MASK	(_RISAF_REG_CIDCFGR_RDENC_MASK | \
					 _RISAF_REG_CIDCFGR_WRENC_MASK)

#define _RISAF_HWCFGR_CFG1_MASK			GENMASK_32(7, 0)
#define _RISAF_HWCFGR_CFG1_SHIFT		0
#define _RISAF_HWCFGR_CFG2_MASK			GENMASK_32(15, 8)
#define _RISAF_HWCFGR_CFG2_SHIFT		8
#define _RISAF_HWCFGR_CFG3_MASK			GENMASK_32(23, 16)
#define _RISAF_HWCFGR_CFG3_SHIFT		16
#define _RISAF_HWCFGR_CFG4_MASK			GENMASK_32(31, 24)
#define _RISAF_HWCFGR_CFG4_SHIFT		24

#define _RISAF_GET_REGION_CFG(cfg)					\
	((_FLD_GET(DT_RISAF_EN, cfg) << _RISAF_REG_CFGR_BREN_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_SEC, cfg) << _RISAF_REG_CFGR_SEC_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_ENC, cfg) << _RISAF_REG_CFGR_ENC_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_PRIV, cfg) << _RISAF_REG_CFGR_PRIVC_SHIFT))

#define _RISAF_GET_REGION_CID_CFG(cfg)						\
	((_FLD_GET(DT_RISAF_WRITE, cfg) << _RISAF_REG_CIDCFGR_WRENC_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_READ, cfg) << _RISAF_REG_CIDCFGR_RDENC_SHIFT))

struct risaf_region {
	uint32_t id;
	uint32_t cfg;
	uint32_t cid_cfg;
	uint32_t start_addr;
	uint32_t end_addr;
};

struct risaf_dt_region{
	uint32_t st_protreg;
	uint32_t start_addr;
	uint32_t end_addr;
};

struct stm32_risaf_config {
	uintptr_t base;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct risaf_dt_region *dt_regions;
	const int ndt_regions;
	const bool has_enc;
};

struct stm32_risaf_data {
	uint8_t hw_nregions;
	uint8_t hw_nsubregions;
	uint8_t hw_granularity;
	uint8_t hw_naddr_bits;
};

static void stm32_risaf_write_cfg(uintptr_t base,
		const struct risaf_region *region)
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

static void stm32_risaf_read_cfg(uintptr_t base, struct risaf_region *region)
{
	region->start_addr = mmio_read_32(base + _RISAF_REG_STARTR);
	region->end_addr = mmio_read_32(base + _RISAF_REG_ENDR);
	region->cid_cfg = mmio_read_32(base + _RISAF_REG_CIDCFGR);
	region->cfg = mmio_read_32(base + _RISAF_REG_CFGR);
}

/* The copy is done in max_region */
static void stm32_risaf_tmp_copy(const struct device *dev, uint8_t id)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	struct stm32_risaf_data *drv_data = dev_get_data(dev);
	struct risaf_region tmp_region;
	uintptr_t base;

	base = drv_cfg->base + _RISAF_REGX_OFFSET(id);
	stm32_risaf_read_cfg(base, &tmp_region);
	base = drv_cfg->base + _RISAF_REGX_OFFSET(drv_data->hw_nregions);
	stm32_risaf_write_cfg(base, &tmp_region);
}

static void stm32_risaf_tmp_disable(const struct device *dev)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	struct stm32_risaf_data *drv_data = dev_get_data(dev);
	uintptr_t base;

	base = drv_cfg->base + _RISAF_REGX_OFFSET(drv_data->hw_nregions);
	mmio_write_32(base + _RISAF_REG_CFGR, 0);
	__DSB();
	__ISB();
}

static void stm32_risaf_dt_to_region(const struct device *dev,
				     uint8_t idx, struct risaf_region *region)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	const struct risaf_dt_region *dt_region = &(drv_cfg->dt_regions[idx]);

	region->id = _FLD_GET(DT_RISAF_ID, dt_region->st_protreg);
	region->cfg = _RISAF_GET_REGION_CFG(dt_region->st_protreg);
	region->cid_cfg = _RISAF_GET_REGION_CID_CFG(dt_region->st_protreg);
	region->start_addr = dt_region->start_addr;
	region->end_addr = dt_region->end_addr;
}

static int stm32_risaf_region_cfg(const struct device *dev,
				  uint8_t idx, bool update)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	struct stm32_risaf_data *drv_data = dev_get_data(dev);
	struct risaf_region region;
	uintptr_t base;
	uint32_t enabled;

	stm32_risaf_dt_to_region(dev, idx, &region);

	/*
	 * The last region is reserved like temporary region, to
	 * allow On-the-fly update. you can setup this region in last
	 */
	if (idx != (drv_cfg->ndt_regions - 1) && region.id >= drv_data->hw_nregions)
		return -EINVAL;

	if (region.cfg & _RISAF_REG_CFGR_ENC) {
		if ((!drv_cfg->has_enc) || !(region.cfg & _RISAF_REG_CFGR_SEC))
			return -EINVAL;
	}

	base = drv_cfg->base + _RISAF_REGX_OFFSET(region.id);
	enabled = mmio_read_32(base + _RISAF_REG_CFGR) &
		_RISAF_REG_CFGR_BREN ? true : false;

	/* create a temporary region before disabling and updating region */
	if (enabled)
		stm32_risaf_tmp_copy(dev, region.id);

	stm32_risaf_write_cfg(base, &region);

	if (enabled)
		stm32_risaf_tmp_disable(dev);

	return 0;
}

static void stm32_risaf_get_hwconfig(const struct device *dev)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	struct stm32_risaf_data *drv_data = dev_get_data(dev);
	uint32_t regval;

	regval = io_read32(drv_cfg->base + _RISAF_HWCFGR);

	/* hw_nregions take account the base0, which is not configurable */
	drv_data->hw_nregions = _FLD_GET(_RISAF_HWCFGR_CFG1, regval) - 1;
	drv_data->hw_nsubregions = _FLD_GET(_RISAF_HWCFGR_CFG2, regval);
	drv_data->hw_granularity = _FLD_GET(_RISAF_HWCFGR_CFG3, regval);
	drv_data->hw_naddr_bits = _FLD_GET(_RISAF_HWCFGR_CFG4, regval);
}

static int stm32_risaf_init(const struct device *dev)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	struct stm32_risaf_data *drv_data = dev_get_data(dev);
	struct clk *clk;
	int i, err;

	clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);
	if (!clk)
		return -ENODEV;

	err = clk_enable(clk);
	if (err)
		return err;

	stm32_risaf_get_hwconfig(dev);

	if (drv_cfg->ndt_regions > drv_data->hw_nregions)
		return -EINVAL;

	for(i = 0; i < drv_cfg->ndt_regions; i++) {
		err = stm32_risaf_region_cfg(dev, i, true);
		if (err)
			break;
	}

	clk_disable(clk);

	return err;
}


#define RISAF_ADDR(_n, _mem_region)							\
	COND_CODE_1(CONFIG_STM32MP25X_REVA,						\
		    (_mem_region),							\
		    (_mem_region - DT_INST_PROP_BY_IDX(_n, st_mem_map, 1)))

#define RISAF_MR_DT_REG_ADDR(_node_id, _prop, _idx, _n)					\
	RISAF_ADDR(_n, DT_REG_ADDR(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx)))

#define RISAF_MR_DT_REG_END_ADDR(_node_id, _prop, _idx, _n)				\
	(RISAF_ADDR(_n, DT_REG_ADDR(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx))) +	\
	 DT_REG_SIZE(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx)) - 1)

#define RISAF_MR_DT_PROTREG(_node_id, _prop, _idx)					\
	DT_PROP_BY_IDX(DT_PHANDLE_BY_IDX(_node_id, _prop, _idx), st_protreg, 0)

#define RISAF_PROT_FLD(_field, _node_id, _prop, _idx)					\
	((RISAF_MR_DT_PROTREG(_node_id, _prop, _idx)) & (_field))

#define RISAF_REGION(_node_id, _prop, _idx, _n)						\
	{										\
		.start_addr = RISAF_MR_DT_REG_ADDR(_node_id, _prop, _idx, _n),		\
		.end_addr = RISAF_MR_DT_REG_END_ADDR(_node_id, _prop, _idx, _n),	\
		.st_protreg = RISAF_MR_DT_PROTREG(_node_id, _prop, _idx),		\
	},

#define STM32_RISAF_X_INIT(n, variant)							\
											\
static const struct risaf_dt_region risaf_dt_regions_##variant####n[] = {		\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, memory_region, RISAF_REGION, (), n)	\
};											\
											\
static const struct stm32_risaf_config stm32_risaf_cfg_##variant####n = {		\
	.base = DT_INST_REG_ADDR(n),							\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),			\
	.dt_regions = risaf_dt_regions_##variant####n,					\
	.ndt_regions = ARRAY_SIZE(risaf_dt_regions_##variant####n),			\
	.has_enc = DT_NODE_HAS_COMPAT(DT_DRV_INST(n), st_stm32mp25_risaf_enc),		\
};											\
											\
static struct stm32_risaf_data stm32_risaf_data_##variant####n = {};			\
											\
DEVICE_DT_INST_DEFINE(n, &stm32_risaf_init,						\
		      &stm32_risaf_data_##variant####n,					\
		      &stm32_risaf_cfg_##variant####n,					\
		      CORE, 30,								\
		      NULL);

#define STM32_RISAF_ENC_INIT(n) STM32_RISAF_X_INIT(n, enc)
#define STM32_RISAF_INIT(n) STM32_RISAF_X_INIT(n, )

DT_INST_FOREACH_STATUS_OKAY(STM32_RISAF_INIT)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32mp25_risaf_enc
DT_INST_FOREACH_STATUS_OKAY(STM32_RISAF_ENC_INIT)
