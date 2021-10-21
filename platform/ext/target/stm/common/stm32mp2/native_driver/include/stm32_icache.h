/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, STMicroelectronics
 */
#ifndef __STM32_ICACHE_H
#define __STM32_ICACHE_H

#include <stdbool.h>

struct icache_driver_data {
	uint32_t version;
	uint8_t nb_region;
	uint8_t ways;
	uint32_t range;
};

struct stm32_icache_platdata {
	uintptr_t base;
	struct icache_driver_data *drv_data;
	int irq;
};

struct stm32_icache_region {
	uint8_t n_region; // region number
	uintptr_t icache_addr;
	uintptr_t device_addr;
	bool slow_c_bus;
	uint32_t size;
};

struct stm32_icache_mon {
	uint32_t miss;
	uint32_t hit;
};

int stm32_icache_enable_irq(void);
int stm32_icache_monitor_reset(void);
int stm32_icache_monitor_start(void);
int stm32_icache_monitor_stop(void);
int stm32_icache_monitor_get(struct stm32_icache_mon *mon);
int stm32_icache_full_inv(void);
int stm32_icache_disable(void);
int stm32_icache_enable(bool monitor, bool inv);
int stm32_icache_region_enable(struct stm32_icache_region* region);
int stm32_icache_region_disable(struct stm32_icache_region* region);

int stm32_icache_get_platdata(struct stm32_icache_platdata *pdata);
int stm32_icache_init(void);
#endif /* __STM32_ICACHE_H */
