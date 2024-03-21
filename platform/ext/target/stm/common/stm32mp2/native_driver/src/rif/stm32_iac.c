/*
 * Copyright (c) 2023, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@foss.st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_iac

#include <cmsis.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <device.h>
#include <lib/utils_def.h>
#include <stm32_iac.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <uart_stdout.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* IAC offset register */
#define _IAC_IER0		U(0x000)
#define _IAC_ISR0		U(0x080)
#define _IAC_ICR0		U(0x100)
#define _IAC_IISR0		U(0x36C)

#define _IAC_HWCFGR2		U(0x3EC)
#define _IAC_HWCFGR1		U(0x3F0)
#define _IAC_VERR		U(0x3F4)

/* IAC_HWCFGR2 register fields */
#define _IAC_HWCFGR2_CFG1_MASK	GENMASK_32(3, 0)
#define _IAC_HWCFGR2_CFG1_SHIFT	0
#define _IAC_HWCFGR2_CFG2_MASK	GENMASK_32(7, 4)
#define _IAC_HWCFGR2_CFG2_SHIFT	4

/* IAC_HWCFGR1 register fields */
#define _IAC_HWCFGR1_CFG1_MASK	GENMASK_32(3, 0)
#define _IAC_HWCFGR1_CFG1_SHIFT	0
#define _IAC_HWCFGR1_CFG2_MASK	GENMASK_32(7, 4)
#define _IAC_HWCFGR1_CFG2_SHIFT	4
#define _IAC_HWCFGR1_CFG3_MASK	GENMASK_32(11, 8)
#define _IAC_HWCFGR1_CFG3_SHIFT	8
#define _IAC_HWCFGR1_CFG4_MASK	GENMASK_32(15, 12)
#define _IAC_HWCFGR1_CFG4_SHIFT	12
#define _IAC_HWCFGR1_CFG5_MASK	GENMASK_32(24, 16)
#define _IAC_HWCFGR1_CFG5_SHIFT	16

/* IAC_VERR register fields */
#define _IAC_VERR_MINREV_MASK	GENMASK_32(3, 0)
#define _IAC_VERR_MINREV_SHIFT	0
#define _IAC_VERR_MAJREV_MASK	GENMASK_32(7, 4)
#define _IAC_VERR_MAJREV_SHIFT	4

/* Periph id per register */
#define _PERIPH_IDS_PER_REG	32

#define _IAC_FLD_PREP(field, value)	(((uint32_t)(value) << (field ## _SHIFT)) & (field ## _MASK))
#define _IAC_FLD_GET(field, value)	(((uint32_t)(value) & (field ## _MASK)) >> (field ## _SHIFT))

/*
 * no common errno between component
 * define iac internal errno
 */
#define	IAC_ERR_NOMEM		12	/* Out of memory */
#define IAC_ERR_NODEV		19	/* No such device */
#define IAC_ERR_INVAL		22	/* Invalid argument */
#define IAC_ERR_NOTSUP		45	/* Operation not supported */

struct stm32_iac_config {
	uintptr_t base;
	uint32_t irq;
	uint32_t id_disable[DT_INST_PROP_LEN_OR(0, id_disable, 0)];
};

struct stm32_iac_data {
	uint8_t num_ilac;
	bool rif_en;
	bool sec_en;
	bool priv_en;
};

static const struct stm32_iac_config iac_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.irq = DT_INST_IRQN(0),
	.id_disable = DT_INST_PROP_OR(0, id_disable, {}),
};

static struct stm32_iac_data iac_data = {};

static void stm32_iac_get_hwconfig(void)
{
	const struct stm32_iac_config *drv_cfg = &iac_cfg;
	struct stm32_iac_data *drv_data = &iac_data;
	uint32_t regval;

	regval = io_read32(drv_cfg->base + _IAC_HWCFGR1);
	drv_data->num_ilac = _IAC_FLD_GET(_IAC_HWCFGR1_CFG5, regval);
	drv_data->rif_en = _IAC_FLD_GET(_IAC_HWCFGR1_CFG1, regval) != 0;
	drv_data->sec_en = _IAC_FLD_GET(_IAC_HWCFGR1_CFG2, regval) != 0;
	drv_data->priv_en = _IAC_FLD_GET(_IAC_HWCFGR1_CFG3, regval) != 0;

	regval = io_read32(drv_cfg->base + _IAC_VERR);

	DMSG("IAC version %"PRIu32".%"PRIu32,
	     _IAC_FLD_GET(_IAC_VERR_MAJREV, regval),
	     _IAC_FLD_GET(_IAC_VERR_MINREV, regval));

	DMSG("HW cap: enabled[rif:sec:priv]:[%s:%s:%s] num ilac:[%"PRIu8"]",
	     drv_data->rif_en ? "true" : "false",
	     drv_data->sec_en ? "true" : "false",
	     drv_data->priv_en ? "true" : "false",
	     drv_data->num_ilac);
}

