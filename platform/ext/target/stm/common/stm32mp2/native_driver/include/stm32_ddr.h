/*
 * Copyright (c) 2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_DDR_H
#define STM32_DDR_H

struct stm32_ddr_platdata {
	uintptr_t base;
};

int stm32_ddr_init(void);

#endif /* STM32_DDR_H */
