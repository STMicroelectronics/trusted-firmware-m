// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#define DT_DRV_COMPAT st_stm32mp1_exti

#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <stm32_rif.h>

/* EXTI offset register */
#define _EXTI_SECCFGR		U(0x014)
#define _EXTI_PRIVCFGR		U(0x018)
#define _EXTI_RCFGLOCKR		U(0x008)
#define _EXTI_ENCIDCFGR		U(0x180)
#define _EXTI_CMCIDCFGR		U(0x300)
#define _EXTI_HWCFGR1		U(0x3f0)

// CIDCFGR register bitfields
#define _CIDCFGR_CFEN_MASK	BIT(0)
#define _CIDCFGR_CFEN_SHIFT	0
#define _CIDCFGR_SCID_MASK	GENMASK_32(6, 4)
#define _CIDCFGR_SCID_SHIFT	4

/* EXTI_HWCFGR1 bit fields */
#define _EXTI_HWCFGR1_NBEVENTS_MASK	GENMASK_32(7, 0)
#define _EXTI_HWCFGR1_NBEVENTS_SHIFT	0U
#define _EXTI_HWCFGR1_NBCPUS_MASK	GENMASK_32(11, 8)
#define _EXTI_HWCFGR1_NBCPUS_SHIFT	8U
#define _EXTI_HWCFGR1_CPUEVENT_MASK	GENMASK_32(15, 12)
#define _EXTI_HWCFGR1_CPUEVENT_SHIFT	12U
#define _EXTI_HWCFGR1_NBIOPORT_MASK	GENMASK_32(23, 16)
#define _EXTI_HWCFGR1_NBIOPORT_SHIFT	16U
#define _EXTI_HWCFGR1_CIDWIDTH_MASK	GENMASK_32(27, 24)
#define _EXTI_HWCFGR1_CIDWIDTH_SHIFT	24U

/* RIF miscellaneous */
#define _PERIPH_IDS_PER_REG	32
#define SEC_PRIV_X_OFFSET(_id)	(U(0x20) * (_id / _PERIPH_IDS_PER_REG))
#define SEC_PRIV_X_SHIFT(_id)	(_id % _PERIPH_IDS_PER_REG)
#define CID_X_OFFSET(_id)	(U(0x4) * (_id))
#define CMCID_X_OFFSET(_proc)	(U(0x4) * (_proc))

#define EXTI_RIF_RES		U(85)

#define EXTI_MAX_CPU		3

struct stm32_exti_config {
	uintptr_t base;
	const struct rifprot_controller *rif_ctl;
	uint32_t proc_cid[EXTI_MAX_CPU];
};

struct stm32_exti_data {
	uint8_t hw_nbcpus;
	uint8_t hw_nbevents;
};

/*
 * specific function for:
 *  - no standard offset on priv
 *  - init sequence with processor filtering
 */
static __unused
int stm32_exti_rif_set_conf(const struct rifprot_controller *ctl,
			    struct rifprot_config *cfg)
{
	const struct stm32_exti_data *drv_data = dev_get_data(ctl->dev);
	uintptr_t offset = SEC_PRIV_X_OFFSET(cfg->id);
	uint32_t shift = SEC_PRIV_X_SHIFT(cfg->id);

	if (!IS_ENABLED(STM32_M33TDCID))
		return 0;

	if (cfg->id > drv_data->hw_nbevents)
		return -EINVAL;

	/* disable filtering befor write sec and priv cfgr */
	io_clrbits32(ctl->rbase->cid + CID_X_OFFSET(cfg->id), _CIDCFGR_CFEN_MASK);

	io_clrsetbits32(ctl->rbase->sec + offset, BIT(shift),
			cfg->sec << shift);
	io_clrsetbits32(ctl->rbase->priv + offset, BIT(shift),
			cfg->priv << shift);

	io_write32(ctl->rbase->cid + CID_X_OFFSET(cfg->id), cfg->cid_attr);

	return 0;
}

