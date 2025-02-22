// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#define DT_DRV_COMPAT st_stm32mp25_rifsc

#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <stm32_rif.h>
#include <stm32_rifsc.h>
#include <firewall.h>

#include <dt-bindings/rif/stm32mp25-rifsc.h>

/* RIFSC offset register */
#define _RIFSC_SECCFGR0			U(0x10)
#define _RIFSC_PRIVCFGR0		U(0x30)
#define _RIFSC_PERX_CIDCFGR		U(0x100)
#define _RIFSC_PERX_SEMCR		U(0x104)

#define _RIFSC_RIMC_ATTR0		U(0xC10)

#define _RIFSC_HWCFGR3			U(0xFE8)
#define _RIFSC_HWCFGR2			U(0xFEC)
#define _RIFSC_HWCFGR1			U(0xFF0)
#define _RIFSC_VERR			U(0xFF4)

/* RIFSC_RISC_PERX_CIDCFG register fields */
#define _RIFSC_CIDCFGR_CFEN_MASK	BIT(0)
#define _RIFSC_CIDCFGR_CFEN_SHIFT	0
#define _RIFSC_CIDCFGR_SEM_EN_MASK	BIT(1)
#define _RIFSC_CIDCFGR_SEM_EN_SHIFT	1
#define _RIFSC_CIDCFGR_SCID_MASK	GENMASK_32(6, 4)
#define _RIFSC_CIDCFGR_SCID_SHIFT	4
#define _RIFSC_CIDCFGR_SEMWLC_MASK	GENMASK_32(23, 16)
#define _RIFSC_CIDCFGR_SEMWLC_SHIFT	16

/* RIFSC_HWCFGR2 register fields */
#define _RIFSC_HWCFGR2_CFG1_MASK	GENMASK_32(15, 0)
#define _RIFSC_HWCFGR2_CFG1_SHIFT	0
#define _RIFSC_HWCFGR2_CFG2_MASK	GENMASK_32(23, 16)
#define _RIFSC_HWCFGR2_CFG2_SHIFT	16
#define _RIFSC_HWCFGR2_CFG3_MASK	GENMASK_32(31, 24)
#define _RIFSC_HWCFGR2_CFG3_SHIFT	24

/* RIFSC_HWCFGR1 register fields */
#define _RIFSC_HWCFGR1_CFG1_MASK	GENMASK_32(3, 0)
#define _RIFSC_HWCFGR1_CFG1_SHIFT	0
#define _RIFSC_HWCFGR1_CFG2_MASK	GENMASK_32(7, 4)
#define _RIFSC_HWCFGR1_CFG2_SHIFT	4
#define _RIFSC_HWCFGR1_CFG3_MASK	GENMASK_32(11, 8)
#define _RIFSC_HWCFGR1_CFG3_SHIFT	8
#define _RIFSC_HWCFGR1_CFG4_MASK	GENMASK_32(15, 12)
#define _RIFSC_HWCFGR1_CFG4_SHIFT	12
#define _RIFSC_HWCFGR1_CFG5_MASK	GENMASK_32(19, 16)
#define _RIFSC_HWCFGR1_CFG5_SHIFT	16
#define _RIFSC_HWCFGR1_CFG6_MASK	GENMASK_32(23, 20)
#define _RIFSC_HWCFGR1_CFG6_SHIFT	20

/* RIFSC_VERR register fields */
#define _RIFSC_VERR_MINREV_MASK		GENMASK_32(3, 0)
#define _RIFSC_VERR_MINREV_SHIFT	0
#define _RIFSC_VERR_MAJREV_MASK		GENMASK_32(7, 4)
#define _RIFSC_VERR_MAJREV_SHIFT	4

/* Periph id per register */
#define _PERIPH_IDS_PER_REG		32
#define _OFST_PERX_CIDCFGR		U(0x8)

/* max entries */
#define MAX_RIMU	16
#define MAX_RISUP	128

/* RIF miscellaneous */
/* Compartiment IDs */
#define RIF_CID0			0x0
#define RIF_CID1			0x1
#define RIF_CID2			0x2
#define STM32MP25_RIFSC_ENTRIES		178

struct rimu_cfg {
	uint32_t id;
	uint32_t attr;
};

