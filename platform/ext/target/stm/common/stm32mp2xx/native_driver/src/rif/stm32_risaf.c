/*
 * Copyright (c) 2023-2025, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_risaf

#include <device.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <debug.h>

#include "lib/delay.h"
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include "lib/timeout.h"
#include <lib/utils_def.h>
#include <cmsis.h>
#include <clk.h>

#include <entropy.h>
#include <string.h>
#include <strings.h>

#include <dt-bindings/rif/stm32mp2-risaf.h>

/* ID Registers */
#define _RISAF_SR			0x04U
#define _RISAF_KEYR			0x30U
#define _RISAF_REG_CFGR			0x40U
#define _RISAF_REG_STARTR		0x44U
#define _RISAF_REG_ENDR			0x48U
#define _RISAF_REG_CIDCFGR		0x4CU
#define _RISAF_REGX_OFFSET(x)		(0x40 * (x - 1))

/* RISAF MCE extension registers */
#define _RISAF_XCR			U(0x1C00)
#define _RISAF_XSR			U(0x1C04)
#define _RISAF_MKEYR			U(0x1E00)

#define _RISAF_HWCFGR			0xFF0U
#define _RISAF_VERR			0xFF4U

/* _RISAF_SR register fields */
#define _RISAF_SR_KEYVALID_SHIFT	U(0)
#define _RISAF_SR_KEYVALID		BIT(_RISAF_SR_KEYVALID_SHIFT)
#define _RISAF_SR_KEYRDY_SHIFT		U(1)
#define _RISAF_SR_KEYRDY		BIT(_RISAF_SR_KEYRDY_SHIFT)
#define _RISAF_SR_ENCDIS_SHIFT		U(2)
#define _RISAF_SR_ENCDIS		BIT(_RISAF_SR_ENCDIS_SHIFT)

/* _RISAF_REG_CFGR(n) register fields */
#define _RISAF_REG_CFGR_BREN_SHIFT	U(0)
#define _RISAF_REG_CFGR_BREN		BIT(_RISAF_REG_CFGR_BREN_SHIFT)
#define _RISAF_REG_CFGR_SEC_SHIFT	U(8)
#define _RISAF_REG_CFGR_SEC		BIT(_RISAF_REG_CFGR_SEC_SHIFT)
#ifdef STM32MP21xxxx
#define _RISAF_REG_CFGR_ENC_SHIFT	U(14)
#define _RISAF_REG_CFGR_ENC		GENMASK_32(15, 14)
#else /* STM32MP21xxxx */
#define _RISAF_REG_CFGR_ENC_SHIFT	U(15)
#define _RISAF_REG_CFGR_ENC		BIT(_RISAF_REG_CFGR_ENC_SHIFT)
#endif /* STM32MP21xxxx */
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

#define _RISAF_HWCFGR_CFG1_MASK		GENMASK_32(7, 0)
#define _RISAF_HWCFGR_CFG1_SHIFT	0
#define _RISAF_HWCFGR_CFG2_MASK		GENMASK_32(15, 8)
#define _RISAF_HWCFGR_CFG2_SHIFT	8
#define _RISAF_HWCFGR_CFG3_MASK		GENMASK_32(23, 16)
#define _RISAF_HWCFGR_CFG3_SHIFT	16
#define _RISAF_HWCFGR_CFG4_MASK		GENMASK_32(31, 24)
#define _RISAF_HWCFGR_CFG4_SHIFT	24

/* RISAF MCE extension register field description */
/* _RISAF_XCR register fields */
#define _RISAF_XCR_XLOCK		BIT(0)
#define _RISAF_XCR_MKLOCK		BIT(1)
#define _RISAF_XCR_CIPHERSEL_SHIFT	4
#define _RISAF_XCR_CIPHERSEL_MASK	GENMASK_32(5, 4)
#define _RISAF_XCR_CIPHERSEL_AES128	1U
#define _RISAF_XCR_CIPHERSEL_AES256	3U
/* _RISAF_XSR register fields */
#define _RISAF_XSR_MKVALID		BIT(0)

#define _RISAF_GET_REGION_CFG(cfg)					\
	((_FLD_GET(DT_RISAF_EN, cfg) << _RISAF_REG_CFGR_BREN_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_SEC, cfg) << _RISAF_REG_CFGR_SEC_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_ENC, cfg) << _RISAF_REG_CFGR_ENC_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_PRIV, cfg) << _RISAF_REG_CFGR_PRIVC_SHIFT))

