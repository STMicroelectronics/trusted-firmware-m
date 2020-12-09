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

#ifndef __ASSEMBLER__
#include <stdint.h>

/* client api */
#define STM32_PINCFG(_pinsmux, _slew_rate, _pull, _open_drain) \
	{                                                          \
		.pinsmux = _pinsmux,                                   \
		.npinsmux = ARRAY_SIZE(_pinsmux),                      \
		.slew_rate = _slew_rate,                               \
		.pull = _pull,                                         \
		.open_drain = _open_drain,                             \
	}

struct pin_cfg {
	uint32_t *pinsmux;
	int npinsmux;
	uint32_t slew_rate;
	uint32_t pull;
	uint32_t open_drain;
};

#define PINCTRL_CFG_DEFINE(_name, _pinctrl_cfg)            \
	struct pinctrl_cfg _name = {                           \
		.pins = _pinctrl_cfg,                              \
	    .npins = ARRAY_SIZE(_pinctrl_cfg),                 \
	};

/**
 * struct pinctrl_cfg - Representation of pinctrl config
 *
 * @pins:	    reference on pins configuration
 * @npins:      number of pins cfg in this pinctrl config
 */
struct pinctrl_cfg {
	struct pin_cfg *pins;
	int npins;
};

int set_pinctrl_config(struct pinctrl_cfg *pctrl_cfg);

/* driver init */
#define BANK_TO_ID(_bank) (_bank - 'A')

#define STM32_BANK_CFG(_bank, _base, _clk_id) \
	{                                         \
		.id = BANK_TO_ID(_bank),              \
		.base = _base,                        \
		.clk_id = _clk_id,                    \
	}

struct bank_cfg {
	uint32_t id;
	unsigned long base;
	uint32_t clk_id;
};

struct stm32_gpio_platdata {
	const struct bank_cfg *banks;
	int nbanks;
};

int stm32_gpio_init(void);

#endif /*__ASSEMBLER__*/

#endif /* STM32_GPIO_H */
