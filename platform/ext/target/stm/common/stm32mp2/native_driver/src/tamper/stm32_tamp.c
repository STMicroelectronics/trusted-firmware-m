// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <stm32_rif.h>
#include <clk.h>

/* TAMP offset register */
#define _TAMP_SECCFGR			U(0x20)
#define _TAMP_PRIVCFGR			U(0x24)
#define _TAMP_BKPRIFR1			U(0x70)
#define _TAMP_BKPRIFR2			U(0x74)
#define _TAMP_BKPRIFR3			U(0x78)
#define _TAMP_RxCIDCFGR(x)		(U(0x80) + (0x4 * (x)))
#define _TAMP_BKPxR(x)			(U(0x100) + (0x4 * (x)))
#define _TAMP_HWCFGR1			U(0x3F0)

/* _TAMP_SECCFGR bit fields */
#define _SECCFGR_BKPRWSEC_MASK		GENMASK_32(7, 0)
#define _SECCFGR_BKPRWSEC_SHIFT		0
#define _SECCFGR_CNT2SEC_MASK		BIT(14)
#define _SECCFGR_CNT2SEC_SHIFT		14
#define _SECCFGR_CNT1SEC_MASK		BIT(15)
#define _SECCFGR_CNT1SEC_SHIFT		15
#define _SECCFGR_BKPWSEC_MASK		GENMASK_32(23, 16)
#define _SECCFGR_BKPWSEC_SHIFT		16
#define _SECCFGR_BHKLOCK_MASK		BIT(30)
#define _SECCFGR_BHKLOCK_SHIFT		30
#define _SECCFGR_TAMPSEC_MASK		BIT(31)
#define _SECCFGR_TAMPSEC_SHIFT		31

/* _TAMP_PRIVCFGR bit fields */
#define _PRIVCFG_CNT2PRIV_MASK		BIT(14)
#define _PRIVCFG_CNT2PRIV_SHIFT		14
#define _PRIVCFG_CNT1PRIV_MASK		BIT(15)
#define _PRIVCFG_CNT1PRIV_SHIFT		15
#define _PRIVCFG_BKPRWPRIV_MASK		BIT(29)
#define _PRIVCFG_BKPRWPRIV_SHIFT	29
#define _PRIVCFG_BKPWPRIV_MASK		BIT(30)
#define _PRIVCFG_BKPWPRIV_SHIFT		30
#define _PRIVCFG_TAMPPRIV_MASK		BIT(31)
#define _PRIVCFG_TAMPPRIV_SHIFT		31

/* _TAMP_BKPRIFR3 bit fields */
#define _BKPRIFR3_BKPWRIF1_MASK		GENMASK_32(7, 0)
#define _BKPRIFR3_BKPWRIF1_SHIFT	0
#define _BKPRIFR3_BKPWRIF2_MASK		GENMASK_32(23, 16)
#define _BKPRIFR3_BKPWRIF2_SHIFT	16

/* CIDCFGR register bitfields */
#define _CIDCFGR_CFEN_MASK		BIT(0)
#define _CIDCFGR_CFEN_SHIFT		0
#define _CIDCFGR_SCID_MASK		GENMASK_32(6, 4)
#define _CIDCFGR_SCID_SHIFT		4

/* _TAMP_HWCFGR1 register bitfields */
#define _HWCFGR1_BKPREG_MASK		GENMASK_32(7, 0)
#define _HWCFGR1_BKPREG_SHIFT		0
#define _HWCFGR1_TAMPER_MASK		GENMASK_32(11, 8)
#define _HWCFGR1_TAMPER_SHIFT		8
#define _HWCFGR1_ACTIVE_MASK		GENMASK_32(15, 12)
#define _HWCFGR1_ACTIVE_SHIFT		12
#define _HWCFGR1_INTERN_MASK		GENMASK_32(31, 16)
#define _HWCFGR1_INTERN_SHIFT		16

/* RIF miscellaneous */
#define _CID_X_OFFSET(_id)		(U(0x4) * (_id))
#define TAMP_RIF_RES			U(3)
#define MAX_DT_BKP_ZONES		7
#define BKP_ZONE_LEN			0
#define MP25_BKP_ZONE_LEN		7

struct stm32_tamp_variant {
	int (*pre_init_fn)(const struct device *dev);
	int (*init_bkpr_fn)(const struct device *dev);
};

struct stm32_tamp_config {
	uintptr_t base;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct rifprot_controller *rif_ctl;
	const uint32_t bkp_zones[MAX_DT_BKP_ZONES];
};

struct stm32_tamp_data {
	const struct stm32_tamp_variant *variant;
	uint8_t hw_nb_bkp_reg;
};

/*
 * specific function to setup a resource:
 *  - no standard id
 */
static __unused
int stm32_tamp_rif_set_conf(const struct rifprot_controller *ctl,
			    struct rifprot_config *cfg)
{
	uint32_t seccfgr, privcfgr, sec_mask, priv_mask;

	if (!IS_ENABLED(STM32_M33TDCID))
		return 0;