struct stm32_rifsc_config {
	uintptr_t base;
	const struct rifprot_controller *risup_ctl;
	const struct rimu_cfg *rimu;
	const int nrimu;
};

struct rifsc_driver_data {
	uint8_t nb_rimu;
	uint8_t nb_risup;
	uint8_t nb_risal;
	bool rif_en;
	bool sec_en;
	bool priv_en;
};

/*
 * Must be rework
 * - a firewall framework must be created
 * - firewall = <&rifsc id>
 */
int stm32_rifsc_get_access_by_id(const struct device *dev, uint32_t id)
{
	const struct stm32_rifsc_config *dev_cfg = dev_get_config(dev);
	uintptr_t x_offset = _RIFSC_PERX_CIDCFGR + _OFST_PERX_CIDCFGR * id;
	unsigned int master = RIF_CID2;
	uint32_t cid_cfgr = 0;

	if (id >= STM32MP25_RIFSC_ENTRIES)
		return -1;

	cid_cfgr = io_read32(dev_cfg->base + x_offset);

	/*
	 * First check conditions for semaphore mode, which doesn't
	 * take into account static CID.
	 */
	if (cid_cfgr & _RIFSC_CIDCFGR_SEM_EN_MASK) {
		if (cid_cfgr & BIT(master + _RIFSC_CIDCFGR_SEMWLC_SHIFT)) {
			/* Static CID is irrelevant if semaphore mode */
			return 0;
		} else {
			return -1;
		}
	}

	if (!(_FLD_GET(_RIFSC_CIDCFGR_CFEN, cid_cfgr)) ||
	    _FLD_GET(_RIFSC_CIDCFGR_SCID, cid_cfgr) == RIF_CID0)
		return 0;

	/* Coherency check with the CID configuration */
	if (_FLD_GET(_RIFSC_CIDCFGR_SCID, cid_cfgr) != master)
		return -1;

	return 0;
}

/*
 * Must be rework
 * - a firewall framework must be created
 */

static int stm32_rimu_cfg(const struct device *dev, const struct rimu_cfg *rimu)
{
	const struct stm32_rifsc_config *dev_cfg = dev_get_config(dev);
	struct rifsc_driver_data *drv_data = dev_get_data(dev);
	uintptr_t offset =  _RIFSC_RIMC_ATTR0 + (sizeof(uint32_t) * rimu->id);

	if (!rimu || rimu->id >= drv_data->nb_rimu)
		return -EINVAL;

	if (drv_data->rif_en)
		io_write32(dev_cfg->base + offset, rimu->attr);

	return 0;
}

static int stm32_rimu_setup(const struct device *dev)
{
	const struct stm32_rifsc_config *dev_cfg = dev_get_config(dev);
	struct rifsc_driver_data *drv_data = dev_get_data(dev);
	int i = 0;
	int err = 0;

	for (i = 0; i < dev_cfg->nrimu && i < drv_data->nb_rimu; i++) {
		const struct rimu_cfg *rimu = dev_cfg->rimu + i;

		err = stm32_rimu_cfg(dev, rimu);
		if (err) {
			EMSG("rimu cfg(%d/%d) error", i + 1, dev_cfg->nrimu);
			return err;
		}
	}

	return 0;
}

static void stm32_rifsc_set_drvdata(const struct device *dev)
{
	const struct stm32_rifsc_config *rifsc_cfg = dev_get_config(dev);
	struct rifsc_driver_data *rifsc_drvdata = dev_get_data(dev);
	uint32_t regval = 0;

	regval = io_read32(rifsc_cfg->base + _RIFSC_HWCFGR1);
	rifsc_drvdata->rif_en = _FLD_GET(_RIFSC_HWCFGR1_CFG1, regval) != 0;
	rifsc_drvdata->sec_en = _FLD_GET(_RIFSC_HWCFGR1_CFG2, regval) != 0;
	rifsc_drvdata->priv_en = _FLD_GET(_RIFSC_HWCFGR1_CFG3, regval) != 0;

	regval = io_read32(rifsc_cfg->base + _RIFSC_HWCFGR2);
	rifsc_drvdata->nb_risup = _FLD_GET(_RIFSC_HWCFGR2_CFG1, regval);
	rifsc_drvdata->nb_rimu = _FLD_GET(_RIFSC_HWCFGR2_CFG2, regval);
	rifsc_drvdata->nb_risal = _FLD_GET(_RIFSC_HWCFGR2_CFG3, regval);

	regval = io_read8(rifsc_cfg->base + _RIFSC_VERR);

	DMSG("RIFSC version %"PRIu32".%"PRIu32,
	     _FLD_GET(_RIFSC_VERR_MAJREV, regval),
	     _FLD_GET(_RIFSC_VERR_MINREV, regval));

	DMSG("HW cap: enabled[rif:sec:priv]:[%s:%s:%s] nb[risup|rimu|risal]:[%"PRIu8",%"PRIu8",%"PRIu8"]",
	     rifsc_drvdata->rif_en ? "true" : "false",
	     rifsc_drvdata->sec_en ? "true" : "false",
	     rifsc_drvdata->priv_en ? "true" : "false",
	     rifsc_drvdata->nb_risup,
	     rifsc_drvdata->nb_rimu,
	     rifsc_drvdata->nb_risal);
}