#define IAC_EXCEPT_MSB_BIT(x) (x * _PERIPH_IDS_PER_REG + _PERIPH_IDS_PER_REG - 1)
#define IAC_EXCEPT_LSB_BIT(x) (x * _PERIPH_IDS_PER_REG)

#define IAC_LOG(str) do { \
    stdio_output_string((const unsigned char *)str, strlen(str)); \
} while (0);

__weak void access_violation_handler(void)
{
	IAC_LOG("Ooops...\n\r");
	while (1) {
		;
	}
}

void IAC_IRQHandler(void)
{
	const struct stm32_iac_config *drv_cfg = &iac_cfg;
	struct stm32_iac_data *drv_data = &iac_data;
	int nreg = div_round_up(drv_data->num_ilac, _PERIPH_IDS_PER_REG);
	uint32_t isr = 0;
	char tmp[50];
	int i = 0;

	for (i = 0; i < nreg; i++) {
		uint32_t offset = sizeof(uint32_t) * i;

		isr = io_read32(drv_cfg->base + _IAC_ISR0 + offset);
		isr &= io_read32(drv_cfg->base + _IAC_IER0 + offset);
		if (isr) {
			snprintf(tmp, sizeof(tmp),
				 "\r\niac exceptions: [%d:%d]=%#08x\r\n",
				 IAC_EXCEPT_MSB_BIT(i),
				 IAC_EXCEPT_LSB_BIT(i), isr);

			IAC_LOG(tmp);
			io_write32(drv_cfg->base + _IAC_ICR0 + offset, isr);
		}
	}

	NVIC_ClearPendingIRQ(drv_cfg->irq);

	access_violation_handler();
}

static void stm32_iac_setup(void)
{
	const struct stm32_iac_config *drv_cfg = &iac_cfg;
	struct stm32_iac_data *drv_data = &iac_data;
	int nreg = div_round_up(drv_data->num_ilac, _PERIPH_IDS_PER_REG);
	int i = 0;

	for (i = 0; i < nreg; i++) {
		uint32_t reg_ofst = drv_cfg->base + sizeof(uint32_t) * i;

		//clear status flags
		io_write32(reg_ofst + _IAC_ICR0, UINT32_MAX);
		//enable all peripherals of nreg
		io_write32(reg_ofst + _IAC_IER0, ~0x0);
	}
}

int stm32_iac_enable_irq(void)
{
	const struct stm32_iac_config *drv_cfg = &iac_cfg;

	if (drv_cfg->base == 0)
		return -ENODEV;

	/* just less than exception fault */
	NVIC_SetPriority(drv_cfg->irq, 1);
	NVIC_EnableIRQ(drv_cfg->irq);
}

/*FIXME just a workaround for poc */
void stm32_iac_id_disable(void)
{
	const struct stm32_iac_config *drv_cfg = &iac_cfg;
	int i;

	for (i = 0; i < ARRAY_SIZE(drv_cfg->id_disable); i++) {
		uint32_t reg_ofst = (drv_cfg->id_disable[i] / _PERIPH_IDS_PER_REG) * sizeof(uint32_t);
		uint32_t bit_ofst = (drv_cfg->id_disable[i]) & 0x1F;

		io_clrbits32(drv_cfg->base + _IAC_IER0 + reg_ofst,
			     BIT(bit_ofst));
	}
}

static int stm32_iac_init(void)
{
	stm32_iac_get_hwconfig();
	stm32_iac_setup();

	stm32_iac_id_disable();

	return 0;
}

SYS_INIT(stm32_iac_init, PRE_CORE, 15);
#endif
