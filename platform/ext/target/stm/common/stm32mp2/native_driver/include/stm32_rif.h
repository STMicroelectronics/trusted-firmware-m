/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_RISAF_H
#define STM32_RISAF_H

#include <stddef.h>
#include <lib/utils_def.h>

#define RISAF_REG(_id, _cfg, _cid_cfg, _start_addr, _end_addr)		\
{									\
	.id = _id,							\
	.cfg = _cfg,							\
	.cid_cfg = _cid_cfg,						\
	.start_addr= _start_addr,					\
	.end_addr = _end_addr,						\
}

#define RISAF_CFG(_instance, _max_region, _base, _regions, _clk_id)	\
{									\
	.base = _base,							\
	.instance = _instance,						\
	.clock_id = _clk_id,						\
	.max_region = _max_region,					\
	.regions = _regions,						\
	.nregions = ARRAY_SIZE(_regions),				\
}

struct risaf_region_cfg {
	uint32_t id;
	uint32_t cfg;
	uint32_t cid_cfg;
	uint32_t start_addr;
	uint32_t end_addr;
};

struct risaf_cfg {
	uintptr_t base;
	uint32_t instance;
	unsigned long clock_id;
	int max_region;
	const struct risaf_region_cfg *regions;
	int nregions;
};

struct stm32_rif_platdata {
	const struct risaf_cfg *risafs;
	int nrisafs;
};

/* _RISAF_CFGR flags */
#define _RISAF_CFGR_BREN		BIT(0)
#define _RISAF_CFGR_SEC			BIT(8)
#define _RISAF_CFGR_ENC			BIT(15)
#define _RISAF_CFGR_PRIVC_MASK		GENMASK(23, 16)
#define _RISAF_CFGR_PRIVC(_cid)		(BIT(16 + _cid) & _RISAF_CFGR_PRIVC_MASK)

#define _RISAF_CIDCFGR_RDEN_MASK	GENMASK(7, 0)
#define _RISAF_CIDCFGR_RDEN(_cid)	(BIT(_cid) & _RISAF_CIDCFGR_RDEN_MASK)
#define _RISAF_CIDCFGR_WREN_MASK	GENMASK(23, 16)
#define _RISAF_CIDCFGR_WREN(_cid)	(BIT(16 + _cid) & _RISAF_CIDCFGR_WREN_MASK)

int stm32_rif_init(void);
#endif /* STM32_RISAF_H */
