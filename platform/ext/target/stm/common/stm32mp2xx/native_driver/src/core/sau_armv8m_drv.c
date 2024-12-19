// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022, STMicroelectronics
 */
#include <cmsis.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <lib/utils_def.h>
#include <sau_armv8m_drv.h>

static struct sau_platdata sau_pdata;

/*
 * This function could be overridden by platform to define
 * pdata of sau driver
 */
__weak int sau_get_platdata(struct sau_platdata *pdata)
{
	return -ENODEV;
}

static void sau_region_cfg(void)
{
	SAU_Type *sau = (SAU_Type *)sau_pdata.base;
	struct sau_region_cfg_t *rg;
	int32_t idx = 0;

	/* Disable SAU */
	TZ_SAU_Disable();

	for_each_sau_region(sau_pdata.sau_region, rg, sau_pdata.nr_region, idx) {
		sau->RNR = idx;
		sau->RBAR = rg->base & SAU_RBAR_BADDR_Msk;
		sau->RLAR = (rg->limit & SAU_RLAR_LADDR_Msk) | rg->attr;
	}

	/* Force memory writes before continuing */
	__DSB();
	/* Flush and refill pipeline with updated permissions */
	__ISB();

	/* Enable SAU */
	TZ_SAU_Enable();
}

static int sau_get_nb_region(void)
{
	SAU_Type *sau = (SAU_Type *)sau_pdata.base;

	return sau->TYPE & SAU_TYPE_SREGION_Msk;
}

int sau_init(void)
{
	int err;

	err = sau_get_platdata(&sau_pdata);
	if (err)
		return err;

	if (sau_pdata.nr_region > sau_get_nb_region())
		return -ENOTSUP;

	sau_region_cfg();

	return 0;
}
