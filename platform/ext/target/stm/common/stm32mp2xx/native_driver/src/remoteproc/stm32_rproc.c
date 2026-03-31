// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <lib/delay.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <inttypes.h>
#include <debug.h>
#include <errno.h>

#include <device.h>
#include <firewall.h>
#include <stm32mp2_pwr.h>
#include <regulator.h>
#include <remoteproc.h>
#include <reset.h>
#include <clk.h>
#include <cmsis.h>
#include <stm32_bsec3.h>
#include <stm32_tamp.h>
#include <tfm_platform_system.h>
#include <devicetree.h>
#include <devicetree/nvmem.h>
#include <nvmem.h>
#include <rproc_srm_core.h>

#define IRQ_INVALID	UINT32_MAX

struct clock_control {
	const struct device *dev;
	const clk_subsys_t subsys;
};

struct stm32_rproc_variant {
	int (*init_fn)(const struct device *dev);
	int (*start_fn)(const struct device *dev);
	bool (*is_running_fn)(const struct device *dev);
	int (*stop_fn)(const struct device *dev);
	int (*suspend_fn)(const struct device *dev);
	int (*resume_fn)(const struct device *dev);
	void (*irq_handler)(const struct device *dev);
};

struct stm32_rproc_config {
	const struct reset_control rst_ctl;
	const struct reset_control hold_boot;
	const struct firewall_spec *firewall_ctrls;
	const int n_firewall_ctrls;
	const uint32_t irq_ack;
	const struct clock_control *clk_ctl;
	int n_clk;
	const struct device **regu;
	int nb_regu;
	const struct device **rproc_srm;
	int nb_rproc_srm;

};

struct stm32_rproc_data {
	struct rproc_spec rproc;
	bool running;
	bool restore_on_ack;
	const struct stm32_rproc_variant *variant;
	const struct device *rsc_tab_addr_dev;
	const struct device *rsc_tab_size_dev;
	const struct device *stop2_nvmem_dev;
};

static void stm32_rproc_running_set(const struct device *dev, bool running)
{
	struct stm32_rproc_data *data = dev_get_data(dev);

	data->running = running;
}

static bool stm32_rproc_running_get(const struct device *dev)
{
	struct stm32_rproc_data *data = dev_get_data(dev);

	return data->running;
}

static __unused bool stm32mp2_a35_is_running(const struct device *dev)
{
	return stm32_rproc_running_get(dev);
}

static int stm32mp2_a35_regu(const struct device *dev)
{
	int i;
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	int nb_regu = cfg->nb_regu;
	int32_t volt_uv;
	int err;

	for (i = 0; i < nb_regu; i++) {
		err = regulator_force_disable(cfg->regu[i]);
		if (err ) {
			EMSG("[%s] regu force disable err: %d", cfg->regu[i]->name, err);
			return err;
		}
	}

	/* force default voltage */
	for (i = 0; i < nb_regu; i++) {
		if (!regulator_get_default_voltage(cfg->regu[i], &volt_uv)) {
			err = regulator_set_voltage(cfg->regu[i], volt_uv, volt_uv);
			if (err) {
				EMSG("[%s] regu set voltage err: %d", cfg->regu[i]->name, err);
				return err;
			}
		}
	}
	udelay(10000);

	/* enable ALL regu */
	for (i = 0; i < nb_regu; i++) {
		err = regulator_force_enable(cfg->regu[i]);
		if (err) {
			EMSG("[%s] regu force enable err: %d", cfg->regu[i]->name, err);
			return err;
		}
	}

	return 0;
}

/*
 * FIXME
 * All direct access to exti, pwr or IAC hadware block must be rework.
 * Wait the interrupt framework to enable or mask a specific interrupt
 */
#define IAC_BIT(_id) BIT(_id % 32)

#define LL_PWR_CPU_RESET   0x0U
#define LL_PWR_CPU_CRUN    0x1U
#define LL_PWR_CPU_CSLEEP  0x2U
#define LL_PWR_CPU_CSTOP   0x3U

#define LL_PWR_DRUN       0x0U
#define LL_PWR_DSTOP1     0x1U
#define LL_PWR_DSTOP2     0x2U
#define LL_PWR_DSTOP3     0x3U
#define LL_PWR_DSTANDBY   0x4U

#define PWR_CPU1D1SR_DSTATE_DSTANDBY	(LL_PWR_DSTANDBY << PWR_CPU1D1SR_DSTATE_Pos)
#define PWR_CPU1D1SR_CSTATE_CSTOP	(LL_PWR_CPU_CSTOP << PWR_CPU1D1SR_CSTATE_Pos)
#define PWR_CPU1D1SR_CSTATE_RESET	(LL_PWR_CPU_RESET << PWR_CPU1D1SR_CSTATE_Pos)

