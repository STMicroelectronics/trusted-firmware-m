// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 */
#define DT_DRV_COMPAT st_stm32mp25_ipcc

#include <stdint.h>
#include <string.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <debug.h>
#include <mbox.h>
#include <clk.h>
#include <stm32_rif.h>

#define _IPCC_C1CR			U(0x00)
#define _IPCC_C1MR			U(0x04)
#define _IPCC_C1SCR			U(0x08)
#define _IPPC_C1TOC2SR			U(0x0C)
#define _IPCC_C2CR			U(0x10)
#define _IPCC_C2MR			U(0x14)
#define _IPCC_C2SCR			U(0x18)
#define _IPPC_C2TOC2SR			U(0x1C)
#define _IPCC_C1SECCFGR			U(0x80)
#define _IPCC_C1PRIVCFGR		U(0x84)
#define _IPCC_C1CIDCFGR			U(0x88)
#define _IPCC_HWCFGR			U(0x3F0)
#define _IPCC_VER			U(0x3F4)

/* Offset within a core instance */
#define IPCC_CR				U(0x0)
#define IPCC_MR				U(0x4)
#define IPCC_SCR			U(0x8)
#define IPCC_TOSR			U(0xC)
#define IPCC_SECCFGR			U(0x80)
#define IPCC_PRIVCFGR			U(0x84)
#define IPCC_CIDCFGR			U(0x98)
#define IPCC_RIF_CX_OFFSET(_id)		((_id) * U(0x10))

/* Mask for  channel rxo and txf */
#define IPCC_ALL_MR_TXF_CH_MASK		GENMASK_32(31, 16)
#define IPCC_ALL_MR_RXO_CH_MASK		GENMASK_32(15, 0)
#define IPCC_ALL_SR_CH_MASK		GENMASK_32(15, 0)

/* Define for core instance register */
#define IPCC_MR_CH1FM_POS		U(16)
#define IPCC_SCR_CH1S_POS		U(16)

/* CIDCFGR register bitfields */
#define _CIDCFGR_CFEN_MASK		BIT(0)
#define _CIDCFGR_CFEN_SHIFT		0
#define _CIDCFGR_SCID_MASK		GENMASK_32(7, 4)
#define _CIDCFGR_SCID_SHIFT		4

/* IPCC_HWCFGR register bitfields */
#define _HWCFGR_CHAN_MASK		GENMASK_32(7, 0)
#define _HWCFGR_CHAN_SHIFT		0

/* RIF miscellaneous */
#define IPCC_RIF_MAX_RES		U(32)
#define IPCC_RIF_FIRST_IPCC2_ID		16

enum {
	IPCC_ITR_RXO,
	IPCC_ITR_TXF,
	IPCC_ITR_NUM,
};

#define MAX_CHANNELS 16
struct stm32_ipcc_config {
	uintptr_t base;
	/* remote processor base address */
	uintptr_t rbase;
	/* local processor base address */
        uintptr_t lbase;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	const struct rifprot_controller *rif_ctl;
	const uint32_t irq;
};

struct stm32_ipcc_data {
	struct clk *clk;
	uint32_t channel_sec_mask;
        uint32_t channel_enable_mask;
	uint32_t hw_n_ch;
	mbox_callback_t cb[MAX_CHANNELS];
	void *user_data[MAX_CHANNELS];
};

static bool ipcc_channel_is_active(uintptr_t lbase, unsigned int chn)
{
	return io_read32(lbase + IPCC_TOSR) & BIT(chn);
}

static void ipcc_channel_transmit(uintptr_t lbase, unsigned int chn, bool enable)
{
	if (enable)
		io_clrbits32(lbase + IPCC_MR, BIT(chn + IPCC_MR_CH1FM_POS));
	else
		io_setbits32(lbase + IPCC_MR, BIT(chn + IPCC_MR_CH1FM_POS));
}

