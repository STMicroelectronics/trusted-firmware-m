/*
 * Copyright (c) 2021, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
 */

#ifndef STM32_OSPI_H
#define STM32_OSPI_H

#include <stddef.h>

struct stm32_ospi_platdata {
	const struct stm32_omi_config *drv_cfg;

	struct spi_slave *spi_slave;
};

int stm32_ospi_init(void);
int stm32_ospi_deinit(void);

#endif /* STM32_OSPI_H */
