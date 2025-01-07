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
#include <lib/mmiopoll.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <firewall.h>
#include <remoteproc.h>
#include <reset.h>
#include <clk.h>
#include <cmsis.h>

#define IRQ_INVALID	UINT32_MAX

struct clock_control {
	const struct device *dev;
	const clk_subsys_t subsys;
};

struct stm32_rproc_variant {
	int (*init_fn)(const struct device *dev);
	int (*start_fn)(const struct device *dev);
	int (*stop_fn)(const struct device *dev);
	void (*irq_handler)(const struct device *dev);
};

struct stm32_rproc_config {
	const struct reset_control rst_ctl;
	const struct firewall_spec *firewall_ctrls;
	const int n_firewall_ctrls;
	const uint32_t irq_ack;
	const struct clock_control *clk_ctl;
	int n_clk;
};

struct stm32_rproc_data {
	struct rproc_spec rproc;
	const struct stm32_rproc_variant *variant;
};

/*
 * FIXME
 * All direct access to exti, pwr or IAC hadware block must be rework.
 * Wait the interrupt framework to enable or mask a specific interrupt
 */
#define PWR_CPU1D1SR_DSTATE_STANDBY ( 0x4 << PWR_CPU1D1SR_DSTATE_Pos)
/*
 * Stop procedure is setting the cortex A in reset on holboot.
 * At cold start the cortexA is in standby reset and all exti are masked.
 * so before send event we must:
 * - unmask cpu2_sev (exti1 64) of cortexA (C1)
 * - apply rif access on exti (exti driver)
 * - send event by exti software interrupt
 * At cold start When the m33 debug wrapper is used (cortexA is in standby stop)
 * or when cortex A is running :
 */
static __unused int stm32mp2_a35_stop(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	uint32_t cfgr;

	/* the deassert release hold boot and set hold boot */
	reset_control_deassert(&cfg->rst_ctl);

	/* clear event if pending */
	EXTI1->RPR3 = BIT(0);
	/* send CPU2 SEV event to cpu1 (exti 64)*/
	EXTI1->SWIER3 |= BIT(0);
	/* reset cpu */
	reset_control_assert(&cfg->rst_ctl);
	/* check cpu in holdboot */

	return  mmio_read32_poll_timeout(((uint32_t)&PWR_S->CPU1D1SR), cfgr,
					 (cfgr & PWR_CPU1D1SR_HOLD_BOOT_Msk) &&
					 ((cfgr & PWR_CPU1D1SR_DSTATE)
					  != PWR_CPU1D1SR_DSTATE_STANDBY), 10);
}

static __unused int stm32mp2_a35_init(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);

	/* unmask event before rif has initialized CID1 filtering on EXTI1_C1CIDCFGR*/
	EXTI1->C1IMR3 |= BIT(0);

	if (cfg->irq_ack != IRQ_INVALID){
		/* enable rising trigger */
		EXTI1->RTSR3 |= BIT(1);
		NVIC_SetPriority(cfg->irq_ack, 1);
	}

	return stm32mp2_a35_stop(dev);
}

#define IAC_BIT(_id) BIT(_id % 32)

static __unused int stm32mp2_a35_start(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	struct firewall_spec *firewall;
	uint32_t cfgr;
	int i, err;

	if (((PWR_S->CPU1D1SR & PWR_CPU1D1SR_DSTATE) == PWR_CPU1D1SR_DSTATE_STANDBY)
	    ||!(PWR_S->CPU1D1SR & PWR_CPU1D1SR_HOLD_BOOT_Msk)) {
		err = stm32mp2_a35_stop(dev);
		if (err)
			return err;
	}

	if (cfg->irq_ack != IRQ_INVALID) {
		/* clear rising pending register */
		EXTI1->RPR3 = BIT(1);
		/* unmask C2 int & event C1SEV (65) */
		EXTI1->C2IMR3 |= BIT(1);
		EXTI1->C2EMR3 |= BIT(1);
		NVIC_EnableIRQ(cfg->irq_ack);
	}

	/* set bootrom access controller configuration */
	for_each_firewall(cfg->firewall_ctrls, firewall,
			  cfg->n_firewall_ctrls, i) {
		err = firewall_set_configuration(firewall);
		if (err != 0) {
			DMSG("[%s] fail to set firewall conf %d\n", __func__, i);
			return err;
		}
	}

	/*
	 * IAC workaround
	 * mask:
	 * RAMCFG(108), TAMP(152), PWR(155), RCC(156)
	 */
	IAC->IER[3] &= ~(IAC_BIT(108));
	IAC->IER[4] &= ~(IAC_BIT(152) | IAC_BIT(155) | IAC_BIT(156));

	/*  power up cpu, in case it is in standby */
	err = reset_control_deassert(&cfg->rst_ctl);
	if (err)
		return err;

	/*  check cpu is not in holbot */
	return mmio_read32_poll_timeout(((uint32_t)&PWR_S->CPU1D1SR), cfgr,
					!(cfgr & PWR_CPU1D1SR_HOLD_BOOT_Msk) ||
					((cfgr & PWR_CPU1D1SR_DSTATE)
					 == PWR_CPU1D1SR_DSTATE_STANDBY), 10);

}

