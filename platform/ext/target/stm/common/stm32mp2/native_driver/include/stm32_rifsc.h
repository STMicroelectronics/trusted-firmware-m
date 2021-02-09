/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_RIFSC_H
#define __STM32_RIFSC_H

#ifndef TFM_ENV
/* optee */
#include <types_ext.h>
#endif

struct risup_cfg {
	uint32_t id;
	bool sec;
	bool priv;
	uint32_t cid_attr;
};

struct rimu_cfg {
	uint32_t id;
	uint32_t attr;
};

struct rifsc_driver_data {
	uint32_t version;
	uint8_t nb_rimu;
	uint8_t nb_risup;
	uint8_t nb_risal;
	bool rif_en;
	bool sec_en;
	bool priv_en;
};

struct stm32_rifsc_platdata {
	uintptr_t base;
	struct rifsc_driver_data *drv_data;
	struct risup_cfg *risup;
	int nrisup;
	struct rimu_cfg *rimu;
	int nrimu;
};

int stm32_rifsc_get_platdata(struct stm32_rifsc_platdata *pdata);
int stm32_rifsc_init(void);
#endif /* __STM32_RIFSC_H */