#define PWR_CPU1D1SR_D1_DSTANDBY	(PWR_CPU1D1SR_DSTATE_DSTANDBY | \
					 PWR_CPU1D1SR_CSTATE_RESET)

/* Waiting EXTI support with interrupt framework */
#define EXTI1_C2SEV	BIT(0)
#define EXTI1_C1SEV	BIT(1)

void stm32mp2_irq_ack_enable(uint32_t irq_ack)
{
	/* clear rising pending register C1SEV */
	EXTI1->RPR3 = EXTI1_C1SEV;
	EXTI1->RTSR3 |= EXTI1_C1SEV;
	/* unmask C2 int & event C1SEV (65) */
	EXTI1->C2IMR3 |= EXTI1_C1SEV;
	EXTI1->C2EMR3 |= EXTI1_C1SEV;

	if (irq_ack != IRQ_INVALID) {
		NVIC_ClearPendingIRQ(irq_ack);
		NVIC_EnableIRQ(irq_ack);
	}
}

void stm32mp2_irq_ack_disable(uint32_t irq_ack)
{
	/* mask c2 int & event C1SEV */
	EXTI1->C2IMR3 &= ~EXTI1_C1SEV;
	EXTI1->C2EMR3 &= ~EXTI1_C1SEV;

	/* clear rising pending register C1SEV */
	EXTI1->RPR3 = EXTI1_C1SEV;
	EXTI1->RTSR3 &= ~EXTI1_C1SEV;

	if (irq_ack != IRQ_INVALID) {
		NVIC_DisableIRQ(irq_ack);
		NVIC_ClearPendingIRQ(irq_ack);
	}
}

void stm32mp2_irq_ack_clear(uint32_t irq_ack)
{
	/* clear rising pending register C1SEV */
	EXTI1->RPR3 = EXTI1_C1SEV;
	EXTI1->RTSR3 &= ~EXTI1_C1SEV;
	if (irq_ack != IRQ_INVALID) {
		NVIC_ClearPendingIRQ(irq_ack);
	}
}

static __unused int stm32mp2_a35_set_boot_config(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	int i, ret = 0, err;
	struct firewall_spec *firewall;

	/* set bootrom access controller configuration */
	for_each_firewall(cfg->firewall_ctrls, firewall,
			  cfg->n_firewall_ctrls, i) {
		err = firewall_set_configuration(firewall);
		if (err != 0) {
			DMSG("[%s] fail to set firewall conf %d\n", __func__, i);
			return err;
		}
	}

#if defined(CONFIG_STM32MP25X_REVY)
	/*
	 * IAC workaround
	 * mask:
	 * RAMCFG(108), TAMP(152), PWR(155), RCC(156)
	 */
	IAC->IER[3] &= ~(IAC_BIT(108));
	IAC->IER[4] &= ~(IAC_BIT(152) | IAC_BIT(155) | IAC_BIT(156));
#endif

	return ret;
}

static __unused int stm32mp2_a35_restore(const struct device *dev)
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

#if defined(CONFIG_STM32MP25X_REVY)
	/*
	 * IAC workaround
	 * clear and unmask:
	 * RAMCFG(108), TAMP(152), PWR(155), RCC(156)
	 */
	IAC->ICR[3] = IAC_BIT(108);
	IAC->ICR[4] = (IAC_BIT(152) | IAC_BIT(155) | IAC_BIT(156));
	IAC->IER[3] |= IAC_BIT(108);
	IAC->IER[4] |= (IAC_BIT(152) | IAC_BIT(155) | IAC_BIT(156));
