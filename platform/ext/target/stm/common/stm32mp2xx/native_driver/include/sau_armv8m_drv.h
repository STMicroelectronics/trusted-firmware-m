/*
 * Copyright (C) 2022, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __SAU_ARMV8M_DRV_H__
#define __SAU_ARMV8M_DRV_H__

#include <stdint.h>

enum sau_armv8m_attr_t {
    SAU_DIS,
    SAU_EN,
    SAU_NSC,
};

struct sau_region_cfg_t {
	uint32_t base;
	uint32_t limit;
	enum sau_armv8m_attr_t attr;
};

#define for_each_sau_region(rgt, rg, nr, idx)		\
	for (rg = ((struct sau_region_cfg_t *)rgt);	\
	     idx >= 0 && idx < (nr);			\
	     idx++, rg = rg + 1)

#define SAU_REGION_CFG(_base, _limit, _attr)	\
	{					\
		.base = _base,			\
		.limit = _limit,		\
		.attr = _attr,			\
	}

struct sau_platdata {
	uint32_t base;

	const struct sau_region_cfg_t *sau_region;
	int nr_region;
};

int sau_get_platdata(struct sau_platdata *pdata);
int sau_init(void);

#endif /* __SAU_ARMV8M_DRV_H__ */
