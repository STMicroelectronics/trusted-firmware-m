/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_IAC_H
#define __STM32_IAC_H

#include <stdbool.h>

#ifndef TFM_ENV
/* optee */
#include <types_ext.h>
#endif

struct iac_cfg {
	uint32_t *it_mask;
	uint32_t n_mask;
};

struct iac_driver_data {
	uint32_t version;
	uint8_t num_ilac;
	bool rif_en;
	bool sec_en;
	bool priv_en;
};

struct stm32_iac_platdata {
	uintptr_t base;
	struct iac_driver_data *drv_data;
	uint32_t *it_mask;
	uint32_t nmask;
	int irq;
};

int stm32_iac_enable_irq(void);
int stm32_iac_get_platdata(struct stm32_iac_platdata *pdata);
int stm32_iac_init(void);
#endif /* __STM32_IAC_H */
