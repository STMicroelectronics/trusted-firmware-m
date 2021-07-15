/*
 * Copyright (c) 2021, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
 */

#ifndef STM32_OSPI_H
#define STM32_OSPI_H

#include <stm32_gpio.h>
#include <stddef.h>

#define _OSPI_MAX_RESET		2U

struct stm32_ospi_platdata {
	uintptr_t reg_base;
	uintptr_t mm_base;
	size_t mm_size;

	unsigned long clock_id;
	unsigned int reset_id[_OSPI_MAX_RESET];

	struct pinctrl_cfg *pinctrl;

	struct spi_slave *spi_slave;
};

int stm32_ospi_init(void);
int stm32_ospi_deinit(void);

#endif /* STM32_OSPI_H */