static void ipcc_channel_receive(uintptr_t lbase, unsigned int chn, bool enable)
{
	if (enable)
		io_clrbits32(lbase + IPCC_MR, BIT(chn));
	else
		io_setbits32(lbase + IPCC_MR, BIT(chn));
}

static inline bool is_channel_valid(const struct device *dev, uint32_t chn)
{
	struct stm32_ipcc_data *drv_data = dev_get_data(dev);

	if (chn >= drv_data->hw_n_ch ||
	    !(BIT(chn) & drv_data->channel_sec_mask))
		return false;

	return true;
}

static __unused void stm32_ipcc_mbox_rxo_isr(const struct device *dev)
{
	struct stm32_ipcc_data *drv_data = dev_get_data(dev);
	const struct stm32_ipcc_config *drv_cfg = dev_get_config(dev);
	uint32_t mask = 0;
	unsigned int chn = 0;

	mask = ~io_read32(drv_cfg->lbase + IPCC_MR) & IPCC_ALL_MR_RXO_CH_MASK;
	mask &= io_read32(drv_cfg->rbase + IPCC_TOSR) & IPCC_ALL_SR_CH_MASK;

	/* Get mask for enabled channels only */
	mask &= drv_data->channel_enable_mask;

	for (chn = 0; chn < drv_data->hw_n_ch; chn++) {
		if (!(BIT(chn) & mask))
			continue;
		/* Clear rxo flag */
		io_write32(drv_cfg->lbase + IPCC_SCR, BIT(chn));

		FMSG("rx channel = %"PRIu32, chn);
		if (drv_data->cb[chn])
			drv_data->cb[chn](dev, chn, drv_data->user_data[chn],
					  (struct mbox_msg *)NULL);

	}
}


static __unused void stm32_ipcc_mbox_txf_isr(const struct device *dev)
{
	struct stm32_ipcc_data *drv_data = dev_get_data(dev);
	const struct stm32_ipcc_config *drv_cfg = dev_get_config(dev);
	uint32_t mask = 0;
	uint32_t i = 0;

	mask = ~io_read32(drv_cfg->lbase + IPCC_MR) & IPCC_ALL_MR_TXF_CH_MASK;
	mask = mask >> IPCC_MR_CH1FM_POS;

	mask &= ~io_read32(drv_cfg->lbase + IPCC_TOSR) & IPCC_ALL_SR_CH_MASK;

	/* Get mask for enabled channels only */
	mask &= drv_data->channel_enable_mask;

	for (i = 0; i < drv_data->hw_n_ch; i++) {
		if (BIT(i) & mask) {
			/* Disable txf interrupt */
			ipcc_channel_transmit(drv_cfg->lbase, i, false);
		}
	}
}

static int stm32_ipcc_send(const struct device *dev, uint32_t chn,
			    const struct mbox_msg *msg)
{
	const struct stm32_ipcc_config *drv_cfg = dev_get_config(dev);

	/* No data transmission, only doorbell */
	if ((msg && msg->size) || !is_channel_valid(dev, chn))
		return -EINVAL;

	/* Check that the channel is free */
	if (ipcc_channel_is_active(drv_cfg->lbase, chn)) {
		FMSG("Waiting for channel to be freed");
		return -EBUSY;
	}

	/* Set channel txs Flag */
	io_write32(drv_cfg->lbase + IPCC_SCR, BIT(chn + IPCC_SCR_CH1S_POS));

	return 0;
}

