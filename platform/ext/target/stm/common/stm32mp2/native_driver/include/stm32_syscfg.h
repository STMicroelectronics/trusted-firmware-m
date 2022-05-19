/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_SYSCFG_H
#define STM32_SYSCFG_H

#include <stdint.h>
#include <stdbool.h>

struct stm32_syscfg_platdata {
	uintptr_t base;
};

void stm32_syscfg_write(uint32_t offset, uint32_t val);
uint32_t stm32_syscfg_read(uint32_t offset);
void stm32_syscfg_safe_rst(bool enable);

int stm32_syscfg_init(void);

#endif /* STM32_SYSCFG_H */
