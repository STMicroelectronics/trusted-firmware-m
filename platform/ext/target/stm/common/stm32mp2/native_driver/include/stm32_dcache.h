/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_DCACHE_H
#define __STM32_DCACHE_H

#include <stdbool.h>

struct dcache_driver_data {
	uint32_t version;
	uint8_t ways;
};

struct stm32_dcache_platdata {
	uintptr_t base;
	struct dcache_driver_data *drv_data;
	int irq;
};

struct stm32_dcache_mon {
	uint32_t rmiss;
	uint32_t rhit;
	uint32_t wmiss;
	uint32_t whit;
};

int stm32_dcache_enable_irq(void);
int stm32_dcache_monitor_reset(void);
int stm32_dcache_monitor_start(void);
int stm32_dcache_monitor_stop(void);
int stm32_dcache_monitor_get(struct stm32_dcache_mon *mon);
int stm32_dcache_full_inv(void);
int stm32_dcache_maintenance(int cmd, uintptr_t start, uintptr_t end);
int stm32_dcache_disable(void);
int stm32_dcache_enable(bool monitor, bool inv);

#define DCACHE_CMD_CLR		0x1
#define DCACHE_CMD_INV		0x2
#define DCACHE_CMD_CLRINV	0x3

#define stm32_dcache_clean(s, e)	stm32_dcache_maintenance(DCACHE_CMD_CLR, s, e)
#define stm32_dcache_inv(s, e)		stm32_dcache_maintenance(DCACHE_CMD_INV, s, e)
#define stm32_dcache_clean_inv(s, e)	stm32_dcache_maintenance(DCACHE_CMD_CLRINV, s, e)

int stm32_dcache_get_platdata(struct stm32_dcache_platdata *pdata);
int stm32_dcache_init(void);
#endif /* __STM32_DCACHE_H */