static int stm32_ipcc_register_cb(const struct device *dev, uint32_t channel,
					 mbox_callback_t cb, void *user_data)
{
	struct stm32_ipcc_data *data = dev_get_data(dev);

	if (!is_channel_valid(dev, channel)) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int stm32_ipcc_mtu_get(const struct device *dev)
{
	/* No data transfer capability */
	return 0;
}

static uint32_t stm32_ipcc_max_channels_get(const struct device *dev)
{
	struct stm32_ipcc_data *drv_data = dev_get_data(dev);

	return drv_data->hw_n_ch;
}

static int stm32_ipcc_set_enabled(const struct device *dev, uint32_t chn,
				   bool enable)
{
	struct stm32_ipcc_data *drv_data = dev_get_data(dev);
	const struct stm32_ipcc_config *drv_cfg = dev_get_config(dev);
	uint32_t channel_enable_mask = 0;

	if (!(BIT(chn) & drv_data->channel_sec_mask))
		return -EINVAL;

	if (enable)
		channel_enable_mask = (BIT(chn) & drv_data->channel_sec_mask) |
			drv_data->channel_enable_mask;
	else
		channel_enable_mask = ~(BIT(chn) & drv_data->channel_sec_mask) &
			drv_data->channel_enable_mask;

	if (channel_enable_mask && !drv_data->channel_enable_mask) {
		/* Enable secure txf and rxo interrupt */
		io_setbits32(drv_cfg->lbase + IPCC_CR,
			     IPCC_CR_SECTXFIE | IPCC_CR_SECRXOIE);
	}

	if (!channel_enable_mask && drv_data->channel_enable_mask) {
		/* Disable secure txf and rxo interrupt */
		io_clrbits32(drv_cfg->lbase + IPCC_CR,
			     IPCC_CR_SECTXFIE | IPCC_CR_SECRXOIE);
	}

	/*
	 * Update mask and then enable rxo interrupt
	 * since mask is used within rxo interrupt handler and
	 * do the opposite for disable
	 */
	if (enable)
		drv_data->channel_enable_mask = channel_enable_mask;

	ipcc_channel_receive(drv_cfg->lbase, chn, enable);

	if (!enable)
		drv_data->channel_enable_mask = channel_enable_mask;

	return 0;
}

/*
 * IPCC has one cid configuration by cpu and set all cid access of cpuX channel.
 * Evaluate RIF cid filtering configuration before setting it.
 * Rif parsing configuration must be consistent for all channel of same cpu:
 * - cfen
 * - cid
 */
static __unused int stm32_ipcc_rif_init(const struct rifprot_controller *ctl)
{
	struct rifprot_config *rcfg_elem, *rcfg_prev[2] = {NULL, NULL};
	uint32_t sec[2] = {0, 0}, priv[2] = {0, 0};
	int i = 0;

	for_each_rifprot_cfg(ctl->rifprot_cfg, rcfg_elem, ctl->nrifprot, i) {
		uint8_t cpux_id = rcfg_elem->id / IPCC_RIF_FIRST_IPCC2_ID;
		uint8_t channel = rcfg_elem->id % IPCC_RIF_FIRST_IPCC2_ID;

		sec[cpux_id] |= rcfg_elem->sec << channel;
		priv[cpux_id] |= rcfg_elem->priv << channel;

		if (!rcfg_prev[cpux_id]) {
			rcfg_prev[cpux_id] = rcfg_elem;
			continue;
		}

		if (rcfg_prev[cpux_id]->cid_attr != rcfg_elem->cid_attr)
			return -EINVAL;
	}

	for(i = 0; i < 2; i++) {
		uint32_t cx_offset = IPCC_RIF_CX_OFFSET(i);

		io_clrbits32(ctl->rbase->cid + cx_offset, _CIDCFGR_CFEN_MASK);
		io_write32(ctl->rbase->sec + cx_offset, sec[i]);
		io_write32(ctl->rbase->priv + cx_offset, priv[i]);
		io_write32(ctl->rbase->cid + cx_offset, rcfg_prev[i]->cid_attr);
	}

	return 0;
}

static void stm32_ipcc_get_hwconfig(const struct device *dev)
{
	const struct stm32_ipcc_config *dev_cfg = dev_get_config(dev);
	struct stm32_ipcc_data *dev_data = dev_get_data(dev);

	dev_data->hw_n_ch = _FLD_GET(_HWCFGR_CHAN,
				     io_read32(dev_cfg->base + _IPCC_HWCFGR));
}

static int stm32_ipcc_init(const struct device *dev)
{
	const struct stm32_ipcc_config *drv_cfg = dev_get_config(dev);
	struct stm32_ipcc_data *drv_data = dev_get_data(dev);
	int err;
	int i;

	drv_data->clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);
	if (!drv_data->clk)
		return -ENODEV;

	/* Clock is enabled once in product life and remains on */
	err = clk_enable(drv_data->clk);
	if (err)
		return err;

	stm32_ipcc_get_hwconfig(dev);

	if (drv_cfg->rif_ctl) {
		err = stm32_rifprot_init(drv_cfg->rif_ctl);
		if (err)
			goto err;
	}

	/* Disable secure rxo and txf interrupts */
	io_clrbits32(drv_cfg->lbase + IPCC_CR, IPCC_CR_SECTXFIE |
		     IPCC_CR_SECRXOIE);

	/* Set channel_sec_mask according to rif protection */
	drv_data->channel_sec_mask = io_read32(drv_cfg->lbase + IPCC_SECCFGR);
	drv_data->channel_enable_mask = 0;

	/* Fix Me : Possibly Add Check on cid filtering and privileged */

	for (i = 0; i < drv_data->hw_n_ch; i++) {
		/* Clear rxo status */
		io_write32(drv_cfg->lbase + IPCC_SCR, BIT(i));
		/* Mask channel rxo and txf interrupts */
		ipcc_channel_receive(drv_cfg->lbase, i, false);
		ipcc_channel_transmit(drv_cfg->lbase, i, false);
	}

	/* Enable RXO interrupt */
	NVIC_SetPriority(drv_cfg->irq, 1);
	NVIC_EnableIRQ(drv_cfg->irq);

	return 0;

err:
	clk_disable(drv_data->clk);

	return err;
}