#endif

	/* restore clocks touched by bootrom */
	for (i = 0, clock_ctl = cfg->clk_ctl; i < cfg->n_clk; i++, clock_ctl++) {

		clk = clk_get(clock_ctl->dev, clock_ctl->subsys);
		if (clk)
			clk_restore_context(clk);
		else
			WMSG("%s: get clock[%d] fail\n", __func__, i);
	}

	return ret;
}

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
	int err_srm = 0;
	int err;

	/* when ack not received (not running), restore the configuration */
	if (stm32mp2_a35_is_running(dev))
		stm32_rproc_running_set(dev, false);
	else
		stm32mp2_a35_restore(dev);

	/* clear event if pending C2SEV */
	EXTI1->RPR3 = EXTI1_C2SEV;
	/* reset cpu */
	reset_control_assert(&cfg->rst_ctl);
	/* send CPU2 SEV event to cpu1 (exti 64)*/
	EXTI1->SWIER3 = EXTI1_C2SEV;
	/* Disable CPU1 interruption */
	if (cfg->irq_ack != IRQ_INVALID) {
		stm32mp2_irq_ack_disable(cfg->irq_ack);
	}
	/* check cpu in hold boot */
	err = mmio_read32_poll_timeout(((uint32_t)&PWR_S->CPU1D1SR), cfgr,
				       (cfgr & PWR_CPU1D1SR_HOLD_BOOT_Msk) &&
				       ((cfgr & PWR_CPU1D1SR_DSTATE)
					!= PWR_CPU1D1SR_DSTATE_DSTANDBY), 10);

	/* Reset Cortex-A35 resource */
	if (cfg->rproc_srm[0])
		err_srm = rproc_srm_reset(cfg->rproc_srm[0]);

	return err ? err : err_srm;
}

static __unused int stm32mp2_a35_init(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);

	stm32_rproc_running_set(dev, false);

	/* unmask event before rif has initialized CID1 filtering on EXTI1_C1CIDCFGR */
	EXTI1->C1IMR3 |= EXTI1_C2SEV;

	if (cfg->irq_ack != IRQ_INVALID){
		NVIC_SetPriority(cfg->irq_ack, 1);
	}

	return 0;
}

static __unused int stm32mp2_a35_start(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	struct stm32_rproc_data *data = dev_get_data(dev);
	uint32_t cfgr;
	int err;

	if (((PWR_S->CPU1D1SR & PWR_CPU1D1SR_DSTATE) == PWR_CPU1D1SR_DSTATE_DSTANDBY)
	    ||!(PWR_S->CPU1D1SR & PWR_CPU1D1SR_HOLD_BOOT_Msk)) {
		err = stm32mp2_a35_stop(dev);
		if (err)
			return err;
	}

	if (cfg->nb_regu) {
		err = stm32mp2_a35_regu(dev);
		if (err)
			return err;
	}

	/* Clear the ROM code context */
	if (data->stop2_nvmem_dev) {
		uint32_t addr = 0;
		const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(tamp));

		/* Grant access to CPU2 to TAMP backup registers Protection Zone 1-RIF1 */
		stm32_tamp_bkpreg_zone1_rif1(dev, true);
		/* Clear CA35 return address for stop2 modes in backup registers */
		err = nvmem_write_cell(data->stop2_nvmem_dev, sizeof(uint32_t), (uint8_t *)&addr);
		/* Restore access to CPU1 to TAMP backup registers Protection Zone 1-RIF1 */
		stm32_tamp_bkpreg_zone1_rif1(dev, false);
		if (err < 0)
			return err;
	}

	/* Prepare ROM code execution */
	err = stm32mp2_a35_set_boot_config(dev);
	if (err)
		return err;

	if (cfg->irq_ack != IRQ_INVALID) {
		data->restore_on_ack = true;
		stm32mp2_irq_ack_enable(cfg->irq_ack);
	} else {
		stm32_rproc_running_set(dev, true);
	}

	/*  power up cpu, in case it is in standby */
	err = reset_control_deassert(&cfg->rst_ctl);
	if (err)
		return err;

	/*  check cpu is not in holbot */
	return mmio_read32_poll_timeout(((uint32_t)&PWR_S->CPU1D1SR), cfgr,
					!(cfgr & PWR_CPU1D1SR_HOLD_BOOT_Msk) ||
					((cfgr & PWR_CPU1D1SR_DSTATE)
					 == PWR_CPU1D1SR_DSTATE_DSTANDBY), 10);

}

static __unused int stm32mp2_a35_release(const struct device *dev)
{
	int ret;

	ret = stm32mp2_a35_restore(dev);

	stm32_pwr_regulator_restore();

	/* Restore a35 debug configuration */
	stm32_bsec_restore_cortexa_debug_conf();

	stm32_rproc_running_set(dev, true);

	return ret;
}

static __unused void stm32mp2_a35_irq_ack(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	struct stm32_rproc_data *data = dev_get_data(dev);

	if (data->restore_on_ack) {
		stm32mp2_a35_release(dev);
		data->restore_on_ack = false;
	} else {
		/*
		 * Treat IRQ ack only when allowed.
		 * Waking up Cortex-A35 is not allowed between
		 * stm32mp2_a35_suspend() and stm32mp2_a35_resume().
		 */
		if (stm32mp2_a35_is_running(dev))
			/* Allow CPU1 wake-up form D1 DStandby with CPU2 SEV */
			EXTI1->SWIER3 = EXTI1_C2SEV;
	}
	stm32mp2_irq_ack_clear(cfg->irq_ack);
}