#define _RISAF_GET_REGION_CID_CFG(cfg)						\
	((_FLD_GET(DT_RISAF_WRITE, cfg) << _RISAF_REG_CIDCFGR_WRENC_SHIFT) |	\
	 (_FLD_GET(DT_RISAF_READ, cfg) << _RISAF_REG_CIDCFGR_RDENC_SHIFT))

#define _RISAF_TIMEOUT_1MS_IN_US	USEC_PER_MSEC
#define _RISAF_TIMEOUT_100MS_IN_US	USEC_PER_MSEC * 100U
#define _RISAF_TIMEOUT_STEP_10US	10U
#define _RISAF_TIMEOUT_STEP_100US	100U

#define BITS_PER_BYTES	8

enum risaf_key_size {
	RISAF_NO_KEY = 0,
	RISAF_KEY_128BITS = 128,
	RISAF_KEY_256BITS = 256,
	RISAF_MAX_KEY_SZ = 256,
};

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

struct stm32_risaf_variant {
	bool has_enc;
	int (*default_encryption_fn)(const struct device *dev, uint8_t *key);
	int (*mce_encryption_fn)(const struct device *dev, uint8_t *key);
	uint32_t max_key_sz;
};

struct stm32_risaf_config {
	uintptr_t base;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct risaf_dt_region *dt_regions;
	const int ndt_regions;
	const struct device *entropy_dev;
	const uint32_t st_mce_keysize;
};

struct stm32_risaf_data {
	const struct stm32_risaf_variant *variant;
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
		if ((!drv_data->variant->has_enc) || !(region.cfg & _RISAF_REG_CFGR_SEC))
			return -EINVAL;

		if (!(mmio_read_32(drv_cfg->base + _RISAF_SR) & _RISAF_SR_KEYVALID) ||
		    !(mmio_read_32(drv_cfg->base + _RISAF_SR) & _RISAF_SR_KEYRDY))
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

static bool stm32_risaf_region_need_encryption_key(const struct device *dev)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	uint32_t status = mmio_read_32(drv_cfg->base + _RISAF_SR);

	if ((status & _RISAF_SR_KEYVALID) && (status & _RISAF_SR_KEYRDY) &&
	   !(status & _RISAF_SR_ENCDIS))
		return false;

	return true;
}

static __unused int stm32_risaf_install_encryption_key(const struct device *dev, uint8_t *key)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	uint32_t key_size = RISAF_KEY_128BITS;
	uint64_t sr;
	uint32_t i;
	int err;

	for (i = 0U; i < key_size / BITS_PER_BYTES; i += sizeof(uint32_t)) {
		uint32_t key_val = 0U;

		memcpy(&key_val, key + i, sizeof(uint32_t));
		mmio_write_32(drv_cfg->base + _RISAF_KEYR + i, key_val);
	}

	err = mmio_read32_poll_timeout(drv_cfg->base + _RISAF_SR,
				       sr,
				       (sr & (_RISAF_SR_KEYVALID | _RISAF_SR_KEYRDY)),
				       _RISAF_TIMEOUT_1MS_IN_US);
	if (err)
		EMSG("[%s] Timeout waiting encryption key expension\n", dev->name);

	return err;
}

static __unused int stm32_risaf_install_mce_encryption_key(const struct device *dev, uint8_t *mkey)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	uint32_t key_size = drv_cfg->st_mce_keysize;
	uint64_t xsr;
	uint32_t i;
	int err;

	if (key_size == RISAF_KEY_128BITS)
		mmio_write_32(drv_cfg->base + _RISAF_XCR,
			      _RISAF_XCR_CIPHERSEL_AES128 << _RISAF_XCR_CIPHERSEL_SHIFT);
	else if (key_size == RISAF_KEY_256BITS)
		mmio_write_32(drv_cfg->base + _RISAF_XCR,
			      _RISAF_XCR_CIPHERSEL_AES256 << _RISAF_XCR_CIPHERSEL_SHIFT);
	else
		return -EINVAL;

	for (i = 0U; i < key_size / BITS_PER_BYTES; i += sizeof(uint32_t)) {
		uint32_t key_val = 0U;

		memcpy(&key_val, mkey + i, sizeof(uint32_t));
                mmio_write_32(drv_cfg->base + _RISAF_MKEYR + i, key_val);
        }

        err = mmio_read32_poll_timeout(drv_cfg->base + _RISAF_XSR,
				       xsr,
				       (xsr & _RISAF_XSR_MKVALID),
				       _RISAF_TIMEOUT_100MS_IN_US);

	if (err)
		EMSG("[%s] Timeout waiting mce encryption key expension\n", dev->name);

	return err;
}