static int stm32_rifsc_firewall_set_conf(const struct firewall_spec *spec)
{
	const struct stm32_rifsc_config *rifsc_cfg = dev_get_config(spec->dev);
	struct rifprot_config rifprot_cfg = RIFPROT_CFG(spec->args[0]);

	return stm32_rifprot_set_conf(rifsc_cfg->risup_ctl, &rifprot_cfg);
}

static const struct firewall_controller_api stm32_rifsc_firewall_api = {
	.set_conf = stm32_rifsc_firewall_set_conf,
};

static int stm32_rifsc_init(const struct device *dev)
{
	const struct stm32_rifsc_config *rifsc_cfg = dev_get_config(dev);
	struct rifsc_driver_data *rifsc_data = dev_get_data(dev);
	int err;

	if (!rifsc_cfg || !rifsc_data)
		return -ENODEV;

	stm32_rifsc_set_drvdata(dev);

	if (rifsc_cfg->risup_ctl->nrifprot >= rifsc_data->nb_risup)
		return -EINVAL;

	err = stm32_rifprot_init(rifsc_cfg->risup_ctl);
	if (err)
		return err;

	return stm32_rimu_setup(dev);
}

#define STM32_RIMU(_node_id, _prop, _idx)					\
	{									\
		.id = RIFPROT_FLD(RIFSC_RIMC_M_ID, _node_id, _prop, _idx),	\
		.attr = RIFPROT_FLD(RIFSC_RIMC_ATTRx, _node_id, _prop, _idx),	\
	}

#define STM32_RIFSC_INIT(n)								\
											\
static const struct rimu_cfg rimu_config_##n[] = {					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, st_rimu),					\
		    (DT_INST_FOREACH_PROP_ELEM_SEP(n, st_rimu, STM32_RIMU, (,))),	\
		    ())									\
};											\
											\
static __unused const struct rif_base rbase_##n = {					\
	.sec = DT_INST_REG_ADDR(n) + _RIFSC_SECCFGR0,					\
	.priv = DT_INST_REG_ADDR(n) + _RIFSC_PRIVCFGR0,					\
	.cid = DT_INST_REG_ADDR(n) + _RIFSC_PERX_CIDCFGR,				\
	.sem = DT_INST_REG_ADDR(n) + _RIFSC_PERX_SEMCR,					\
};											\
											\
DT_INST_RIFPROT_CTRL_DEFINE(n, &rbase_##n, NULL, MAX_RISUP);				\
											\
static const struct stm32_rifsc_config stm32_rifsc_cfg_##n = {				\
	.base = DT_INST_REG_ADDR(n),							\
	.rimu = rimu_config_##n,							\
	.nrimu = ARRAY_SIZE(rimu_config_##n),						\
	.risup_ctl = DT_INST_RIFPROT_CTRL_GET(n),					\
};											\
											\
static struct rifsc_driver_data stm32_rifsc_data_##n = {};				\
											\
DEVICE_DT_INST_DEFINE(n, &stm32_rifsc_init,						\
		      &stm32_rifsc_data_##n, &stm32_rifsc_cfg_##n,			\
		      PRE_CORE, 0,							\
		      &stm32_rifsc_firewall_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_RIFSC_INIT)