/* Suspend procedure before low power entry*/
static __unused int stm32mp2_a35_suspend(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	uint32_t cpu1d1sr;

	stm32mp2_irq_ack_enable(cfg->irq_ack);

	stm32_rproc_running_set(dev, false);

	/* Check that CPU1 low power state is D1 DStandby */
	cpu1d1sr = mmio_read_32((uint32_t)&PWR_S->CPU1D1SR);
	if (cpu1d1sr != PWR_CPU1D1SR_D1_DSTANDBY) {
		stm32_rproc_running_set(dev, true);

		/* Allow CPU1 wake-up with CPU2 SEV event (exti 64) */
		EXTI1->SWIER3 = EXTI1_C2SEV;
		stm32mp2_irq_ack_clear(cfg->irq_ack);

		return -EBUSY;
	}

	return 0;
}

/* Resume procedure after low power exit */
static __unused int stm32mp2_a35_resume(const struct device *dev)
{
	const struct stm32_rproc_config *cfg = dev_get_config(dev);
	struct stm32_rproc_data *data = dev_get_data(dev);
	int ret;

	/*
	 * For Standby exit, the CPU1 is in hold boot,set by HW and
	 * CPU2 need to reconfigure the RIF before ROM code execution
	 */
	if (!reset_control_status(&cfg->hold_boot)) {
		/* Prepare ROM code execution */
		ret = stm32mp2_a35_set_boot_config(dev);
		if (ret)
			return ret;
		data->restore_on_ack = true;
		stm32mp2_irq_ack_enable(cfg->irq_ack);
	} else {
		stm32_rproc_running_set(dev, true);
	}
	/* Allow CPU1 wake-up with CPU2 SEV event (exti 64) */
	EXTI1->SWIER3 = EXTI1_C2SEV;

	/* Remove HOLD BOOT */
	reset_control_assert(&cfg->hold_boot);

	return 0;
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

static bool stm32_rproc_is_running(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->is_running_fn)
		return true;

	return data->variant->is_running_fn(rproc->dev);
}

int stm32_rproc_stop(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->stop_fn)
		return -ENOTSUP;

	return data->variant->stop_fn(rproc->dev);
}

static int _stm32_rproc_set_rsc_tab(const struct device *dev,
				    uint32_t addr, uint32_t size)
{
	struct stm32_rproc_data *data = dev_get_data(dev);
	int err;

	err = nvmem_write_cell(data->rsc_tab_addr_dev, sizeof(uint32_t),
			       (uint8_t *)&addr);
	if (err < 0) {
		return err;
	}

	err = nvmem_write_cell(data->rsc_tab_size_dev, sizeof(uint32_t),
			       (uint8_t *)&size);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int stm32_rproc_set_rsc_tab(struct rproc_spec *rproc,
				   uint32_t addr, uint32_t size)
{
	return _stm32_rproc_set_rsc_tab(rproc->dev, addr, size);
}

int stm32_rproc_suspend(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->suspend_fn)
		return -ENOTSUP;

	return data->variant->suspend_fn(rproc->dev);
}

int stm32_rproc_resume(struct rproc_spec *rproc)
{
	struct stm32_rproc_data *data = dev_get_data(rproc->dev);

	if (!data->variant->resume_fn)
		return -ENOTSUP;

	return data->variant->resume_fn(rproc->dev);
}

static __unused int stm32_rproc_init(const struct device *dev)
{
	struct stm32_rproc_data *data = dev_get_data(dev);
	uint32_t err = 0;

	if (data->variant->init_fn)
		err = data->variant->init_fn(dev);

	data->stop2_nvmem_dev = DEVICE_DT_GET(DT_NODELABEL(stop2_entrypoint));

	/* reset resource table tamp back-up registers */
	_stm32_rproc_set_rsc_tab(dev, 0, 0);

	rproc_init(dev, &data->rproc);

	return err;
}

static __unused const struct  stm32_rproc_variant stm32mp2_a35_var = {
	.init_fn = stm32mp2_a35_init,
	.start_fn = stm32mp2_a35_start,
	.is_running_fn = stm32mp2_a35_is_running,
	.stop_fn = stm32mp2_a35_stop,
	.suspend_fn = stm32mp2_a35_suspend,
	.resume_fn = stm32mp2_a35_resume,
	.irq_handler = stm32mp2_a35_irq_ack,
};