static __unused int stm32mp2_a35_release(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	const struct clock_control *clock_ctl;
	struct firewall_spec *firewall;
	int i, ret = 0, err;
	struct clk *clk;

	/* release firewall access right needed by bootromm */
	for_each_firewall(cfg->firewall_ctrls, firewall, cfg->n_firewall_ctrls, i) {
		err = firewall_release_configuration(firewall);
		if (err) {
			EMSG("[%s] release firewall[%d] err:%d\n", __func__, i, err);
			ret = -EINVAL;
		}
	}

	/*
	 * IAC workaround
	 * clear and unmask:
	 * RAMCFG(108), TAMP(152), PWR(155), RCC(156)
	 */
	IAC->ICR[3] = IAC_BIT(108);
	IAC->ICR[4] = (IAC_BIT(152) | IAC_BIT(155) | IAC_BIT(156));
	IAC->IER[3] |= IAC_BIT(108);
	IAC->IER[4] |= (IAC_BIT(152) | IAC_BIT(155) | IAC_BIT(156));

	/* restore clocks touched by bootrom */
	for (i = 0, clock_ctl = cfg->clk_ctl; i < cfg->n_clk; i++, clock_ctl++) {

		clk = clk_get(clock_ctl->dev, clock_ctl->subsys);
		if (clk)
			clk_restore_context(clk);
		else
			WMSG("%s: get clock[%d] fail\n", __func__, i);
	}

	if (cfg->irq_ack != IRQ_INVALID) {
		NVIC_DisableIRQ(cfg->irq_ack);
		NVIC_ClearPendingIRQ(cfg->irq_ack);
		/* mask c2 int & event C1SEV */
		EXTI1->C2IMR3 &= ~BIT(1);
		EXTI1->C2EMR3 &= ~BIT(1);
		/* clear rising pending register C1SEV */
		EXTI1->RPR3 = BIT(1);
	}

	return ret;
}


static __unused void stm32mp2_a35_irq_ack(const struct device *dev)
{
	stm32mp2_a35_release(dev);
}

static struct rproc_spec *stm32_rproc_get(const struct device *dev)
{
	struct stm32_rproc_data *data = dev_get_data(dev);

	return &data->rproc;
}

int stm32_rproc_start(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->start_fn)
		return -ENOTSUP;

	return data->variant->start_fn(rproc->dev);
}

int stm32_rproc_stop(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->stop_fn)
		return -ENOTSUP;

	return data->variant->stop_fn(rproc->dev);
}

static __unused int stm32_rproc_init(const struct device *dev)
{
	struct stm32_rproc_data *data = dev_get_data(dev);
	uint32_t err = 0;

	if (data->variant->init_fn)
		err = data->variant->init_fn(dev);

	rproc_init(dev, &data->rproc);

	return err;
}

static __unused const struct  stm32_rproc_variant stm32mp2_a35_var = {
	.init_fn = stm32mp2_a35_init,
	.start_fn = stm32mp2_a35_start,
	.stop_fn = stm32mp2_a35_stop,
	.irq_handler = stm32mp2_a35_irq_ack,
};

static struct remoteproc_driver_api stm32_rproc_api = {
	.get_rproc = stm32_rproc_get,
	.start = stm32_rproc_start,
	.stop = stm32_rproc_stop,
};

#define DT_CLOCK_CONTROL_GET_BY_IDX(node_id, idx)					\
	{										\
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),		\
		.subsys = (clk_subsys_t) DT_CLOCKS_CELL_BY_IDX(node_id, idx, bits)	\
	}

#define _DT_CLOCK_CTL(clk_id, node_id) DT_CLOCK_CONTROL_GET_BY_IDX(node_id, clk_id)

#define DT_CLOCK_CONTROL(node_id)						\
	{									\
		LISTIFY(DT_NUM_CLOCKS(node_id), _DT_CLOCK_CTL, (,), node_id)	\
	}

#define DT_INST_CLOCK_CONTROL(inst) DT_CLOCK_CONTROL(DT_DRV_INST(inst))

#define DT_INST_IRQ_BY_NAME_OR(n, name, cell)					\
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(n, name),				\
		    (DT_INST_IRQ_BY_NAME(n, name, cell)),			\
		    (IRQ_INVALID))

#define STM32_RPROC_INIT(n, name, _variant, _irqhandler)			\
										\
DT_INST_ACCESS_CTRLS_DEFINE(n);							\
										\
static const struct clock_control clk_ctrl_##n[] = DT_INST_CLOCK_CONTROL(n);	\
										\
static const struct stm32_rproc_config _##name##_cfg##n = {			\
	.rst_ctl = DT_INST_RESET_CONTROL_GET(n),				\
	.firewall_ctrls = DT_INST_ACCESS_CTRLS_GET(n),				\
	.n_firewall_ctrls = DT_INST_ACCESS_CTRLS_NUM(n),			\
	.irq_ack = DT_INST_IRQ_BY_NAME_OR(n, ack, irq),				\
	.clk_ctl = clk_ctrl_##n,						\
	.n_clk = DT_INST_NUM_CLOCKS(n),						\
};										\
										\
static struct stm32_rproc_data _##name##_data##n = {				\
	.variant = &_variant,							\
};										\
										\
void _irqhandler(void)								\
{										\
	if (_variant.irq_handler)						\
		_variant.irq_handler(DEVICE_DT_INST_GET(n));			\
}										\
										\
DEVICE_DT_INST_DEFINE(n, &stm32_rproc_init,					\
		      &_##name##_data##n, &_##name##_cfg##n,			\
		      CORE, 9, &stm32_rproc_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp2_a35

DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_RPROC_INIT, DT_DRV_COMPAT,
				  stm32mp2_a35_var, CPU1_SEV_IRQHandler)
