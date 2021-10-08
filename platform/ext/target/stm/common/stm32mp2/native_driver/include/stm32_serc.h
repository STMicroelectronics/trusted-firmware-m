/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_SERC_H
#define __STM32_SERC_H

#ifndef TFM_ENV
/* optee */
#include <types_ext.h>
#endif

struct serc_cfg {
	uint32_t *it_mask;
	uint32_t n_mask;
};

struct serc_driver_data {
	uint32_t version;
	uint8_t num_ilac;
};

struct stm32_serc_platdata {
	uintptr_t base;
	unsigned long clk_id;
	struct serc_driver_data *drv_data;
	uint32_t *it_mask;
	uint32_t nmask;
	int irq;
};

int stm32_serc_enable_irq(void);
int stm32_serc_get_platdata(struct stm32_serc_platdata *pdata);
int stm32_serc_init(void);
#endif /* __STM32_SERC_H */