static __unused
int stm32_exti_rif_init(const struct rifprot_controller *ctl)
{
	const struct stm32_exti_config *dev_cfg = dev_get_config(ctl->dev);
	struct stm32_exti_data *drv_data = dev_get_data(ctl->dev);
	uintptr_t cmcidcfgr_base = dev_cfg->base + _EXTI_CMCIDCFGR;
	struct rifprot_config *rcfg_elem;
	int err, i = 0;

	/* disable cmcidcfgr */
	for (i = 0; i <= drv_data->hw_nbcpus; i++)
		io_clrbits32(cmcidcfgr_base + CMCID_X_OFFSET(i),
			     _CIDCFGR_CFEN_MASK);

	for_each_rifprot_cfg(ctl->rifprot_cfg, rcfg_elem, ctl->nrifprot, i) {
		err = stm32_rifprot_set_conf(ctl, rcfg_elem);
		if (err) {
			EMSG("rifprot id:%d setup fail", rcfg_elem->id);
			return err;
		}
	}

	/* enable and set processor filtering cmcidcfgr */
	for (i = 0; i <= drv_data->hw_nbcpus; i++)
		io_write32(cmcidcfgr_base + CID_X_OFFSET(i),
			   _FLD_PREP(_CIDCFGR_SCID, dev_cfg->proc_cid[i]) |
			    _CIDCFGR_CFEN_MASK);

	return 0;
}

static void stm32_exti_get_hwconfig(const struct device *dev)
{
	const struct stm32_exti_config *dev_cfg = dev_get_config(dev);
	struct stm32_exti_data *drv_data = dev_get_data(dev);
	uint32_t regval;

	regval = io_read32(dev_cfg->base + _EXTI_HWCFGR1);
	drv_data->hw_nbcpus = _FLD_GET(_EXTI_HWCFGR1_NBCPUS, regval);
	drv_data->hw_nbevents = _FLD_GET(_EXTI_HWCFGR1_NBEVENTS, regval);
}

static __unused int stm32_exti_init(const struct device *dev)
{
	const struct stm32_exti_config *cfg = dev_get_config(dev);

	if (!cfg)
		return -ENODEV;

	stm32_exti_get_hwconfig(dev);

	return stm32_rifprot_init(cfg->rif_ctl);
}

#define VALUE_2X(i, _) UTIL_X2(i)

#define CID_INIT(idx, inst)							\
	COND_CODE_1(DT_INST_PROP_HAS_IDX(inst, st_proccid, idx),		\
		    ([UTIL_DEC(DT_INST_PROP_BY_IDX(inst, st_proccid, idx))] =	\
		     DT_INST_PROP_BY_IDX(inst, st_proccid, UTIL_INC(idx)),),	\
		     ())

/*
 * Macro to evaluate st,proccid property
 *
 * transform st,proccid = <PROC1 RIF_CID1>, <PROC2 RIF_CID2>;
 * in proc_cid [] = { [PROC1] = RIF_CID1, [PROC2] = RIF_CID2, }
 */
#define PROC_CID_INIT(inst)							\
	FOR_EACH_FIXED_ARG(CID_INIT, (), inst,					\
			   LISTIFY(DT_INST_PROP_LEN_OR(inst, st_proccid, 0),	\
				   VALUE_2X, (,)))

#define STM32_EXTI_INIT(n)						\
									\
static __unused const struct rif_base rbase_##n = {			\
	.sec = DT_INST_REG_ADDR(n) + _EXTI_SECCFGR,			\
	.priv = DT_INST_REG_ADDR(n) + _EXTI_PRIVCFGR,			\
	.cid = DT_INST_REG_ADDR(n) + _EXTI_ENCIDCFGR,			\
};									\
									\
static __unused struct rif_ops rops_##n = {				\
	.set_conf = stm32_exti_rif_set_conf,				\
	.init = stm32_exti_rif_init,					\
};									\
									\
DT_INST_RIFPROT_CTRL_DEFINE(n, &rbase_##n, &rops_##n, EXTI_RIF_RES);	\
									\
static const struct stm32_exti_config exti_cfg_##n = {			\
	.base = DT_INST_REG_ADDR(n),					\
	.rif_ctl = DT_INST_RIFPROT_CTRL_GET(n),				\
	.proc_cid = {PROC_CID_INIT(n)},					\
};									\
									\
static struct stm32_exti_data exti_data_##n = {};			\
									\
DEVICE_DT_INST_DEFINE(n, &stm32_exti_init,				\
		      &exti_data_##n, &exti_cfg_##n,			\
		      CORE, 10, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_EXTI_INIT)
