/*
 * Copyright (c) 2015-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef STM32_GPIO_H
#define STM32_GPIO_H

#include <lib/utils_def.h>

#define GPIO_ALTERNATE_(_x)	U(_x)

#define GPIO_MODE_INPUT			0x00
#define GPIO_MODE_OUTPUT		0x01
#define GPIO_MODE_ALTERNATE		0x02
#define GPIO_MODE_ANALOG		0x03
#define GPIO_MODE_MASK			U(0x03)

#define GPIO_PUSH_PULL			0x00
#define GPIO_OPEN_DRAIN			0x01

#define GPIO_SPEED_LOW			0x00
#define GPIO_SPEED_MEDIUM		0x01
#define GPIO_SPEED_HIGH			0x02
#define GPIO_SPEED_VERY_HIGH		0x03
#define GPIO_SPEED_MASK			U(0x03)

#define GPIO_NO_PULL			0x00
#define GPIO_PULL_UP			0x01
#define GPIO_PULL_DOWN			0x02
#define GPIO_PULL_MASK			U(0x03)

#include <stdint.h>

struct gpio_cfg {
	uint32_t bank;
	uint32_t pin;
	uint32_t mode;
	uint32_t type;
	uint32_t speed;
	uint32_t pull;
	uint32_t alternate;
};

#endif /* STM32_GPIO_H */