static int stm32_risaf_encryption_init(const struct device *dev)
{
	const struct stm32_risaf_config *drv_cfg = dev_get_config(dev);
	struct stm32_risaf_data *drv_data = dev_get_data(dev);
	const struct stm32_risaf_variant *variant = drv_data->variant;
	uint8_t key[RISAF_MAX_KEY_SZ / BITS_PER_BYTES];
	int err;

	if (!variant->default_encryption_fn && !variant->mce_encryption_fn)
		return -EINVAL;

	bzero(key, RISAF_MAX_KEY_SZ / BITS_PER_BYTES);

	err = entropy_get_entropy(drv_cfg->entropy_dev, key, variant->max_key_sz / BITS_PER_BYTES);
	if (err) {
		EMSG("[%s] Could not get specific entropy\n", dev->name);
		goto out;
	}

	if (variant->default_encryption_fn) {
		err = variant->default_encryption_fn(dev, key);
		if (err)
			goto out;
	}

	if (variant->mce_encryption_fn)
		err = variant->mce_encryption_fn(dev, key);

out:
	return err;
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

	if (drv_data->variant->has_enc && stm32_risaf_region_need_encryption_key(dev)) {
		if (!drv_cfg->entropy_dev)
			return -EINVAL;

		err = stm32_risaf_encryption_init(dev);
		if (err)
			return err;
	}

	for(i = 0; i < drv_cfg->ndt_regions; i++) {
		err = stm32_risaf_region_cfg(dev, i, true);
		if (err)
			break;
	}

	clk_disable(clk);

	return err;
}

static __unused const struct stm32_risaf_variant stm32mp25_variant = {
	.has_enc = false,
	.default_encryption_fn = NULL,
	.mce_encryption_fn = NULL,
	.max_key_sz = RISAF_NO_KEY,
};

static __unused const struct stm32_risaf_variant stm32mp25_enc_variant = {
	.has_enc = true,
	.default_encryption_fn = &stm32_risaf_install_encryption_key,
	.mce_encryption_fn = NULL,
	.max_key_sz = RISAF_KEY_128BITS,
};

static __unused const struct stm32_risaf_variant stm32mp21_enc_variant = {
	.has_enc = true,
	.default_encryption_fn = &stm32_risaf_install_encryption_key,
	.mce_encryption_fn = &stm32_risaf_install_mce_encryption_key,
	.max_key_sz = RISAF_KEY_256BITS,
};

#define RISAF_ADDR(_n, _mem_region)							\
	(_mem_region - DT_INST_PROP_BY_IDX(_n, st_mem_map, 1))

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

#define STM32_RISAF_INIT(n, name, _variant)						\
											\
static const struct risaf_dt_region risaf_dt_regions_##name####n[] = {			\
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(n, memory_region, RISAF_REGION, (), n)	\
};											\
											\
static const struct stm32_risaf_config stm32_risaf_cfg_##name####n = {			\
	.base = DT_INST_REG_ADDR(n),							\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),			\
	.dt_regions = risaf_dt_regions_##name####n,					\
	.ndt_regions = ARRAY_SIZE(risaf_dt_regions_##name####n),			\
	.entropy_dev = DEVICE_DT_GET_OR_NULL(DT_INST_ENTROPY_CTLR(n)),			\
	.st_mce_keysize = DT_INST_PROP_OR(n, st_mce_keysize_bits,			\
					  RISAF_KEY_128BITS)				\
};											\
											\
static struct stm32_risaf_data stm32_risaf_data_##name####n = {				\
	.variant = &_variant,								\
};											\
											\
DEVICE_DT_INST_DEFINE(n, &stm32_risaf_init,						\
		      &stm32_risaf_data_##name####n,					\
		      &stm32_risaf_cfg_##name####n,					\
		      CORE, 30,								\
		      NULL);

DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_RISAF_INIT, DT_DRV_COMPAT, stm32mp25_variant)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32mp25_risaf_enc
DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_RISAF_INIT, DT_DRV_COMPAT, stm32mp25_enc_variant)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT st_stm32mp21_risaf_enc
DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_RISAF_INIT, DT_DRV_COMPAT, stm32mp21_enc_variant)

