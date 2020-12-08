/*
 * Copyright (C) 2018-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STM32MP2_CLK_H
#define STM32MP2_CLK_H

#include <stdbool.h>

enum OSC {
	OSC_HSI,
	OSC_HSE,
	OSC_CSI,
	OSC_LSI,
	OSC_LSE,
	OSC_I2SCKIN,
	OSC_SPDIFSYMB,
	NB_OSCILLATOR
};

enum pll_id {
	_PLL1,
	_PLL2,
	_PLL3,
	_PLL4,
	_PLL5,
	_PLL6,
	_PLL7,
	_PLL8,
	_PLL_NB
};

enum pll_cfg {
	FBDIV,
	REFDIV,
	POSTDIV1,
	POSTDIV2,
	PLLCFG_NB
};

enum pll_csg {
	DIVVAL,
	SPREAD,
	DOWNSPREAD,
	PLLCSG_NB
};

enum clkdiv_id {
	CLKDIV_APB1,
	CLKDIV_APB2,
	CLKDIV_APB3,
	CLKDIV_APB4,
	CLKDIV_APBDBG,
	CLKDIV_RTC,
	CLKDIV_NB
};

#define STM32_OSCI_CFG(_id, _freq, _bypass, _digbyp, css, _drive) \
	[_id] = {                                                     \
		.freq = _freq,                                            \
		.bypass = _bypass,                                        \
		.digbyp = _digbyp,                                        \
		.drive = _drive,                                          \
	}

struct stm32_osci_dt_cfg {
	unsigned long freq;
	bool bypass;
	bool digbyp;
	bool css;
	uint32_t drive;
};

#define STM32_PLL_CFG_CSG(_id, _cfg, _frac, _csg) \
	[_id] = {                                     \
		.enabled = true,                          \
		.cfg = _cfg,                              \
		.frac = _frac,                            \
		.csg = _csg,                              \
		.csg_enabled = true,                      \
	}

#define STM32_PLL_CFG(_id, _cfg, _frac) \
	[_id] = {                           \
		.enabled = true,                \
		.cfg = _cfg,                    \
		.frac = _frac,                  \
		.csg = {},                      \
		.csg_enabled = false,           \
	}

struct stm32_pll_dt_cfg {
	bool enabled;
	uint32_t cfg[PLLCFG_NB];
	uint32_t frac;
	uint32_t csg[PLLCSG_NB];
	bool csg_enabled;
};

struct stm32mp2_clk_platdata {
	uintptr_t rcc_base;
	struct stm32_osci_dt_cfg *osci;
	uint32_t nosci;
	struct stm32_pll_dt_cfg *pll;
	uint32_t npll;
	uint32_t *clksrc;
	uint32_t nclksrc;
	uint32_t *clkdiv;
	uint32_t nclkdiv;
};

int stm32mp2_clk_init(void);

#endif /* STM32MP2_CLK_H */
