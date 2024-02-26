// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, STMicroelectronics
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 */
#define DT_DRV_COMPAT st_stm32mp25_serc

#include <cmsis.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <lib/utils_def.h>
#include <stm32_serc.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <clk.h>
#include <target_cfg.h>
#include <tfm_hal_spm_logdev.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* SERC offset register */
#define _SERC_IER0		U(0x000)
#define _SERC_ISR0		U(0x040)
#define _SERC_ICR0		U(0x080)
#define _SERC_ENABLE		U(0x100)

#define _SERC_HWCFGR		U(0x3F0)
#define _SERC_VERR		U(0x3F4)

/* SERC_ENABLE register fields */
#define _SERC_ENABLE_SERFEN	BIT(0)

/* SERC_HWCFGR register fields */
#define _SERC_HWCFGR_CFG1_MASK	GENMASK_32(7, 0)
#define _SERC_HWCFGR_CFG1_SHIFT	0
#define _SERC_HWCFGR_CFG2_MASK	GENMASK_32(18, 16)
#define _SERC_HWCFGR_CFG2_SHIFT	16

/* SERC_VERR register fields */
#define _SERC_VERR_MINREV_MASK	GENMASK_32(3, 0)
#define _SERC_VERR_MINREV_SHIFT	0
#define _SERC_VERR_MAJREV_MASK	GENMASK_32(7, 4)
#define _SERC_VERR_MAJREV_SHIFT	4

/* Periph id per register */
#define _PERIPH_IDS_PER_REG	32

struct stm32_serc_config {
	uintptr_t base;
	const struct device *clk_dev;
	const clk_subsys_t clk_subsys;
	uint32_t irq;
	uint32_t id_disable[DT_INST_PROP_LEN_OR(0, id_disable, 0)];
};

struct stm32_serc_data {
	uint8_t num_ilac;
};

static const struct stm32_serc_config serc_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clk_subsys = (clk_subsys_t) DT_INST_CLOCKS_CELL(0, bits),
	.irq = DT_INST_IRQN(0),
	.id_disable = DT_INST_PROP_OR(0, id_disable, {}),
};

static struct stm32_serc_data serc_data = {};

static void stm32_serc_get_hwconfig(void)
{
	const struct stm32_serc_config *drv_cfg = &serc_cfg;
	struct stm32_serc_data *drv_data = &serc_data;
	uint32_t regval;

	regval = io_read32(drv_cfg->base + _SERC_HWCFGR);
	drv_data->num_ilac = _FLD_GET(_SERC_HWCFGR_CFG1, regval);

	regval = io_read32(drv_cfg->base + _SERC_VERR);

	DMSG("SERC version %"PRIu32".%"PRIu32,
	     _FLD_GET(_SERC_VERR_MAJREV, regval),
	     _FLD_GET(_SERC_VERR_MINREV, regval));

	DMSG("HW cap: num ilac:[%"PRIu8"]", drv_data->num_ilac);
}

#define SERC_EXCEPT_MSB_BIT(x) (x * _PERIPH_IDS_PER_REG + _PERIPH_IDS_PER_REG - 1)
#define SERC_EXCEPT_LSB_BIT(x) (x * _PERIPH_IDS_PER_REG)

#define SERC_LOG(x) tfm_hal_output_spm_log((x), sizeof(x))

__weak void access_violation_handler(void)
{
	SERC_LOG("Ooops...\n\r");
	while (1) {
		;
	}
}

void SERF_IRQHandler(void)
{
	const struct stm32_serc_config *drv_cfg = &serc_cfg;
	struct stm32_serc_data *drv_data = &serc_data;
	int nreg = div_round_up(drv_data->num_ilac, _PERIPH_IDS_PER_REG);
	uint32_t isr = 0;
	char tmp[50];
	int i = 0;

	for (i = 0; i < nreg; i++) {
		uint32_t offset = sizeof(uint32_t) * i;

		isr = io_read32(drv_cfg->base + _SERC_ISR0 + offset);
		isr &= io_read32(drv_cfg->base + _SERC_IER0 + offset);
		if (isr) {
			snprintf(tmp, sizeof(tmp),
				 "\r\nserc exceptions: [%d:%d]=%#08x\r\n",
				 SERC_EXCEPT_MSB_BIT(i),
				 SERC_EXCEPT_LSB_BIT(i), isr);

			tfm_hal_output_spm_log(tmp, strlen(tmp));
			io_write32(drv_cfg->base + _SERC_ICR0 + offset, isr);
		}
	}

	NVIC_ClearPendingIRQ(drv_cfg->irq);

	access_violation_handler();
}

static void stm32_serc_setup(void)
{
	const struct stm32_serc_config *drv_cfg = &serc_cfg;
	struct stm32_serc_data *drv_data = &serc_data;
	int nreg = div_round_up(drv_data->num_ilac, _PERIPH_IDS_PER_REG);
	int i = 0;

	mmio_setbits_32(drv_cfg->base + _SERC_ENABLE, _SERC_ENABLE_SERFEN);

	for (i = 0; i < nreg; i++) {
		uint32_t reg_ofst = drv_cfg->base + sizeof(uint32_t) * i;

		//clear status flags
		io_write32(reg_ofst + _SERC_ICR0, 0x0);
		//enable all peripherals of nreg
		io_write32(reg_ofst + _SERC_IER0, ~0x0);
	}
}

int stm32_serc_enable_irq(void)
{
	const struct stm32_serc_config *drv_cfg = &serc_cfg;

	if (drv_cfg->base == 0)
		return -ENODEV;

	/* just less than exception fault */
	NVIC_SetPriority(drv_cfg->irq, 1);
	NVIC_EnableIRQ(drv_cfg->irq);
}

/*FIXME just a workaround for poc */
void stm32_serc_id_disable(void)
{
	const struct stm32_serc_config *drv_cfg = &serc_cfg;
	int i;

	for (i = 0; i < ARRAY_SIZE(drv_cfg->id_disable); i++) {
		uint32_t reg_ofst = (drv_cfg->id_disable[i] / _PERIPH_IDS_PER_REG) * sizeof(uint32_t);
		uint32_t bit_ofst = (drv_cfg->id_disable[i]) & 0x1F;

		io_clrbits32(drv_cfg->base + _SERC_IER0 + reg_ofst,
			     BIT(bit_ofst));
	}
}

static int stm32_serc_init(void)
{
	const struct stm32_serc_config *drv_cfg = &serc_cfg;
	struct clk *clk;
	int err;

	clk = clk_get(drv_cfg->clk_dev, drv_cfg->clk_subsys);
	if (!clk)
		return -ENODEV;

	err = clk_enable(clk);
	if (err)
		return err;

	stm32_serc_get_hwconfig();
	stm32_serc_setup();

	stm32_serc_id_disable();

	return 0;
}

SYS_INIT(stm32_serc_init, PRE_CORE, 15);
#endif