static const struct mbox_driver_api stm32_ipcc_api = {
	.send = stm32_ipcc_send,
	.register_callback = stm32_ipcc_register_cb,
	.mtu_get = stm32_ipcc_mtu_get,
	.max_channels_get = stm32_ipcc_max_channels_get,
	.set_enabled = stm32_ipcc_set_enabled,
};

#define STM32_MBOX_INIT(n)							\
										\
static __unused const struct rif_base rbase_##n = {				\
	.sec = DT_INST_REG_ADDR(n) + _IPCC_C1SECCFGR,				\
	.priv = DT_INST_REG_ADDR(n) + _IPCC_C1PRIVCFGR,				\
	.cid = DT_INST_REG_ADDR(n) + _IPCC_C1CIDCFGR,				\
};										\
										\
static __unused struct rif_ops rops_##n = {					\
	.init = stm32_ipcc_rif_init,						\
};										\
										\
DT_INST_RIFPROT_CTRL_DEFINE(n, &rbase_##n, &rops_##n, IPCC_RIF_MAX_RES);	\
										\
static const struct stm32_ipcc_config cfg_##n = {				\
	.base = DT_INST_REG_ADDR(n),						\
	.lbase =  DT_INST_REG_ADDR(n) + _IPCC_C2CR,				\
	.rbase =  DT_INST_REG_ADDR(n) + _IPCC_C1CR,				\
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(n, bits),		\
	.rif_ctl = DT_INST_RIFPROT_CTRL_GET(n),					\
	.irq = DT_INST_IRQN(0),							\
};										\
										\
										\
void IPCC_HANDLE_##n(void) {							\
stm32_ipcc_mbox_rxo_isr(DEVICE_DT_GET(DT_DRV_INST(n)));				\
};										\
										\
static struct stm32_ipcc_data data_##n = {};					\
										\
DEVICE_DT_INST_DEFINE(n,							\
		      &stm32_ipcc_init,						\
		      &data_##n, &cfg_##n,					\
		      CORE, 5,							\
		      &stm32_ipcc_api);

DT_INST_FOREACH_STATUS_OKAY(STM32_MBOX_INIT)