	/* disable filtering befor write sec and priv cfgr */
	io_clrbits32(ctl->rbase->cid + _CID_X_OFFSET(cfg->id), _CIDCFGR_CFEN_MASK);

	switch (cfg->id) {
	case 0:
		sec_mask = _SECCFGR_TAMPSEC_MASK;
		seccfgr = _FLD_PREP(_SECCFGR_TAMPSEC, cfg->sec);
		priv_mask = _PRIVCFG_TAMPPRIV_MASK;
		privcfgr = _FLD_PREP(_PRIVCFG_TAMPPRIV, cfg->priv);
		break;
	case 1:
		sec_mask = _SECCFGR_CNT1SEC_MASK;
		seccfgr = _FLD_PREP(_SECCFGR_CNT1SEC, cfg->sec);
		priv_mask = _PRIVCFG_CNT1PRIV_MASK | _PRIVCFG_BKPRWPRIV_MASK;
		privcfgr = _FLD_PREP(_PRIVCFG_CNT1PRIV, cfg->priv) |
			_PRIVCFG_BKPRWPRIV_MASK;
		break;
	case 2:
		sec_mask = _SECCFGR_CNT2SEC_MASK;
		seccfgr = _FLD_PREP(_SECCFGR_CNT2SEC, cfg->sec);
		priv_mask = _PRIVCFG_CNT2PRIV_MASK | _PRIVCFG_BKPWPRIV_MASK;
		privcfgr = _FLD_PREP(_PRIVCFG_CNT2PRIV, cfg->priv) |
			_PRIVCFG_BKPWPRIV_MASK;
		break;

	default:
		return -EINVAL;
	}

	io_clrsetbits32(ctl->rbase->sec, sec_mask, seccfgr);
	io_clrsetbits32(ctl->rbase->priv, priv_mask, privcfgr);

	io_write32(ctl->rbase->cid + _CID_X_OFFSET(cfg->id), cfg->cid_attr);

	return 0;
}

static __unused int _stm32mp25_tamp_init_bkpr(const struct device *dev)
{
	const struct stm32_tamp_config *cfg = dev_get_config(dev);
	struct stm32_tamp_data *dev_data = dev_get_data(dev);
	uint32_t bkp_zone1, bkp_zone2;
	uint32_t bkp_zone1_rif1, bkp_zone2_rif1, bkp_zone3_rif1, bkp_zone3_rif0;
	uint8_t bkp_limit;

	bkp_zone1_rif1 = cfg->bkp_zones[0];
	bkp_zone1 = bkp_zone1_rif1 + cfg->bkp_zones[1];
	bkp_zone2_rif1 = bkp_zone1 + cfg->bkp_zones[2];
	bkp_zone2 = bkp_zone2_rif1 + cfg->bkp_zones[3];
	bkp_zone3_rif1 = bkp_zone2 + cfg->bkp_zones[4];
	bkp_zone3_rif0 = bkp_zone3_rif1 + cfg->bkp_zones[5];
	bkp_limit = bkp_zone3_rif0 + cfg->bkp_zones[6];

	if (bkp_limit > dev_data->hw_nb_bkp_reg)
		return -EINVAL;

	io_clrsetbits32(cfg->base + _TAMP_SECCFGR,
			_SECCFGR_BKPRWSEC_MASK | _SECCFGR_BKPWSEC_MASK,
			_FLD_PREP(_SECCFGR_BKPRWSEC, bkp_zone1) |
			_FLD_PREP(_SECCFGR_BKPWSEC, bkp_zone2));

	io_write32(cfg->base + _TAMP_BKPRIFR1, bkp_zone1_rif1);
	io_write32(cfg->base + _TAMP_BKPRIFR2, bkp_zone2_rif1);
	io_write32(cfg->base + _TAMP_BKPRIFR3, bkp_zone3_rif1 |
		   _FLD_PREP(_BKPRIFR3_BKPWRIF2, bkp_zone3_rif0));

	return 0;
}

/*
 * Errata: This errata avoid a corteA stuck after reset.
 * When restarting after M33 TDCID, the ROM code read the bkpr11 and if it's valide
 * jump in stop2 mode.
 * To avoid this, M33 bl2 must clear the bkpr11 register before wakeup the cortexA.
 *
 * Warning:
 * The three TAMP_R0CIDCFGR.CFEN, TAMP_R1CIDCFGR.CFEN, TAMP_R2CIDCFGR.CFEN bits could
 * be set after initialization sequence to protect backup registers with CID filtering.
 * It is not allowed to configure some of the bits to 1 and some others to 0
 * (in this case the protection would behave as if all these bits were 1).
 */
#define CLEAR_PATTERN 0x55