static struct remoteproc_driver_api stm32_rproc_api = {
	.get_rproc = stm32_rproc_get,
	.start = stm32_rproc_start,
	.is_running = stm32_rproc_is_running,
	.stop = stm32_rproc_stop,
	.set_rsc_tab = stm32_rproc_set_rsc_tab,
	.suspend = stm32_rproc_suspend,
	.resume = stm32_rproc_resume,
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


#define DT_NUM_REGU(_inst)							\
	DT_INST_PROP_LEN_OR(_inst, regus, 0)

#define _DT_REGU(_idx, _inst)							\
	DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(_inst, regus, _idx))

#define DT_REGU(inst)								\
	{									\
		LISTIFY(DT_NUM_REGU(inst), _DT_REGU, (,), inst)			\
	}

#define DT_NUM_SRM(_inst)							\
	DT_INST_PROP_LEN_OR(_inst, srm, 0)

#define _DT_SRM(_idx, _inst)							\
	DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(_inst, SRM, _idx))

#define DT_SRM(inst)								\
	{									\
		LISTIFY(DT_NUM_SRM(inst), _DT_SRM, (,), inst)			\
	}

#define CHILD_COUNT_STEP(child)		+ 1

#define CHILD_COUNT(parent) \
	(0 DT_FOREACH_CHILD_STATUS_OKAY(parent, CHILD_COUNT_STEP))

#define MY_PARENT_NODE(n) DT_DRV_INST(n)
#define MY_PARENT_CHILD_DEV(child) DEVICE_DT_GET(child)

#define MY_PARENT_CHILDREN_ARRAY(n)				\
	static const struct device *my_parent_children_##n[] = {\
		DT_FOREACH_CHILD_STATUS_OKAY(			\
			MY_PARENT_NODE(n),			\
			MY_PARENT_CHILD_DEV)			\
	}

#define STM32_RPROC_INIT(n, name, _variant, _irqhandler)			\
BUILD_ASSERT(CHILD_COUNT(MY_PARENT_NODE(n)) <= 1,				\
		     "unsupported: too many children under this rproc");	\
										\
DT_INST_ACCESS_CTRLS_DEFINE(n);							\
										\
static const struct clock_control clk_ctrl_##n[] = DT_INST_CLOCK_CONTROL(n);	\
										\
static const struct device *regu_##n[] = DT_REGU(n);				\
										\
MY_PARENT_CHILDREN_ARRAY(n);							\
										\
static const struct stm32_rproc_config _##name##_cfg##n = {			\
	.rst_ctl = DT_INST_RESET_CONTROL_GET_BY_IDX(n, 0),			\
	.hold_boot = DT_INST_RESET_CONTROL_GET_BY_IDX(n, 1),			\
	.firewall_ctrls = DT_INST_ACCESS_CTRLS_GET(n),				\
	.n_firewall_ctrls = DT_INST_ACCESS_CTRLS_NUM(n),			\
	.irq_ack = DT_INST_IRQ_BY_NAME_OR(n, ack, irq),				\
	.clk_ctl = clk_ctrl_##n,						\
	.n_clk = DT_INST_NUM_CLOCKS(n),						\
	.regu =  regu_##n,							\
	.nb_regu = DT_NUM_REGU(n),						\
	.rproc_srm =   my_parent_children_##n,					\
	.nb_rproc_srm = ARRAY_SIZE(my_parent_children_##n),			\
};										\
										\
static struct stm32_rproc_data _##name##_data##n = {				\
	.variant = &_variant,							\
	.rsc_tab_addr_dev = DT_INST_DEV_NVMEM(n, rsc_tab_addr),		\
	.rsc_tab_size_dev = DT_INST_DEV_NVMEM(n, rsc_tab_size),		\
};										\
										\
void _irqhandler(void)								\
{										\
	if (_variant.irq_handler)						\
		_variant.irq_handler(DEVICE_DT_INST_GET(n));			\
}										\
										\
DEVICE_DT_INST_DEFINE(n, &stm32_rproc_init, NULL,				\
		      &_##name##_data##n, &_##name##_cfg##n,			\
		      CORE, 9, &stm32_rproc_api);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp2_a35

DT_INST_FOREACH_STATUS_OKAY_VARGS(STM32_RPROC_INIT, DT_DRV_COMPAT,
				  stm32mp2_a35_var, CPU1_SEV_IRQHandler)
