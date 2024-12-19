// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#define DT_DRV_COMPAT st_stm32mp25_rtc

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

/* RTC offset register */
#define _RTC_PRIVCFGR		U(0x1C)
#define _RTC_SECCFGR		U(0x20)
#define _RTC_CIDCFGR		U(0x80)

#define _CID_X_OFFSET(_id)	(U(0x4) * (_id))
// CIDCFGR register bitfields
#define _CIDCFGR_CFEN_MASK	BIT(0)
#define _CIDCFGR_CFEN_SHIFT	0
#define _CIDCFGR_SCID_MASK	GENMASK_32(6, 4)
#define _CIDCFGR_SCID_SHIFT	4

/* RIF miscellaneous */
#define RTC_PRIV_SEC_ID_SHIFT	GENMASK_32(5, 4)
#define RTC_SECCFGR_SHIFT	U(9)

#define RTC_RIF_RES		U(6)

struct stm32_rtc_config {
	uintptr_t base;
	const struct rifprot_controller *rif_ctl;
	const struct device *pclk_dev;
	const clk_subsys_t pclk_subsys;
	const struct device *rtc_clk_dev;
	const clk_subsys_t rtc_clk_subsys;
};

/*
 * the devicetree provide a configuration per resource (no general
 * configuration), so each resource protection is managed individually
 * and the global SEC & PRIV configuration are not take account.
 */
static int stm32_rtc_rif_set_conf(const struct rifprot_controller *ctl,
				  struct rifprot_config *cfg)
{
	uint32_t shift = cfg->id;

	if (!IS_ENABLED(STM32_M33TDCID))
		return 0;

	if (BIT(cfg->id) & RTC_PRIV_SEC_ID_SHIFT)
		shift += RTC_SECCFGR_SHIFT;

	/* disable filtering befor write sec and priv cfgr */
	io_clrbits32(ctl->rbase->cid + _CID_X_OFFSET(cfg->id), _CIDCFGR_CFEN_MASK);

	io_clrsetbits32(ctl->rbase->sec, BIT(shift), cfg->sec << shift);
	io_clrsetbits32(ctl->rbase->priv, BIT(shift), cfg->priv << shift);

	io_write32(ctl->rbase->cid + _CID_X_OFFSET(cfg->id), cfg->cid_attr);

	return 0;
}

static int stm32_rtc_init(const struct device *dev)
{
	const struct stm32_rtc_config *cfg = dev_get_config(dev);
	struct clk *rtc_clk, *pclk;
	int err;

	if (!cfg)
		return -ENODEV;

	rtc_clk = clk_get(cfg->rtc_clk_dev, cfg->rtc_clk_subsys);
	pclk = clk_get(cfg->pclk_dev, cfg->pclk_subsys);

	if (!rtc_clk || !pclk)
		return -ENODEV;

	err = clk_enable(rtc_clk);
	if (err)
		return err;

	err = clk_enable(pclk);
	if (err)
		return err;

	err = stm32_rifprot_init(cfg->rif_ctl);

	clk_disable(pclk);
	clk_disable(rtc_clk);

	return err;
}

#define STM32_RTC_INIT(n)								\
											\
static const struct rif_base rbase_##n = {						\
	.sec = DT_INST_REG_ADDR(n) + _RTC_SECCFGR,					\
	.priv = DT_INST_REG_ADDR(n) + _RTC_PRIVCFGR,					\
	.cid = DT_INST_REG_ADDR(n) + _RTC_CIDCFGR,					\
	.sem = 0,									\
};											\
											\
struct rif_ops rops_##n = {								\
	.set_conf = stm32_rtc_rif_set_conf,						\
};											\
											\
DT_INST_RIFPROT_CTRL_DEFINE(n, &rbase_##n, &rops_##n, RTC_RIF_RES);			\
											\
static const struct stm32_rtc_config rtc_cfg_##n = {					\
	.base = DT_INST_REG_ADDR(n),							\
	.rif_ctl = DT_INST_RIFPROT_CTRL_GET(n),						\
	.pclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, pclk)),		\
	.pclk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL_BY_NAME(n, pclk, bits),	\
	.rtc_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, rtc_ck)),		\
	.rtc_clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL_BY_NAME(n, rtc_ck, bits),	\
};											\
											\
DEVICE_DT_INST_DEFINE(n, &stm32_rtc_init,						\
		      NULL, &rtc_cfg_##n,						\
		      CORE, 10, NULL);

DT_INST_FOREACH_STATUS_OKAY(STM32_RTC_INIT)
