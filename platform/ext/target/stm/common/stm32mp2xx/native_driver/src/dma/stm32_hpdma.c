// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#define DT_DRV_COMPAT st_stm32_dma3

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

/* HPDMA offset register */
#define _HPDMA_SECCFGR		U(0x00)
#define _HPDMA_PRIVCFGR		U(0x04)
#define _HPDMA_RCFGLOCKR	U(0x08)
#define _HPDMA_CIDCFGR		U(0x054)
#define _HPDMA_SEMCR		U(0x058)

#define _CID_SEM_X_OFFSET(_id)	(U(0x80) * (_id))

// CIDCFGR register bitfields
#define _CIDCFGR_CFEN_MASK	BIT(0)
#define _CIDCFGR_CFEN_SHIFT	0
#define _CIDCFGR_SEMEN_MASK	BIT(1)
#define _CIDCFGR_SEMEN_SHIFT	1
#define _CIDCFGR_SCID_MASK	GENMASK_32(5, 4)
#define _CIDCFGR_SCID_SHIFT	4
#define _CIDCFGR_SEMWLC_MASK	GENMASK_32(18, 16)
#define _CIDCFGR_SEMWLC_SHIFT	16

#define _CIDCFGR_MASK (_CIDCFGR_CFEN_MASK | _CIDCFGR_SEMEN_MASK | \
		       _CIDCFGR_SCID_MASK | _CIDCFGR_SEMWLC_MASK)

#define SEMAPHORE_IS_AVAILABLE(cid_cfgr, my_id)			\
	(_FLD_GET(_CIDCFGR_CFEN, cid_cfgr) &&			\
	 _FLD_GET(_CIDCFGR_SEMEN, cid_cfgr) &&			\
	 ((_FLD_GET(_CIDCFGR_SEMWLC, cid_cfgr)) == BIT(my_id)))

// SEMCR register bitfields
#define _SEMCR_MUTEX_MASK	BIT(0)
#define _SEMCR_MUTEX_SHIFT	0
#define _SEMCR_SCID_MASK	GENMASK_32(5, 4)
#define _SEMCR_SCID_SHIFT	U(4)

/* RIF miscellaneous */
#define HPDMA_RIF_RES		U(16)

#define MY_CID RIF_CID2

struct stm32_hpdma_config {
	uintptr_t base;
	const struct rifprot_controller *rif_ctl;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
};

/*
 * specific function for:
 *  - no standard offset on priv and sem
 */
static __unused
int stm32_hpdma_rif_acquire_sem(const struct rifprot_controller *ctl,
				uint32_t id)
{
	uint32_t semcr;

	io_setbits32(ctl->rbase->sem + _CID_SEM_X_OFFSET(id), _SEMCR_MUTEX_MASK);

	semcr = io_read32(ctl->rbase->sem + _CID_SEM_X_OFFSET(id));
	if (semcr != (_SEMCR_MUTEX_MASK | _FLD_PREP(_SEMCR_SCID, MY_CID)))
		return -EPERM;

	return 0;
}

static __unused
int stm32_hpdma_rif_release_sem(const struct rifprot_controller *ctl,
				uint32_t id)
{
	uint32_t semcr;

	semcr = io_read32(ctl->rbase->sem + _CID_SEM_X_OFFSET(id));
	/* if no semaphore */
	if (!(semcr & _SEMCR_MUTEX_MASK))
		return 0;

	/* if semaphore taken but not my cid */
	if (semcr != (_SEMCR_MUTEX_MASK | _FLD_PREP(_SEMCR_SCID, MY_CID)))
		return -EPERM;

	io_clrbits32(ctl->rbase->sem + _CID_SEM_X_OFFSET(id), _SEMCR_MUTEX_MASK);

	return 0;
}

static __unused
int stm32_hpdma_rif_set_conf(const struct rifprot_controller *ctl,
			     struct rifprot_config *cfg)
{
	uint32_t shift = cfg->id;

	if (!IS_ENABLED(STM32_M33TDCID))
		return 0;

	/* disable filtering befor write sec and priv cfgr */
	io_clrbits32(ctl->rbase->cid + _CID_SEM_X_OFFSET(cfg->id), _CIDCFGR_MASK);

	io_clrsetbits32(ctl->rbase->sec, BIT(shift), cfg->sec << shift);
	io_clrsetbits32(ctl->rbase->priv, BIT(shift), cfg->priv << shift);

	io_write32(ctl->rbase->cid + _CID_SEM_X_OFFSET(cfg->id), cfg->cid_attr);

	if (SEMAPHORE_IS_AVAILABLE(cfg->cid_attr, MY_CID))
		return stm32_rifprot_acquire_sem(ctl, cfg->id);

	/*
	 * Lock RIF configuration if configured. This cannot be undone until
	 * next reset.
	 */
	io_clrsetbits32(ctl->rbase->lock, BIT(shift), cfg->lock << shift);

	return 0;
}

static __unused int stm32_hpdma_init(const struct device *dev)
{
	const struct stm32_hpdma_config *cfg = dev_get_config(dev);
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

	err = stm32_rifprot_init(cfg->rif_ctl);

	clk_disable(clk);

	return err;
}

#define STM32_HPDMA_INIT(n)						\
									\
static __unused const struct rif_base rbase_##n = {			\
	.sec = DT_INST_REG_ADDR(n) + _HPDMA_SECCFGR,			\
	.priv = DT_INST_REG_ADDR(n) + _HPDMA_PRIVCFGR,			\
	.cid = DT_INST_REG_ADDR(n) + _HPDMA_CIDCFGR,			\
	.sem = DT_INST_REG_ADDR(n) + _HPDMA_SEMCR,			\
	.lock = DT_INST_REG_ADDR(n) + _HPDMA_RCFGLOCKR,			\
};									\
									\
static __unused struct rif_ops rops_##n = {				\
	.set_conf = stm32_hpdma_rif_set_conf,				\
	.acquire_sem = stm32_hpdma_rif_acquire_sem,			\
	.release_sem = stm32_hpdma_rif_release_sem,			\
};									\
									\
DT_INST_RIFPROT_CTRL_DEFINE(n, &rbase_##n, &rops_##n, HPDMA_RIF_RES);	\
									\
static const struct stm32_hpdma_config hpdma_cfg_##n = {		\
	.base = DT_INST_REG_ADDR(n),					\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),	\
	.rif_ctl = DT_INST_RIFPROT_CTRL_GET(n),				\
};									\
									\
DEVICE_DT_INST_DEFINE(n, &stm32_hpdma_init,				\
		      NULL, &hpdma_cfg_##n,				\
		      CORE, 10, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_HPDMA_INIT)