static __unused int _stm32mp25_bkpr11_errata(const struct device *dev)
{
	const struct stm32_tamp_config *cfg = dev_get_config(dev);
	uint32_t r0cid_cfgr, r1cid_cfgr, r2cid_cfgr, bkpr11;
	bool protected;

	r0cid_cfgr = io_read32(cfg->base + _TAMP_RxCIDCFGR(0));
	r1cid_cfgr = io_read32(cfg->base + _TAMP_RxCIDCFGR(1));
	r2cid_cfgr = io_read32(cfg->base + _TAMP_RxCIDCFGR(2));
	protected = !!((r0cid_cfgr | r1cid_cfgr | r2cid_cfgr) & _CIDCFGR_CFEN_MASK);

	if (protected) {
		io_write32(cfg->base + _TAMP_RxCIDCFGR(0), r0cid_cfgr & ~_CIDCFGR_CFEN_MASK);
		io_write32(cfg->base + _TAMP_RxCIDCFGR(1), r1cid_cfgr & ~_CIDCFGR_CFEN_MASK);
		io_write32(cfg->base + _TAMP_RxCIDCFGR(2), r2cid_cfgr & ~_CIDCFGR_CFEN_MASK);
	}

	io_write32(cfg->base + _TAMP_BKPxR(11), CLEAR_PATTERN);
	bkpr11 = io_read32(cfg->base + _TAMP_BKPxR(11));

	if (bkpr11 != CLEAR_PATTERN)
		EMSG("tamp: errata: issue not fixed");

	if (protected) {
		io_write32(cfg->base + _TAMP_RxCIDCFGR(0), r0cid_cfgr);
		io_write32(cfg->base + _TAMP_RxCIDCFGR(1), r1cid_cfgr);
		io_write32(cfg->base + _TAMP_RxCIDCFGR(2), r2cid_cfgr);
	}

	return 0;
}

static void stm32_tamp_get_hwconfig(const struct device *dev)
{
	const struct stm32_tamp_config *dev_cfg = dev_get_config(dev);
	struct stm32_tamp_data *dev_data = dev_get_data(dev);
	uint32_t regval;

	regval = io_read32(dev_cfg->base + _TAMP_HWCFGR1);
	dev_data->hw_nb_bkp_reg = _FLD_GET(_HWCFGR1_BKPREG, regval);
}

static __unused int stm32_tamp_init(const struct device *dev)
{
	const struct stm32_tamp_config *cfg = dev_get_config(dev);
	struct stm32_tamp_data *data = dev_get_data(dev);
	struct clk *clk;
	int err;

	if (!cfg)
		return -ENODEV;

	clk = clk_get(cfg->clk_dev, cfg->clk_subsys);
	if (!clk)
		return -ENODEV;

	err = clk_enable(clk);
	if (err)
		return err;

	stm32_tamp_get_hwconfig(dev);

	if (data->variant->pre_init_fn) {
		err = data->variant->pre_init_fn(dev);
		if (err)
			goto out;
	}

	if (data->variant->init_bkpr_fn) {
		err = data->variant->init_bkpr_fn(dev);
		if (err)
			goto out;
	}

	if (cfg->rif_ctl)
		err = stm32_rifprot_init(cfg->rif_ctl);

out:
	clk_disable(clk);

	return err;
}

static __unused const struct  stm32_tamp_variant stm32_variant = {};

static __unused const struct stm32_tamp_variant stm32mp25_variant = {
#if defined(STM32_BL2)
	.pre_init_fn = &_stm32mp25_bkpr11_errata,
#endif
	.init_bkpr_fn = &_stm32mp25_tamp_init_bkpr,

};

#define STM32_TAMP_INIT(n, name, _variant, bkp_zone_len)				\
											\
BUILD_ASSERT(DT_INST_PROP_LEN_OR(n, st_backup_zones, 0) == bkp_zone_len,		\
	    "Incorrect bkp zone property");						\
											\
static __unused const struct rif_base _##name##_rbase##n = {				\
	.sec = DT_INST_REG_ADDR(n) + _TAMP_SECCFGR,					\
	.priv = DT_INST_REG_ADDR(n) + _TAMP_PRIVCFGR,					\
	.cid = DT_INST_REG_ADDR(n) + _TAMP_RxCIDCFGR(0),				\
	.sem = 0,									\
};											\
											\
struct __unused rif_ops _##name##_rops##n = {						\
	.set_conf = stm32_tamp_rif_set_conf,						\
};											\
											\
DT_INST_RIFPROT_CTRL_DEFINE(n, &_##name##_rbase##n,					\
			    &_##name##_rops##n, TAMP_RIF_RES);				\
											\
static const struct stm32_tamp_config _##name##_cfg##n = {				\
	.base = DT_INST_REG_ADDR(n),							\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),			\
	.rif_ctl = DT_INST_RIFPROT_CTRL_GET(n),						\
	.bkp_zones = DT_INST_PROP_OR(n, st_backup_zones, {}),				\
};											\
											\
static struct stm32_tamp_data _##name##_data##n = {					\
	.variant = &_variant,								\
};											\
											\
DEVICE_DT_INST_DEFINE(n, &stm32_tamp_init,						\
		      &_##name##_data##n, &_##name##_cfg##n,				\
		      CORE, 10, NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32_tamp

DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_TAMP_INIT, DT_DRV_COMPAT,
				  stm32_variant, BKP_ZONE_LEN)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp25_tamp

DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_TAMP_INIT, DT_DRV_COMPAT,
				  stm32mp25_variant, MP25_BKP_ZONE_LEN)
