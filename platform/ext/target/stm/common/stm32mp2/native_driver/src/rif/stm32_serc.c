// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifdef TFM_ENV
#include <cmsis.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <stm32_serc.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#include <clk.h>
#include <target_cfg.h>
#else
/* optee */
#include <drivers/stm32_serc.h>
#include <io.h>
#include <kernel/dt.h>
#include <kernel/boot.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <tee_api_defines.h>
#include <trace.h>
#include <util.h>
#endif

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

#define _SERC_FLD_PREP(field, value)	(((uint32_t)(value) << (field ## _SHIFT)) & (field ## _MASK))
#define _SERC_FLD_GET(field, value)	(((uint32_t)(value) & (field ## _MASK)) >> (field ## _SHIFT))

/*
 * no common errno between component
 * define serc internal errno
 */
#define	SERC_ERR_NOMEM		12	/* Out of memory */
#define SERC_ERR_NODEV		19	/* No such device */
#define SERC_ERR_INVAL		22	/* Invalid argument */
#define SERC_ERR_NOTSUP		45	/* Operation not supported */

static struct serc_driver_data serc_drvdata;
static struct stm32_serc_platdata serc_pdata;

static void stm32_serc_get_driverdata(struct stm32_serc_platdata *pdata)
{
	struct clk *clk = clk_get(STM32_DEV_RCC, (clk_subsys_t) pdata->clk_id);
	uint32_t regval = 0;

	clk_enable(clk);

	regval = io_read32(pdata->base + _SERC_HWCFGR);
	serc_drvdata.num_ilac = _SERC_FLD_GET(_SERC_HWCFGR_CFG1, regval);

	pdata->drv_data = &serc_drvdata;

	regval = io_read32(pdata->base + _SERC_VERR);

	DMSG("SERC version %"PRIu32".%"PRIu32,
	     _SERC_FLD_GET(_SERC_VERR_MAJREV, regval),
	     _SERC_FLD_GET(_SERC_VERR_MINREV, regval));

	DMSG("HW cap: num ilac:[%"PRIu8"]", serc_drvdata.num_ilac);

	clk_disable(clk);
}

#ifdef CFG_DT
static int stm32_serc_parse_fdt(struct stm32_serc_platdata *pdata)
{
	return -SERC_ERR_NODEV;
}

__weak int stm32_rifsc_get_platdata(struct stm32_rifsc_platdata *pdata __unused)
{
	/* In DT config, the platform datas are fill by DT file */
	return 0;
}

#else
static int stm32_serc_parse_fdt(struct stm32_serc_platdata *pdata)
{
	return -SERC_ERR_NOTSUP;
}

/*
 * This function could be overridden by platform to define
 * pdata of serc driver
 */
__weak int stm32_serc_get_platdata(struct stm32_serc_platdata *pdata)
{
	return -SERC_ERR_NODEV;
}
#endif /*CFG_DT*/

#define SERC_EXCEPT_MSB_BIT(x) (x * _PERIPH_IDS_PER_REG + _PERIPH_IDS_PER_REG - 1)
#define SERC_EXCEPT_LSB_BIT(x) (x * _PERIPH_IDS_PER_REG)

__weak void access_violation_handler(void)
{
	EMSG("Ooops... %s");
	while (1) {
		;
	}
}

void SERF_IRQHandler(void)
{
	int nreg = div_round_up(serc_drvdata.num_ilac, _PERIPH_IDS_PER_REG);
	uint32_t isr = 0;
	int i = 0;

	for (i = 0; i < nreg; i++) {
		uint32_t offset = sizeof(uint32_t) * i;

		isr = io_read32(serc_pdata.base + _SERC_ISR0 + offset);
		isr &= io_read32(serc_pdata.base + _SERC_IER0 + offset);
		if (isr) {
			EMSG("serc exceptions: [%d:%d]=%#08x",
			     SERC_EXCEPT_MSB_BIT(i),
			     SERC_EXCEPT_LSB_BIT(i), isr);
			io_write32(serc_pdata.base + _SERC_ICR0 + offset, isr);
		}
	}

	NVIC_ClearPendingIRQ(SERF_IRQn);

	access_violation_handler();
}

static void stm32_serc_setup(struct stm32_serc_platdata *pdata)
{
	struct clk *clk = clk_get(STM32_DEV_RCC, (clk_subsys_t) pdata->clk_id);
	struct serc_driver_data *drv_data = pdata->drv_data;
	int nreg = div_round_up(drv_data->num_ilac, _PERIPH_IDS_PER_REG);
	int i = 0;

	clk_enable(clk);
	mmio_setbits_32(pdata->base + _SERC_ENABLE, _SERC_ENABLE_SERFEN);

	for (i = 0; i < nreg; i++) {
		uint32_t reg_ofst = pdata->base + sizeof(uint32_t) * i;

		//clear status flags
		io_write32(reg_ofst + _SERC_ICR0, 0x0);

		if (i < pdata->nmask)
			io_write32(reg_ofst + _SERC_IER0, pdata->it_mask[i]);
	}
}

int stm32_serc_enable_irq(void)
{
	if (serc_pdata.base == 0)
		return -SERC_ERR_NODEV;

	/* just less than exception fault */
	NVIC_SetPriority(serc_pdata.irq, 1);
	NVIC_EnableIRQ(serc_pdata.irq);
}

int stm32_serc_init(void)
{
	int err = 0;

	err = stm32_serc_get_platdata(&serc_pdata);
	if (err)
		return err;

	err = stm32_serc_parse_fdt(&serc_pdata);
	if (err && err != -SERC_ERR_NOTSUP)
		return err;

	if (!serc_pdata.drv_data)
		stm32_serc_get_driverdata(&serc_pdata);

	stm32_serc_setup(&serc_pdata);

	return 0;
}

#ifndef TFM_ENV
static TEE_Result serc_init(void)
{
	if (stm32_serc_init()) {
		panic();
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}
early_init(serc_init);
#endif
