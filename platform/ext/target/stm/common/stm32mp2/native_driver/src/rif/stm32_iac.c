// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifdef TFM_ENV
#include <cmsis.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include <stm32_iac.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <debug.h>
#else
/* optee */
#include <drivers/stm32_iac.h>
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

static struct iac_driver_data iac_drvdata;
static struct stm32_iac_platdata iac_pdata;

static void stm32_iac_get_driverdata(struct stm32_iac_platdata *pdata)
{
	uint32_t regval = 0;

	regval = io_read32(pdata->base + _IAC_HWCFGR1);
	iac_drvdata.num_ilac = _IAC_FLD_GET(_IAC_HWCFGR1_CFG5, regval);
	iac_drvdata.rif_en = _IAC_FLD_GET(_IAC_HWCFGR1_CFG1, regval) != 0;
	iac_drvdata.sec_en = _IAC_FLD_GET(_IAC_HWCFGR1_CFG2, regval) != 0;
	iac_drvdata.priv_en = _IAC_FLD_GET(_IAC_HWCFGR1_CFG3, regval) != 0;

	pdata->drv_data = &iac_drvdata;

	regval = io_read32(pdata->base + _IAC_VERR);

	DMSG("IAC version %"PRIu32".%"PRIu32,
	     _IAC_FLD_GET(_IAC_VERR_MAJREV, regval),
	     _IAC_FLD_GET(_IAC_VERR_MINREV, regval));

	DMSG("HW cap: enabled[rif:sec:priv]:[%s:%s:%s] num ilac:[%"PRIu8"]",
	     iac_drvdata.rif_en ? "true" : "false",
	     iac_drvdata.sec_en ? "true" : "false",
	     iac_drvdata.priv_en ? "true" : "false",
	     iac_drvdata.num_ilac);
}

#ifdef CFG_DT
static int stm32_iac_parse_fdt(struct stm32_iac_platdata *pdata)
{
	return -IAC_ERR_NODEV;
}

__weak int stm32_iac_get_platdata(struct stm32_iac_platdata *pdata __unused)
{
	/* In DT config, the platform datas are fill by DT file */
	return 0;
}

#else
static int stm32_iac_parse_fdt(struct stm32_iac_platdata *pdata)
{
	return -IAC_ERR_NOTSUP;
}

/*
 * This function could be overridden by platform to define
 * pdata of iac driver
 */
__weak int stm32_iac_get_platdata(struct stm32_iac_platdata *pdata)
{
	return -IAC_ERR_NODEV;
}
#endif /*CFG_DT*/

#define IAC_EXCEPT_MSB_BIT(x) (x * _PERIPH_IDS_PER_REG + _PERIPH_IDS_PER_REG - 1)
#define IAC_EXCEPT_LSB_BIT(x) (x * _PERIPH_IDS_PER_REG)

__weak void access_violation_handler(void)
{
	EMSG("Ooops... %s");
	while (1) {
		;
	}
}

void IAC_IRQHandler(void)
{
	int nreg = div_round_up(iac_drvdata.num_ilac, _PERIPH_IDS_PER_REG);
	uint32_t isr = 0;
	int i = 0;

	for (i = 0; i < nreg; i++) {
		uint32_t offset = sizeof(uint32_t) * i;

		isr = io_read32(iac_pdata.base + _IAC_ISR0 + offset);
		isr &= io_read32(iac_pdata.base + _IAC_IER0 + offset);
		if (isr) {
			EMSG("iac exceptions: [%d:%d]=%#08x",
			     IAC_EXCEPT_MSB_BIT(i),
			     IAC_EXCEPT_LSB_BIT(i), isr);
			io_write32(iac_pdata.base + _IAC_ICR0 + offset, isr);
		}
	}

	NVIC_ClearPendingIRQ(IAC_IRQn);

	access_violation_handler();
}

static void stm32_iac_setup(struct stm32_iac_platdata *pdata)
{
	struct iac_driver_data *drv_data = pdata->drv_data;
	int nreg = div_round_up(drv_data->num_ilac, _PERIPH_IDS_PER_REG);
	int i = 0;

	for (i = 0; i < nreg; i++) {
		uint32_t reg_ofst = pdata->base + sizeof(uint32_t) * i;

		//clear status flags
		io_write32(reg_ofst + _IAC_ICR0, UINT32_MAX);

		//set configuration
		if (i < pdata->nmask)
			io_write32(reg_ofst + _IAC_IER0, pdata->it_mask[i]);
	}
}

int stm32_iac_enable_irq(void)
{
	if (iac_pdata.base == 0)
		return -IAC_ERR_NODEV;

	/* just less than exception fault */
	NVIC_SetPriority(iac_pdata.irq, 1);
	NVIC_EnableIRQ(iac_pdata.irq);
}

int stm32_iac_init(void)
{
	int err = 0;

	err = stm32_iac_get_platdata(&iac_pdata);
	if (err)
		return err;

	err = stm32_iac_parse_fdt(&iac_pdata);
	if (err && err != -IAC_ERR_NOTSUP)
		return err;

	if (!iac_pdata.drv_data)
		stm32_iac_get_driverdata(&iac_pdata);

	stm32_iac_setup(&iac_pdata);

	return 0;
}

#ifndef TFM_ENV
static TEE_Result iac_init(void)
{
	if (stm32_iac_init()) {
		panic();
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}
early_init(iac_init);
#endif
