/*
 * Copyright (C) 2018-2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <stdint.h>

#ifdef LIBFDT
#include <libfdt.h>
#include <common/fdt_wrappers.h>
#endif

#ifdef TFM_ENV
#include <lib/timeout.h>
#include <lib/delay.h>
#include <debug.h>
#include <clk.h>
#include <stm32mp_clkfunc.h>
#include <stm32mp2_clk.h>
#include <stm32mp2_rcc.h>
#include <stm32_systick.h>
#else
#include <platform_def.h>
#include <drivers/clk.h>
#include <drivers/delay_timer.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/st/stm32mp_clkfunc.h>
#include <drivers/st/stm32mp2_clk.h>
#endif

#include <dt-bindings/clock/stm32mp2-clksrc.h>
#include <dt-bindings/clock/stm32mp2-clks.h>

#if !defined(TFM_ENV) || defined(TFM_MULTI_CORE_TOPOLOGY)
#include <lib/spinlock.h>
#endif

#include "clk-stm32-core.h"

#ifndef __WORD_BIT
#define __WORD_BIT 32
#endif

static struct stm32mp2_clk_platdata clk_pdata;

/* A35 Sub-System which manages its own PLL (PLL1) */
#define A35_SS_CHGCLKREQ	0x0000
#define A35_SS_PLL_FREQ1	0x0080
#define A35_SS_PLL_FREQ2	0x0090
#define A35_SS_PLL_ENABLE	0x00a0

#define A35_SS_CHGCLKREQ_ARM_CHGCLKREQ		BIT(0)
#define A35_SS_CHGCLKREQ_ARM_CHGCLKACK		BIT(1)

#define A35_SS_PLL_FREQ1_FBDIV_MASK		GENMASK(11, 0)
#define A35_SS_PLL_FREQ1_FBDIV_SHIFT		0
#define A35_SS_PLL_FREQ1_REFDIV_MASK		GENMASK(21, 16)
#define A35_SS_PLL_FREQ1_REFDIV_SHIFT		16

#define A35_SS_PLL_FREQ2_POSTDIV1_MASK		GENMASK(2, 0)
#define A35_SS_PLL_FREQ2_POSTDIV1_SHIFT		0
#define A35_SS_PLL_FREQ2_POSTDIV2_MASK		GENMASK(5, 3)
#define A35_SS_PLL_FREQ2_POSTDIV2_SHIFT		3

#define A35_SS_PLL_ENABLE_PD			BIT(0)
#define A35_SS_PLL_ENABLE_LOCKP			BIT(1)
#define A35_SS_PLL_ENABLE_NRESET_SWPLL_FF	BIT(2)

#define TIMEOUT_US_200MS	U(200000)
#define TIMEOUT_US_1S		U(1000000)

#define PLLRDY_TIMEOUT		TIMEOUT_US_200MS
#define CLKSRC_TIMEOUT		TIMEOUT_US_200MS
#define CLKDIV_TIMEOUT		TIMEOUT_US_200MS
#define OSCRDY_TIMEOUT		TIMEOUT_US_1S

/* Parameters from PLL_CFG in st,cksrc field */
#define PLL_CKSRC_NB			GENMASK(2, 0)
#define PLL_CKSRC_SRC			GENMASK(7, 4)
#define PLL_CKSRC_SRC_OFFSET		4

/* PLL minimal frequencies for clock sources */
#define PLL_REFCLK_MIN			UL(5000000)
#define PLL_FRAC_REFCLK_MIN		UL(10000000)

/* Parameters from XBAR_CFG in st,cksrc field */
#define XBAR_CKSRC_CHANNEL		GENMASK(5, 0)
#define XBAR_CKSRC_SRC			GENMASK(9, 6)
#define XBAR_CKSRC_SRC_OFFSET		6
#define XBAR_CKSRC_PREDIV		GENMASK(19, 10)
#define XBAR_CKSRC_PREDIV_OFFSET	10
#define XBAR_CKSRC_FINDIV		GENMASK(25, 20)
#define XBAR_CKSRC_FINDIV_OFFSET	20

#define XBAR_CHANNEL_NB			64

/* Warning, should be start to 1 */
enum clock {
	_CK_0_MHZ = 1,
	_CK_4_MHZ,
	_CK_16_MHZ,

	/* ROOT CLOCKS */
	_CK_HSI,
	_CK_HSE,
	_CK_CSI,
	_CK_LSI,
	_CK_LSE,
	_I2SCKIN,
	_SPDIFSYMB,
	_CK_PLL1,
	_CK_PLL2,
	_CK_PLL3,
	_CK_PLL4,
	_CK_PLL5,
	_CK_PLL6,
	_CK_PLL7,
	_CK_PLL8,
	_CK_HSE_RTC,
	_CK_RTCCK,
	_CK_ICN_HS_MCU,
	_CK_ICN_SDMMC,
	_CK_ICN_DDR,
	_CK_ICN_DISPLAY,
	_CK_ICN_HSL,
	_CK_ICN_NIC,
	_CK_ICN_VID,
	_CK_ICN_LS_MCU,
	_CK_FLEXGEN_07,
	_CK_FLEXGEN_08,
	_CK_FLEXGEN_09,
	_CK_FLEXGEN_10,
	_CK_FLEXGEN_11,
	_CK_FLEXGEN_12,
	_CK_FLEXGEN_13,
	_CK_FLEXGEN_14,
	_CK_FLEXGEN_15,
	_CK_FLEXGEN_16,
	_CK_FLEXGEN_17,
	_CK_FLEXGEN_18,
	_CK_FLEXGEN_19,
	_CK_FLEXGEN_20,
	_CK_FLEXGEN_21,
	_CK_FLEXGEN_22,
	_CK_FLEXGEN_23,
	_CK_FLEXGEN_24,
	_CK_FLEXGEN_25,
	_CK_FLEXGEN_26,
	_CK_FLEXGEN_27,
	_CK_FLEXGEN_28,
	_CK_FLEXGEN_29,
	_CK_FLEXGEN_30,
	_CK_FLEXGEN_31,
	_CK_FLEXGEN_32,
	_CK_FLEXGEN_33,
	_CK_FLEXGEN_34,
	_CK_FLEXGEN_35,
	_CK_FLEXGEN_36,
	_CK_FLEXGEN_37,
	_CK_FLEXGEN_38,
	_CK_FLEXGEN_39,
	_CK_FLEXGEN_40,
	_CK_FLEXGEN_41,
	_CK_FLEXGEN_42,
	_CK_FLEXGEN_43,
	_CK_FLEXGEN_44,
	_CK_FLEXGEN_45,
	_CK_FLEXGEN_46,
	_CK_FLEXGEN_47,
	_CK_FLEXGEN_48,
	_CK_FLEXGEN_49,
	_CK_FLEXGEN_50,
	_CK_FLEXGEN_51,
	_CK_FLEXGEN_52,
	_CK_FLEXGEN_53,
	_CK_FLEXGEN_54,
	_CK_FLEXGEN_55,
	_CK_FLEXGEN_56,
	_CK_FLEXGEN_57,
	_CK_FLEXGEN_58,
	_CK_FLEXGEN_59,
	_CK_FLEXGEN_60,
	_CK_FLEXGEN_61,
	_CK_FLEXGEN_62,
	_CK_FLEXGEN_63,
	_CK_ICN_APB1,
	_CK_ICN_APB2,
	_CK_ICN_APB3,
	_CK_ICN_APB4,
	_CK_ICN_APBDBG,
	_TIMG1_CK,
	_TIMG2_CK,
	_CK_ADC12,
	_CK_ADC3,
	_CK_BKPSRAM,
	_CK_BSEC,
	_CK_BUSPERFM,
	_CK_CCI,
	_CK_CRC,
	_CK_CRYP1,
	_CK_CRYP2,
	_CK_CSI2,
	_CK_CSI2TXESC,
	_CK_CSI2PHY,
	_CK_TSDBG,
	_CK_TPIU,
	_CK_DCMIPP,
	_CK_DDR,
	_CK_DDRCAPB,
	_CK_DDRCP,
	_CK_DDRPHYC,
	_CK_DSIBLANE,
	_CK_DSIPHY,
	_CK_ETH1,
	_CK_ETH1PTP,
	_CK_ETH2,
	_CK_ETH2PTP,
	_CK_ETHSW,
	_CK_ETHSWREF,
	_CK_ETR,
	_CK_FDCAN,
	_CK_FMC,
	_CK_GICV2M,
	_CK_GPIOA,
	_CK_GPIOB,
	_CK_GPIOC,
	_CK_GPIOD,
	_CK_GPIOE,
	_CK_GPIOF,
	_CK_GPIOG,
	_CK_GPIOH,
	_CK_GPIOI,
	_CK_GPIOJ,
	_CK_GPIOK,
	_CK_GPIOZ,
	_CK_GPU,
	_CK_HASH,
	_CK_HDP,
	_CK_HPDMA1,
	_CK_HPDMA2,
	_CK_HPDMA3,
	_CK_HSEM,
	_CK_I2C1,
	_CK_I2C2,
	_CK_I2C3,
	_CK_I2C4,
	_CK_I2C5,
	_CK_I2C6,
	_CK_I2C7,
	_CK_I2C8,
	_CK_I3C1,
	_CK_I3C2,
	_CK_I3C3,
	_CK_I3C4,
	_CK_IPCC1,
	_CK_IPCC2,
	_CK_IS2M,
	_CK_IWDG1,
	_CK_IWDG2,
	_CK_IWDG3,
	_CK_IWDG4,
	_CK_IWDG5,
	_CK_LVDSPHY,
	_CK_LPDMA,
	_CK_LPSRAM1,
	_CK_LPSRAM2,
	_CK_LPSRAM3,
	_CK_LPTIM1,
	_CK_LPTIM2,
	_CK_LPTIM3,
	_CK_LPTIM4,
	_CK_LPTIM5,
	_CK_LPUART1,
	_CK_LTDC,
	_CK_MCO1,
	_CK_MCO2,
	_CK_MDF1,
	_CK_ADF1,
	_CK_OSPI1,
	_CK_OSPI2,
	_CK_OSPIIOM,
	_CK_PCIE,
	_CK_PKA,
	_CK_RETRAM,
	_CK_RNG,
	_CK_RTC,
	_CK_SAES,
	_CK_SAI1,
	_CK_SAI2,
	_CK_SAI3,
	_CK_SAI4,
	_CK_SDMMC1,
	_CK_SDMMC2,
	_CK_SDMMC3,
	_CK_SERC,
	_CK_SPDIFRX,
	_CK_SPI1,
	_CK_SPI2,
	_CK_SPI3,
	_CK_SPI4,
	_CK_SPI5,
	_CK_SPI6,
	_CK_SPI7,
	_CK_SPI8,
	_CK_SRAM1,
	_CK_SRAM2,
	_CK_STGEN,
	_CK_STM500,
	_CK_SYSCPU1,
	_CK_SYSRAM,
	_CK_TIM1,
	_CK_TIM10,
	_CK_TIM11,
	_CK_TIM12,
	_CK_TIM13,
	_CK_TIM14,
	_CK_TIM15,
	_CK_TIM16,
	_CK_TIM17,
	_CK_TIM2,
	_CK_TIM20,
	_CK_TIM3,
	_CK_TIM4,
	_CK_TIM5,
	_CK_TIM6,
	_CK_TIM7,
	_CK_TIM8,
	_CK_TMPSENS,
	_CK_UART4,
	_CK_UART5,
	_CK_UART7,
	_CK_UART8,
	_CK_UART9,
	_CK_USART1,
	_CK_USART2,
	_CK_USART3,
	_CK_USART6,
	_CK_USB2EHCI,
	_CK_USB2OHCI,
	_CK_USB2PHY1,
	_CK_USB2PHY2,
	_CK_USB3DRD,
	_CK_USB3PCIEPHY,
	_CK_USBTC,
	_CK_VDEC,
	_CK_VDERAM,
	_CK_VENC,
	_CK_VREF,
	_CK_WWDG1,
	_CK_WWDG2,

	CK_LAST
};

#define MUX_CFG(id, src, _offset, _shift, _witdh)[id] = {\
	.id_parents	= src,\
	.num_parents	= ARRAY_SIZE(src),\
	.mux		= &(struct mux_cfg) {\
		.offset	= (_offset),\
		.shift	= (_shift),\
		.width	= (_witdh),\
		.bitrdy = UINT8_MAX,\
	},\
}

enum enum_mux_cfg {
	MUX_MUXSEL0,
	MUX_MUXSEL1,
	MUX_MUXSEL2,
	MUX_MUXSEL3,
	MUX_MUXSEL4,
	MUX_MUXSEL5,
	MUX_MUXSEL6,
	MUX_MUXSEL7,
	MUX_XBARSEL,
	MUX_RTC,

	LAST_MUX
};

static const uint16_t muxsel_src[] = {
	 _CK_HSI, _CK_HSE, _CK_CSI, _CK_0_MHZ
};

static const uint16_t xbarsel_src[] = {
	_CK_PLL4, _CK_PLL5, _CK_PLL6, _CK_PLL7, _CK_PLL8,
	_CK_HSI, _CK_HSE, _CK_CSI, _CK_HSI, _CK_HSE, _CK_CSI,
	_SPDIFSYMB, _I2SCKIN, _CK_LSI, _CK_LSE
};

static const uint16_t rtc_src[] = {
	_CK_0_MHZ, _CK_LSE, _CK_LSI, _CK_HSE_RTC
};

static const struct parent_cfg parent_mp25[LAST_MUX] = {
	MUX_CFG(MUX_MUXSEL0, muxsel_src, _RCC_MUXSELCFGR, 0, 2),
	MUX_CFG(MUX_MUXSEL1, muxsel_src, _RCC_MUXSELCFGR, 4, 2),
	MUX_CFG(MUX_MUXSEL2, muxsel_src, _RCC_MUXSELCFGR, 8, 2),
	MUX_CFG(MUX_MUXSEL3, muxsel_src, _RCC_MUXSELCFGR, 12, 2),
	MUX_CFG(MUX_MUXSEL4, muxsel_src, _RCC_MUXSELCFGR, 16, 2),
	MUX_CFG(MUX_MUXSEL5, muxsel_src, _RCC_MUXSELCFGR, 20, 2),
	MUX_CFG(MUX_MUXSEL6, muxsel_src, _RCC_MUXSELCFGR, 24, 2),
	MUX_CFG(MUX_MUXSEL7, muxsel_src, _RCC_MUXSELCFGR, 28, 2),
	MUX_CFG(MUX_XBARSEL, xbarsel_src, _RCC_XBAR0CFGR, 0, 4),
	MUX_CFG(MUX_RTC, rtc_src, _RCC_BDCR, 16, 2),
};

/* GATES */
enum enum_gate_cfg {
	GATE_ZERO, /* reserved for no gate */
	GATE_LSE,
	GATE_RTCCK,
	GATE_LSI,
	GATE_HSI,
	GATE_CSI,
	GATE_HSE,
	GATE_LSI_RDY,
	GATE_CSI_RDY,
	GATE_LSE_RDY,
	GATE_HSE_RDY,
	GATE_HSI_RDY,
	LAST_GATE
};

#define GATE_CFG(id, _offset, _bit_idx, _offset_clr)[id] = {\
	.offset		= (_offset),\
	.bit_idx	= (_bit_idx),\
	.set_clr	= (_offset_clr),\
}

/* TODO : ADD MCO... ETH */
static const struct gate_cfg gates_mp25[LAST_GATE] = {
	GATE_CFG(GATE_LSE,	_RCC_BDCR,	0,	0),
	GATE_CFG(GATE_LSI,	_RCC_BDCR,	9,	0),
	GATE_CFG(GATE_RTCCK,	_RCC_BDCR,	20,	0),
	GATE_CFG(GATE_HSI,	_RCC_OCENSETR,	0,	1),
	GATE_CFG(GATE_HSE,	_RCC_OCENSETR,	8,	1),
	GATE_CFG(GATE_CSI,	_RCC_D3DCR,	0,	0),

	GATE_CFG(GATE_LSI_RDY,	_RCC_BDCR,	10,	0),
	GATE_CFG(GATE_LSE_RDY,	_RCC_BDCR,	2,	0),
	GATE_CFG(GATE_CSI_RDY,	_RCC_D3DCR,	2,	0),
	GATE_CFG(GATE_HSE_RDY,	_RCC_OCRDYR,	8,	0),
	GATE_CFG(GATE_HSI_RDY,	_RCC_OCRDYR,	0,	0),

};

static const struct clk_div_table apb_div_table[] = {
	{ 0, 1 },  { 1, 2 },  { 2, 4 },  { 3, 8 }, { 4, 16 },
	{ 5, 16 }, { 6, 16 }, { 7, 16 }, { 0 },
};

enum enum_div_cfg {
	DIV_LSMCU,
	DIV_APB1DIV,
	DIV_APB2DIV,
	DIV_APB3DIV,
	DIV_APB4DIV,
	DIV_APBDBG,
	DIV_RTC,

	LAST_DIV
};

#define DIV_CFG(id, _offset, _shift, _width, _flags, _table, _bitrdy)[id] = {\
		.offset	= _offset,\
		.shift	= _shift,\
		.width	= _width,\
		.flags	= _flags,\
		.table	= _table,\
		.bitrdy	= _bitrdy,\
}

static const struct div_cfg dividers_mp25[LAST_DIV] = {
	DIV_CFG(DIV_APB1DIV, _RCC_APB1DIVR, 0, 3, 0, apb_div_table, 31),
	DIV_CFG(DIV_APB2DIV, _RCC_APB3DIVR, 0, 3, 0, apb_div_table, 31),
	DIV_CFG(DIV_APB3DIV, _RCC_APB3DIVR, 0, 3, 0, apb_div_table, 31),
	DIV_CFG(DIV_APB4DIV, _RCC_APB4DIVR, 0, 3, 0, apb_div_table, 31),
	DIV_CFG(DIV_APBDBG, _RCC_APBDBGDIVR, 0, 3, 0, apb_div_table, 31),
	DIV_CFG(DIV_LSMCU, _RCC_LSMCUDIVR, 0, 1, 0, NULL, 31),
	DIV_CFG(DIV_RTC, _RCC_RTCDIVR, 0, 6, 0, NULL, 0),

	// DIV_CFG(DIV_MCO2, _RCC_MCO2CFGR, 4, 4, 0, NULL),
	// DIV_CFG(DIV_TRACE, _RCC_DBGCFGR, 0, 3, 0, ck_trace_div_table),
	// DIV_CFG(DIV_ETH1PTP, _RCC_ETH12CKSELR, 4, 4, 0, NULL),
	// DIV_CFG(DIV_ETH2PTP, _RCC_ETH12CKSELR, 12, 4, 0, NULL),
};

const char *stm32mp_osc_node_label[NB_OSCILLATOR] = {
	[OSC_LSI] = "clk-lsi",
	[OSC_LSE] = "clk-lse",
	[OSC_HSI] = "clk-hsi",
	[OSC_HSE] = "clk-hse",
	[OSC_CSI] = "clk-csi",
	[OSC_I2SCKIN] = "i2s_ckin",
	[OSC_SPDIFSYMB] = "spdif_symb",
};

static struct clk_oscillator_data stm32mp25_osc_data[] = {
	OSCILLATOR(OSC_HSI, _CK_HSI, "clk-hsi", GATE_HSI, GATE_HSI_RDY,
		   NULL, NULL, NULL),

	OSCILLATOR(OSC_LSI, _CK_LSI, "clk-lsi", GATE_LSI, GATE_LSI_RDY,
		   NULL, NULL, NULL),

	OSCILLATOR(OSC_CSI, _CK_CSI, "clk-csi", GATE_CSI, GATE_CSI_RDY,
		   NULL, NULL, NULL),

	OSCILLATOR(OSC_HSE, _CK_HSE, "clk-hse", GATE_HSE, GATE_HSE_RDY,
		   BYPASS(_RCC_OCENSETR, 10, 7),
		   CSS(_RCC_OCENSETR, 11),
		   NULL),

	OSCILLATOR(OSC_LSE, _CK_LSE, "clk-lse", GATE_LSE, GATE_LSE_RDY,
		   BYPASS(_RCC_BDCR, 1, 3),
		   CSS(_RCC_BDCR, 8),
		   DRIVE(_RCC_BDCR, 4, 2, 2)),

	OSCILLATOR(OSC_I2SCKIN, _I2SCKIN, "i2s_ckin", NO_GATE, NO_GATE,
		   NULL, NULL, NULL),

	OSCILLATOR(OSC_SPDIFSYMB, _SPDIFSYMB, "spdif_symb", NO_GATE, NO_GATE,
		   NULL, NULL, NULL),
};

#define _CLK_PLL(idx, off1, off2)	\
	[(idx)] = {			\
		.muxsel = (off1),	\
		.pllxcfgr1 = (off2),	\
	}

struct clk_pll {
	uint16_t muxsel;
	uint16_t pllxcfgr1;
};

static const struct clk_pll stm32mp25_clk_pll[_PLL_NB] = {
	_CLK_PLL(_PLL1, _RCC_MUXSELCFGR_MUXSEL5_SHIFT, 0),
	_CLK_PLL(_PLL2, _RCC_MUXSELCFGR_MUXSEL6_SHIFT, _RCC_PLL2CFGR1),
	_CLK_PLL(_PLL3, _RCC_MUXSELCFGR_MUXSEL7_SHIFT, _RCC_PLL3CFGR1),
	_CLK_PLL(_PLL4, _RCC_MUXSELCFGR_MUXSEL0_SHIFT, _RCC_PLL4CFGR1),
	_CLK_PLL(_PLL5, _RCC_MUXSELCFGR_MUXSEL1_SHIFT, _RCC_PLL5CFGR1),
	_CLK_PLL(_PLL6, _RCC_MUXSELCFGR_MUXSEL2_SHIFT, _RCC_PLL6CFGR1),
	_CLK_PLL(_PLL7, _RCC_MUXSELCFGR_MUXSEL3_SHIFT, _RCC_PLL7CFGR1),
	_CLK_PLL(_PLL8, _RCC_MUXSELCFGR_MUXSEL4_SHIFT, _RCC_PLL8CFGR1),
};

/* PLL configuration registers offsets from _RCC_PLLxCFGR1 */
static const uintptr_t pll_cfgr_offsets[] = {
	0x00, /* _RCC_PLLxCFGR1 offset */
	0x04, /* _RCC_PLLxCFGR2 offset */
	0x08, /* _RCC_PLLxCFGR3 offset */
	0x0C, /* _RCC_PLLxCFGR4 offset */
	0x10, /* _RCC_PLLxCFGR5 offset */
	0x18, /* _RCC_PLLxCFGR6 offset */
	0x1C, /* _RCC_PLLxCFGR7 offset */
};

static const struct clk_pll *stm32mp2_pll_ref(unsigned int idx)
{
	return &stm32mp25_clk_pll[idx];
}

static unsigned long clk_get_pll_fvco(struct stm32_clk_priv *priv,
				      const struct clk_pll *pll,
				      unsigned long prate)
{
	uintptr_t rcc_base = priv->base;

	unsigned long refclk, fvco;
	uint32_t fracin, fbdiv, refdiv;
	uintptr_t pllxcfgr1 = rcc_base + pll->pllxcfgr1;
	uintptr_t pllxcfgr2 = pllxcfgr1 + pll_cfgr_offsets[1];
	uintptr_t pllxcfgr3 = pllxcfgr1 + pll_cfgr_offsets[2];

	refclk = prate;

	fracin = mmio_read_32(pllxcfgr3) & _RCC_PLLxCFGR3_FRACIN_MASK;
	fbdiv = (mmio_read_32(pllxcfgr2) & _RCC_PLLxCFGR2_FBDIV_MASK) >>
		_RCC_PLLxCFGR2_FBDIV_SHIFT;
	refdiv = mmio_read_32(pllxcfgr2) & _RCC_PLLxCFGR2_FREFDIV_MASK;

	if (fracin != 0U) {
		uint64_t numerator, denominator;

		numerator = ((uint64_t)fbdiv << 24) + fracin;
		numerator = refclk * numerator;
		denominator = (uint64_t)refdiv << 24;
		fvco = (unsigned long)(numerator / denominator);
	} else {
		fvco = (uint64_t) refclk * fbdiv / refdiv;
	}

	return fvco;
}

#define CLK_OSC(idx, _idx, _parent, _osc_id)[idx] = {\
	.name = #idx,\
	.binding = _idx,\
	.parent = _parent,\
	.flags = CLK_IS_CRITICAL,\
	.clock_cfg = (struct clk_oscillator_data *)&stm32mp25_osc_data[_osc_id],\
	.ops = &clk_stm32_osc_ops,\
}

#define CLK_OSC_FIXED(idx, _idx, _parent, _osc_id)[idx] = {\
	.name = #idx,\
	.binding = _idx,\
	.parent = _parent,\
	.flags = CLK_IS_CRITICAL,\
	.clock_cfg = (struct clk_oscillator_data *)&stm32mp25_osc_data[_osc_id],\
	.ops = &clk_stm32_osc_nogate_ops,\
}

struct stm32_pll_cfg {
	int pll_id;
};

static unsigned long clk_pll_recalc(struct stm32_clk_priv *priv, int idx,
				    unsigned long prate)
{
	const struct clk_stm32 *clk = _clk_get(priv, idx);
	struct stm32_pll_cfg *pll_cfg = clk->clock_cfg;

	uintptr_t rcc_base = priv->base;
	const struct clk_pll *pll = stm32mp2_pll_ref(pll_cfg->pll_id);

	unsigned long dfout;
	uintptr_t pllxcfgr1 = rcc_base + pll->pllxcfgr1;
	uintptr_t pllxcfgr4 = pllxcfgr1 + pll_cfgr_offsets[3];
	uintptr_t pllxcfgr6 = pllxcfgr1 + pll_cfgr_offsets[5];
	uintptr_t pllxcfgr7 = pllxcfgr1 + pll_cfgr_offsets[6];
	uint32_t postdiv1, postdiv2;

	postdiv1 = mmio_read_32(pllxcfgr6) & _RCC_PLLxCFGR6_POSTDIV1_MASK;
	postdiv2 = mmio_read_32(pllxcfgr7) & _RCC_PLLxCFGR7_POSTDIV2_MASK;

	if ((mmio_read_32(pllxcfgr4) & _RCC_PLLxCFGR4_BYPASS) != 0U) {
		dfout = prate;
	} else {
		if ((postdiv1 == 0U) || (postdiv2 == 0U)) {
			dfout = prate;
		} else {
			dfout = clk_get_pll_fvco(priv, pll, prate) / (postdiv1 * postdiv2);
		}
	}

	return dfout;
}

const struct stm32_clk_ops clk_stm32_pll_ops = {
	.recalc_rate = clk_pll_recalc,
};

#define CLK_PLL(idx, _idx, _parent, _gate, _pll_id, _flags)[idx] = {\
	.name = #idx,\
	.binding = _idx,\
	.parent = _parent,\
	.flags = (_flags),\
	.clock_cfg	= &(struct stm32_pll_cfg) {\
		.pll_id = _pll_id,\
	},\
	.ops = &clk_stm32_pll_ops,\
}

struct stm32_clk_flexgen_cfg {
	int id;
};

static unsigned long clk_flexgen_recalc(struct stm32_clk_priv *priv, int idx,
					unsigned long prate)
{
	const struct clk_stm32 *clk = _clk_get(priv, idx);
	struct stm32_clk_flexgen_cfg *cfg = clk->clock_cfg;
	uintptr_t rcc_base = priv->base;
	uint32_t prediv, findiv;
	uint8_t channel = cfg->id;
	unsigned long freq = prate;

	prediv = mmio_read_32(rcc_base + _RCC_PREDIV0CFGR + (0x4 * channel)) &
		_RCC_PREDIVxCFGR_PREDIVx_MASK;
	findiv = mmio_read_32(rcc_base + _RCC_FINDIV0CFGR + (0x4 * channel)) &
		_RCC_FINDIVxCFGR_FINDIVx_MASK;

	if (freq == 0) {
		return 0;
	}

	switch (prediv) {
	case 0x0:
		break;

	case 0x1:
		freq /= 2;
		break;

	case 0x2:
		freq /= 4;
		break;

	case 0x20:
		freq /= 1024;
		break;

	default:
		ERROR("Unsupported PREDIV value (%x)\n", prediv);
		panic();
		break;
	}

	freq /= (findiv + 1);

	return freq;

}

static int clk_flexgen_get_parent(struct stm32_clk_priv *priv, int idx)
{
	const struct clk_stm32 *clk = _clk_get(priv, idx);
	struct stm32_clk_flexgen_cfg *cfg = clk->clock_cfg;
	uint32_t sel;
	uint32_t address;
	uintptr_t rcc_base = priv->base;

	address = _RCC_XBAR0CFGR + (cfg->id * 4);

	sel = mmio_read_32(rcc_base + address) & _RCC_XBARxCFGR_XBARxSEL_MASK;

	return sel;
}

static void clk_flexgen_gate_disable(struct stm32_clk_priv *priv, int id)
{
	const struct clk_stm32 *clk = _clk_get(priv, id);
	struct stm32_clk_flexgen_cfg *cfg = clk->clock_cfg;
	uintptr_t rcc_base = priv->base;
	uint8_t channel = cfg->id;

	mmio_clrbits_32(rcc_base + _RCC_FINDIV0CFGR + (0x4 * channel),
			_RCC_FINDIVxCFGR_FINDIVxEN);

}

static bool clk_flexgen_gate_is_enabled(struct stm32_clk_priv *priv, int id)
{
	const struct clk_stm32 *clk = _clk_get(priv, id);
	struct stm32_clk_flexgen_cfg *cfg = clk->clock_cfg;
	uintptr_t rcc_base = priv->base;
	uint8_t channel = cfg->id;

	return !!(mmio_read_32(rcc_base + _RCC_FINDIV0CFGR + (0x4 * channel)) &
		_RCC_FINDIVxCFGR_FINDIVxEN);
}

static int clk_flexgen_gate_enable(struct stm32_clk_priv *priv, int idx)
{
	const struct clk_stm32 *clk = _clk_get(priv, idx);
	struct stm32_clk_flexgen_cfg *cfg = clk->clock_cfg;
	uintptr_t __unused rcc_base = priv->base;
	uint8_t __unused channel = cfg->id;

	if (!clk_flexgen_gate_is_enabled(priv, idx))
#ifdef STM32_M33TDCID
		mmio_setbits_32(rcc_base + _RCC_FINDIV0CFGR + (0x4 * channel),
				_RCC_FINDIVxCFGR_FINDIVxEN);
#else
		return -ENOTSUP;
#endif

	return 0;
}

const struct stm32_clk_ops clk_stm32_flexgen_ops = {
	.recalc_rate = clk_flexgen_recalc,
	.get_parent = clk_flexgen_get_parent,
	.enable = clk_flexgen_gate_enable,
	.disable = clk_flexgen_gate_disable,
	.is_enabled = clk_flexgen_gate_is_enabled,
};

#define FLEXGEN(idx, _idx, _flags, _id)[idx] = {\
	.name = #idx,\
	.binding = _idx,\
	.parent =  MUX(MUX_XBARSEL),\
	.flags = (_flags),\
	.clock_cfg	= &(struct stm32_clk_flexgen_cfg) {\
		.id	= _id,\
	},\
	.ops = &clk_stm32_flexgen_ops,\
}

#define RCC_0_MHZ UL(0)
#define RCC_4_MHZ UL(4000000)
#define RCC_16_MHZ UL(16000000)

const struct stm32_clk_ops clk_stm32_rtc_ops = {
	.enable = clk_stm32_gate_enable,
	.disable = clk_stm32_gate_disable,
	.is_enabled = clk_stm32_gate_is_enabled,
};

#define CLK_RTC(idx, _binding, _parent, _flags, _gate_id)[idx] = {\
	.name = #idx,\
	.binding = (_binding),\
	.parent =  (_parent),\
	.flags = (_flags),\
	.clock_cfg	= &(struct clk_stm32_gate_cfg) {\
		.id	= (_gate_id),\
	},\
	.ops = &clk_stm32_rtc_ops,\
}

static const struct clk_stm32 stm32mp25_clk[CK_LAST] = {
	CLK_FIXED_RATE(_CK_0_MHZ, _NO_ID, RCC_0_MHZ),
	CLK_FIXED_RATE(_CK_4_MHZ, _NO_ID, RCC_4_MHZ),
	CLK_FIXED_RATE(_CK_16_MHZ, _NO_ID, RCC_16_MHZ),

	/* ROOT CLOCKS */
	CLK_OSC(_CK_HSE, HSE_CK, CLK_IS_ROOT, OSC_HSE),
	CLK_OSC(_CK_LSE, LSE_CK, CLK_IS_ROOT, OSC_LSE),
	CLK_OSC(_CK_HSI, HSI_CK, CLK_IS_ROOT, OSC_HSI),
	CLK_OSC(_CK_LSI, LSI_CK, CLK_IS_ROOT, OSC_LSI),
	CLK_OSC(_CK_CSI, CSI_CK, CLK_IS_ROOT, OSC_CSI),

	CLK_OSC_FIXED(_I2SCKIN, _NO_ID, CLK_IS_ROOT, OSC_I2SCKIN),
	CLK_OSC_FIXED(_SPDIFSYMB, _NO_ID, CLK_IS_ROOT, OSC_SPDIFSYMB),

	STM32_DIV(_CK_HSE_RTC, _NO_ID, _CK_HSE, 0, DIV_RTC),

	CLK_RTC(_CK_RTCCK, RTC_CK, MUX(MUX_RTC), 0, GATE_RTCCK),

	FIXED_FACTOR(_CK_PLL1, PLL1_CK, MUX(MUX_MUXSEL5), 1, 1),
	FIXED_FACTOR(_CK_PLL2, PLL2_CK, MUX(MUX_MUXSEL6), 1, 1),
	FIXED_FACTOR(_CK_PLL3, PLL3_CK, MUX(MUX_MUXSEL7), 1, 1),

	/* TODO GATE PLL */
	CLK_PLL(_CK_PLL4, PLL4_CK, MUX(MUX_MUXSEL0), NO_GATE, _PLL4, 0),
	CLK_PLL(_CK_PLL5, PLL5_CK, MUX(MUX_MUXSEL1), NO_GATE, _PLL5, 0),
	CLK_PLL(_CK_PLL6, PLL6_CK, MUX(MUX_MUXSEL2), NO_GATE, _PLL6, 0),
	CLK_PLL(_CK_PLL7, PLL7_CK, MUX(MUX_MUXSEL3), NO_GATE, _PLL7, 0),
	CLK_PLL(_CK_PLL8, PLL8_CK, MUX(MUX_MUXSEL4), NO_GATE, _PLL8, 0),

	FLEXGEN(_CK_ICN_HS_MCU,	CK_ICN_HS_MCU, CLK_IS_CRITICAL, 0),
	FLEXGEN(_CK_ICN_SDMMC, CK_ICN_SDMMC, CLK_IS_CRITICAL, 1),
	FLEXGEN(_CK_ICN_DDR, CK_ICN_DDR, CLK_IS_CRITICAL, 2),
	FLEXGEN(_CK_ICN_DISPLAY, CK_ICN_DISPLAY, CLK_IS_CRITICAL, 3),
	FLEXGEN(_CK_ICN_HSL, CK_ICN_HSL, CLK_IS_CRITICAL, 4),
	FLEXGEN(_CK_ICN_NIC, CK_ICN_NIC, CLK_IS_CRITICAL, 5),
	FLEXGEN(_CK_ICN_VID, CK_ICN_VID, CLK_IS_CRITICAL, 6),

	STM32_DIV(_CK_ICN_LS_MCU, CK_ICN_LS_MCU, _CK_ICN_HS_MCU, 0, DIV_LSMCU),

	FLEXGEN(_CK_FLEXGEN_07, CK_FLEXGEN_07, 0, 7),
	FLEXGEN(_CK_FLEXGEN_08, CK_FLEXGEN_08, 0, 8),
	FLEXGEN(_CK_FLEXGEN_09, CK_FLEXGEN_09, 0, 9),
	FLEXGEN(_CK_FLEXGEN_10, CK_FLEXGEN_10, 0, 10),
	FLEXGEN(_CK_FLEXGEN_11, CK_FLEXGEN_11, 0, 11),
	FLEXGEN(_CK_FLEXGEN_12, CK_FLEXGEN_12, 0, 12),
	FLEXGEN(_CK_FLEXGEN_13, CK_FLEXGEN_13, 0, 13),
	FLEXGEN(_CK_FLEXGEN_14, CK_FLEXGEN_14, 0, 14),
	FLEXGEN(_CK_FLEXGEN_15, CK_FLEXGEN_15, 0, 15),
	FLEXGEN(_CK_FLEXGEN_16, CK_FLEXGEN_16, 0, 16),
	FLEXGEN(_CK_FLEXGEN_17, CK_FLEXGEN_17, 0, 17),
	FLEXGEN(_CK_FLEXGEN_18, CK_FLEXGEN_18, 0, 18),
	FLEXGEN(_CK_FLEXGEN_19, CK_FLEXGEN_19, 0, 19),
	FLEXGEN(_CK_FLEXGEN_20, CK_FLEXGEN_20, 0, 20),
	FLEXGEN(_CK_FLEXGEN_21, CK_FLEXGEN_21, 0, 21),
	FLEXGEN(_CK_FLEXGEN_22, CK_FLEXGEN_22, 0, 22),
	FLEXGEN(_CK_FLEXGEN_23, CK_FLEXGEN_23, 0, 23),
	FLEXGEN(_CK_FLEXGEN_24, CK_FLEXGEN_24, 0, 24),
	FLEXGEN(_CK_FLEXGEN_25, CK_FLEXGEN_25, 0, 25),
	FLEXGEN(_CK_FLEXGEN_26, CK_FLEXGEN_26, 0, 26),
	FLEXGEN(_CK_FLEXGEN_27, CK_FLEXGEN_27, 0, 27),
	FLEXGEN(_CK_FLEXGEN_28, CK_FLEXGEN_28, 0, 28),
	FLEXGEN(_CK_FLEXGEN_29, CK_FLEXGEN_29, 0, 29),
	FLEXGEN(_CK_FLEXGEN_30, CK_FLEXGEN_30, 0, 30),
	FLEXGEN(_CK_FLEXGEN_31, CK_FLEXGEN_31, 0, 31),
	FLEXGEN(_CK_FLEXGEN_32, CK_FLEXGEN_32, 0, 32),
	FLEXGEN(_CK_FLEXGEN_33, CK_FLEXGEN_33, 0, 33),
	FLEXGEN(_CK_FLEXGEN_34, CK_FLEXGEN_34, 0, 34),
	FLEXGEN(_CK_FLEXGEN_35, CK_FLEXGEN_35, 0, 35),
	FLEXGEN(_CK_FLEXGEN_36, CK_FLEXGEN_36, 0, 36),
	FLEXGEN(_CK_FLEXGEN_37, CK_FLEXGEN_37, 0, 37),
	FLEXGEN(_CK_FLEXGEN_38, CK_FLEXGEN_38, 0, 38),
	FLEXGEN(_CK_FLEXGEN_39, CK_FLEXGEN_39, 0, 39),
	FLEXGEN(_CK_FLEXGEN_40, CK_FLEXGEN_40, 0, 40),
	FLEXGEN(_CK_FLEXGEN_41, CK_FLEXGEN_41, 0, 41),
	FLEXGEN(_CK_FLEXGEN_42, CK_FLEXGEN_42, 0, 42),
	FLEXGEN(_CK_FLEXGEN_43, CK_FLEXGEN_43, 0, 43),
	FLEXGEN(_CK_FLEXGEN_44, CK_FLEXGEN_44, 0, 44),
	FLEXGEN(_CK_FLEXGEN_45, CK_FLEXGEN_45, 0, 45),
	FLEXGEN(_CK_FLEXGEN_46, CK_FLEXGEN_46, 0, 46),
	FLEXGEN(_CK_FLEXGEN_47, CK_FLEXGEN_47, 0, 47),
	FLEXGEN(_CK_FLEXGEN_48, CK_FLEXGEN_48, 0, 48),
	FLEXGEN(_CK_FLEXGEN_49, CK_FLEXGEN_49, 0, 49),
	FLEXGEN(_CK_FLEXGEN_50, CK_FLEXGEN_50, 0, 50),
	FLEXGEN(_CK_FLEXGEN_51, CK_FLEXGEN_51, 0, 51),
	FLEXGEN(_CK_FLEXGEN_52, CK_FLEXGEN_52, 0, 52),
	FLEXGEN(_CK_FLEXGEN_53, CK_FLEXGEN_53, 0, 53),
	FLEXGEN(_CK_FLEXGEN_54, CK_FLEXGEN_54, 0, 54),
	FLEXGEN(_CK_FLEXGEN_55, CK_FLEXGEN_55, 0, 55),
	FLEXGEN(_CK_FLEXGEN_56, CK_FLEXGEN_56, 0, 56),
	FLEXGEN(_CK_FLEXGEN_57, CK_FLEXGEN_57, 0, 57),
	FLEXGEN(_CK_FLEXGEN_58, CK_FLEXGEN_58, 0, 58),
	FLEXGEN(_CK_FLEXGEN_59, CK_FLEXGEN_59, 0, 59),
	FLEXGEN(_CK_FLEXGEN_60, CK_FLEXGEN_60, 0, 60),
	FLEXGEN(_CK_FLEXGEN_61, CK_FLEXGEN_61, 0, 61),
	FLEXGEN(_CK_FLEXGEN_62, CK_FLEXGEN_62, 0, 62),
	FLEXGEN(_CK_FLEXGEN_63, CK_FLEXGEN_62, 0, 63),

	STM32_DIV(_CK_ICN_APB1, CK_ICN_APB1, _CK_ICN_LS_MCU, 0, DIV_APB1DIV),
	STM32_DIV(_CK_ICN_APB2, CK_ICN_APB2, _CK_ICN_LS_MCU, 0, DIV_APB2DIV),
	STM32_DIV(_CK_ICN_APB3, CK_ICN_APB3, _CK_ICN_LS_MCU, 0, DIV_APB3DIV),
	STM32_DIV(_CK_ICN_APB4, CK_ICN_APB4, _CK_ICN_LS_MCU, 0, DIV_APB4DIV),
	STM32_DIV(_CK_ICN_APBDBG, CK_ICN_APBDBG, _CK_ICN_LS_MCU, 0, DIV_APBDBG),

	/* Kernel Timers */
	CK_TIMER(_TIMG1_CK, TIMG1_CK, _CK_ICN_APB1, 0,
		 _RCC_APB1DIVR, _RCC_TIMG1PRER),
	CK_TIMER(_TIMG2_CK, TIMG2_CK, _CK_ICN_APB2, 0,
		 _RCC_APB2DIVR, _RCC_TIMG2PRER),

	/* KERNEL CLOCK */
	GATE(_CK_SYSRAM, CK_BUS_SYSRAM, _CK_ICN_HS_MCU, 0, _RCC_SYSRAMCFGR, 1),
	GATE(_CK_VDERAM, CK_BUS_VDERAM, _CK_ICN_HS_MCU, 0, _RCC_VDERAMCFGR, 1),
	GATE(_CK_RETRAM, CK_BUS_RETRAM, _CK_ICN_HS_MCU, 0, _RCC_RETRAMCFGR, 1),
	GATE(_CK_SRAM1, CK_BUS_SRAM1, _CK_ICN_HS_MCU, 0, _RCC_SRAM1CFGR, 1),
	GATE(_CK_SRAM2, CK_BUS_SRAM2, _CK_ICN_HS_MCU, 0, _RCC_SRAM2CFGR, 1),

	GATE(_CK_DDRPHYC, CK_BUS_DDRPHYC, _CK_ICN_LS_MCU, 0, _RCC_DDRPHYCAPBCFGR, 1),
	GATE(_CK_SYSCPU1, CK_BUS_SYSCPU1, _CK_ICN_LS_MCU, 0, _RCC_SYSCPU1CFGR, 1),
	GATE(_CK_HPDMA1, CK_BUS_HPDMA1, _CK_ICN_LS_MCU, 0, _RCC_HPDMA1CFGR, 1),
	GATE(_CK_HPDMA2, CK_BUS_HPDMA2, _CK_ICN_LS_MCU, 0, _RCC_HPDMA2CFGR, 1),
	GATE(_CK_HPDMA3, CK_BUS_HPDMA3, _CK_ICN_LS_MCU, 0, _RCC_HPDMA3CFGR, 1),

	// GATE(_CK_ADC12, CK_BUS_ADC12, _CK_ICN_LS_MCU, 0, _RCC_ADC12CFGR, 1),
	// GATE(_CK_ADC3, CK_BUS_ADC3, _CK_ICN_LS_MCU, 0, _RCC_ADC3CFGR, 1),
	GATE(_CK_IPCC1, CK_BUS_IPCC1, _CK_ICN_LS_MCU, 0, _RCC_IPCC1CFGR, 1),
	GATE(_CK_CCI, CK_BUS_CCI, _CK_ICN_LS_MCU, 0, _RCC_CCICFGR, 1),
	GATE(_CK_CRC, CK_BUS_CRC, _CK_ICN_LS_MCU, 0, _RCC_CRCCFGR, 1),
	GATE(_CK_OSPIIOM, CK_BUS_OSPIIOM, _CK_ICN_LS_MCU, 0, _RCC_OSPIIOMCFGR, 1),
	GATE(_CK_BKPSRAM, CK_BUS_BKPSRAM, _CK_ICN_LS_MCU, 0, _RCC_BKPSRAMCFGR, 1),
	GATE(_CK_HASH, CK_BUS_HASH, _CK_ICN_LS_MCU, 0, _RCC_HASHCFGR, 1),
	GATE(_CK_RNG, CK_BUS_RNG, _CK_ICN_LS_MCU, 0, _RCC_RNGCFGR, 1),
	GATE(_CK_CRYP1, CK_BUS_CRYP1, _CK_ICN_LS_MCU, 0, _RCC_CRYP1CFGR, 1),
	GATE(_CK_CRYP2, CK_BUS_CRYP2, _CK_ICN_LS_MCU, 0, _RCC_CRYP2CFGR, 1),
	GATE(_CK_SAES, CK_BUS_SAES, _CK_ICN_LS_MCU, 0, _RCC_SAESCFGR, 1),
	GATE(_CK_PKA, CK_BUS_PKA, _CK_ICN_LS_MCU, 0, _RCC_PKACFGR, 1),

	GATE(_CK_GPIOA, CK_BUS_GPIOA, _CK_ICN_LS_MCU, 0, _RCC_GPIOACFGR, 1),
	GATE(_CK_GPIOB, CK_BUS_GPIOB, _CK_ICN_LS_MCU, 0, _RCC_GPIOBCFGR, 1),
	GATE(_CK_GPIOC, CK_BUS_GPIOC, _CK_ICN_LS_MCU, 0, _RCC_GPIOCCFGR, 1),
	GATE(_CK_GPIOD, CK_BUS_GPIOD, _CK_ICN_LS_MCU, 0, _RCC_GPIODCFGR, 1),
	GATE(_CK_GPIOE, CK_BUS_GPIOE, _CK_ICN_LS_MCU, 0, _RCC_GPIOECFGR, 1),
	GATE(_CK_GPIOF, CK_BUS_GPIOF, _CK_ICN_LS_MCU, 0, _RCC_GPIOFCFGR, 1),
	GATE(_CK_GPIOG, CK_BUS_GPIOG, _CK_ICN_LS_MCU, 0, _RCC_GPIOGCFGR, 1),
	GATE(_CK_GPIOH, CK_BUS_GPIOH, _CK_ICN_LS_MCU, 0, _RCC_GPIOHCFGR, 1),
	GATE(_CK_GPIOI, CK_BUS_GPIOI, _CK_ICN_LS_MCU, 0, _RCC_GPIOICFGR, 1),
	GATE(_CK_GPIOJ, CK_BUS_GPIOJ, _CK_ICN_LS_MCU, 0, _RCC_GPIOJCFGR, 1),
	GATE(_CK_GPIOK, CK_BUS_GPIOK, _CK_ICN_LS_MCU, 0, _RCC_GPIOKCFGR, 1),

	GATE(_CK_LPSRAM1, CK_BUS_LPSRAM1, _CK_ICN_LS_MCU, 0, _RCC_LPSRAM1CFGR, 1),
	GATE(_CK_LPSRAM2, CK_BUS_LPSRAM2, _CK_ICN_LS_MCU, 0, _RCC_LPSRAM2CFGR, 1),
	GATE(_CK_LPSRAM3, CK_BUS_LPSRAM3, _CK_ICN_LS_MCU, 0, _RCC_LPSRAM3CFGR, 1),

	GATE(_CK_GPU, CK_BUS_GPU, _CK_FLEXGEN_59, 0, _RCC_GPUCFGR, 1),
	GATE(_CK_PCIE, CK_BUS_PCIE, _CK_ICN_HSL, 0, _RCC_PCIECFGR, 1),

	GATE(_CK_GPIOZ, CK_BUS_GPIOZ, _CK_ICN_LS_MCU, 0, _RCC_GPIOZCFGR, 1),
	GATE(_CK_LPDMA, CK_BUS_LPDMA, _CK_ICN_LS_MCU, 0, _RCC_LPDMACFGR, 1),
	GATE(_CK_HSEM, CK_BUS_HSEM, _CK_ICN_LS_MCU, 0, _RCC_HSEMCFGR, 1),
	GATE(_CK_IPCC2, CK_BUS_IPCC2, _CK_ICN_LS_MCU, 0, _RCC_IPCC2CFGR, 1),
	GATE(_CK_RTC, CK_BUS_RTC, _CK_ICN_LS_MCU, 0, _RCC_RTCCFGR, 1),
	GATE(_CK_IWDG5, CK_BUS_IWDG5, _CK_ICN_LS_MCU, 0, _RCC_IWDG5CFGR, 1),
	GATE(_CK_WWDG2, CK_BUS_WWDG2, _CK_ICN_LS_MCU, 0, _RCC_WWDG2CFGR, 1),

	GATE(_CK_DDRCP, CK_BUS_DDR1, _CK_ICN_DDR, 0, _RCC_DDRCPCFGR, 1),

	GATE(_CK_LTDC, CK_BUS_LTDC, _CK_ICN_DISPLAY, 0, _RCC_LTDCCFGR, 1),
	GATE(_CK_DCMIPP, CK_BUS_DCMIPP, _CK_ICN_DISPLAY, 0, _RCC_DCMIPPCFGR, 1),

	/* WARNING 2 CLOCKS FOR ONE GATE */
	GATE(_CK_USB2OHCI, CK_BUS_USB2OHCI, _CK_ICN_HSL, 0, _RCC_USB2CFGR, 1),
	GATE(_CK_USB2EHCI, CK_BUS_USB2EHCI, _CK_ICN_HSL, 0, _RCC_USB2CFGR, 1),

	GATE(_CK_USB3DRD, CK_BUS_USB3DRD, _CK_ICN_HSL, 0, _RCC_USB3DRDCFGR, 1),

	GATE(_CK_BSEC, CK_BUS_BSEC, _CK_ICN_APB3, 0, _RCC_BSECCFGR, 1),
	GATE(_CK_IWDG1, CK_BUS_IWDG1, _CK_ICN_APB3, 0, _RCC_IWDG1CFGR, 1),
	GATE(_CK_IWDG2, CK_BUS_IWDG2, _CK_ICN_APB3, 0, _RCC_IWDG2CFGR, 1),
	GATE(_CK_IWDG3, CK_BUS_IWDG3, _CK_ICN_APB3, 0, _RCC_IWDG3CFGR, 1),
	GATE(_CK_IWDG4, CK_BUS_IWDG4, _CK_ICN_APB3, 0, _RCC_IWDG4CFGR, 1),
	GATE(_CK_WWDG1, CK_BUS_WWDG1, _CK_ICN_APB3, 0, _RCC_WWDG1CFGR, 1),
	GATE(_CK_VREF, CK_BUS_VREF, _CK_ICN_APB3, 0, _RCC_VREFCFGR, 1),
	GATE(_CK_TMPSENS, CK_BUS_TMPSENS, _CK_ICN_APB3, 0, _RCC_TMPSENSCFGR, 1),
	GATE(_CK_SERC, CK_BUS_SERC, _CK_ICN_APB3, 0, _RCC_SERCCFGR, 1),
	GATE(_CK_HDP, CK_BUS_HDP, _CK_ICN_APB3, 0, _RCC_HDPCFGR, 1),
	GATE(_CK_IS2M, CK_BUS_IS2M, _CK_ICN_APB3, 0, _RCC_IS2MCFGR, 1),

	GATE(_CK_DDRCAPB, CK_BUS_DDRC, _CK_ICN_APB4, 0, _RCC_DDRCAPBCFGR, 1),
	GATE(_CK_DDR, CK_BUS_DDRCFG, _CK_ICN_APB4, 0, _RCC_DDRCFGR, 1),
	GATE(_CK_GICV2M, CK_BUS_GICV2M, _CK_ICN_APB4, 0, _RCC_GICV2MCFGR, 1),
	GATE(_CK_BUSPERFM, CK_BUS_BUSPERFM, _CK_ICN_APB4, 0, _RCC_BUSPERFMCFGR, 1),
	GATE(_CK_STM500, CK_BUS_STM500, _CK_ICN_APBDBG, 0, _RCC_STM500CFGR, 1),

	GATE(_CK_VDEC, CK_BUS_VDEC, _CK_ICN_VID, 0, _RCC_VDECCFGR, 1),
	GATE(_CK_VENC, CK_BUS_VENC, _CK_ICN_VID, 0, _RCC_VENCCFGR, 1),

	GATE(_CK_TIM2, CK_KER_TIM2, _TIMG1_CK, 0, _RCC_TIM2CFGR, 1),
	GATE(_CK_TIM3, CK_KER_TIM3, _TIMG1_CK, 0, _RCC_TIM3CFGR, 1),
	GATE(_CK_TIM4, CK_KER_TIM4, _TIMG1_CK, 0, _RCC_TIM4CFGR, 1),
	GATE(_CK_TIM5, CK_KER_TIM5, _TIMG1_CK, 0, _RCC_TIM5CFGR, 1),
	GATE(_CK_TIM6, CK_KER_TIM6, _TIMG1_CK, 0, _RCC_TIM6CFGR, 1),
	GATE(_CK_TIM7, CK_KER_TIM7, _TIMG1_CK, 0, _RCC_TIM7CFGR, 1),
	GATE(_CK_TIM10, CK_KER_TIM10, _TIMG1_CK, 0, _RCC_TIM10CFGR, 1),
	GATE(_CK_TIM11, CK_KER_TIM11, _TIMG1_CK, 0, _RCC_TIM11CFGR, 1),
	GATE(_CK_TIM12, CK_KER_TIM12, _TIMG1_CK, 0, _RCC_TIM12CFGR, 1),
	GATE(_CK_TIM13, CK_KER_TIM13, _TIMG1_CK, 0, _RCC_TIM13CFGR, 1),
	GATE(_CK_TIM14, CK_KER_TIM14, _TIMG1_CK, 0, _RCC_TIM14CFGR, 1),

	GATE(_CK_TIM1, CK_KER_TIM1, _TIMG2_CK, 0, _RCC_TIM1CFGR, 1),
	GATE(_CK_TIM8, CK_KER_TIM8, _TIMG2_CK, 0, _RCC_TIM8CFGR, 1),
	GATE(_CK_TIM15, CK_KER_TIM15, _TIMG2_CK, 0, _RCC_TIM15CFGR, 1),
	GATE(_CK_TIM16, CK_KER_TIM16, _TIMG2_CK, 0, _RCC_TIM16CFGR, 1),
	GATE(_CK_TIM17, CK_KER_TIM17, _TIMG2_CK, 0, _RCC_TIM17CFGR, 1),
	GATE(_CK_TIM20, CK_KER_TIM20, _TIMG2_CK, 0, _RCC_TIM20CFGR, 1),

	GATE(_CK_LPTIM1, CK_KER_LPTIM1, _CK_FLEXGEN_07, 0, _RCC_LPTIM1CFGR, 1),
	GATE(_CK_LPTIM2, CK_KER_LPTIM2, _CK_FLEXGEN_07, 0, _RCC_LPTIM2CFGR, 1),
	GATE(_CK_USART2, CK_KER_USART2, _CK_FLEXGEN_08, 0, _RCC_USART2CFGR, 1),
	GATE(_CK_UART4, CK_KER_UART4, _CK_FLEXGEN_08, 0, _RCC_UART4CFGR, 1),
	GATE(_CK_USART3, CK_KER_USART3, _CK_FLEXGEN_09, 0, _RCC_USART3CFGR, 1),
	GATE(_CK_UART5, CK_KER_UART5, _CK_FLEXGEN_09, 0, _RCC_UART5CFGR, 1),
	GATE(_CK_SPI2, CK_KER_SPI2, _CK_FLEXGEN_10, 0, _RCC_SPI2CFGR, 1),
	GATE(_CK_SPI3, CK_KER_SPI3, _CK_FLEXGEN_10, 0, _RCC_SPI3CFGR, 1),
	GATE(_CK_SPDIFRX, CK_KER_SPDIFRX, _CK_FLEXGEN_11, 0, _RCC_SPDIFRXCFGR, 1),
	GATE(_CK_I2C1, CK_KER_I2C1, _CK_FLEXGEN_12, 0, _RCC_I2C1CFGR, 1),
	GATE(_CK_I2C2, CK_KER_I2C2, _CK_FLEXGEN_12, 0, _RCC_I2C2CFGR, 1),
	GATE(_CK_I3C1, CK_KER_I3C1, _CK_FLEXGEN_12, 0, _RCC_I3C1CFGR, 1),
	GATE(_CK_I3C2, CK_KER_I3C2, _CK_FLEXGEN_12, 0, _RCC_I3C2CFGR, 1),
	GATE(_CK_I2C3, CK_KER_I2C3, _CK_FLEXGEN_13, 0, _RCC_I2C3CFGR, 1),
	GATE(_CK_I2C5, CK_KER_I2C5, _CK_FLEXGEN_13, 0, _RCC_I2C5CFGR, 1),
	GATE(_CK_I3C3, CK_KER_I3C3, _CK_FLEXGEN_13, 0, _RCC_I3C3CFGR, 1),
	GATE(_CK_I2C4, CK_KER_I2C4, _CK_FLEXGEN_14, 0, _RCC_I2C4CFGR, 1),
	GATE(_CK_I2C6, CK_KER_I2C6, _CK_FLEXGEN_14, 0, _RCC_I2C6CFGR, 1),
	GATE(_CK_I2C7, CK_KER_I2C7, _CK_FLEXGEN_15, 0, _RCC_I2C7CFGR, 1),
	GATE(_CK_SPI1, CK_KER_SPI1, _CK_FLEXGEN_16, 0, _RCC_SPI1CFGR, 1),
	GATE(_CK_SPI4, CK_KER_SPI4, _CK_FLEXGEN_17, 0, _RCC_SPI4CFGR, 1),
	GATE(_CK_SPI5, CK_KER_SPI5, _CK_FLEXGEN_17, 0, _RCC_SPI5CFGR, 1),
	GATE(_CK_SPI6, CK_KER_SPI6, _CK_FLEXGEN_18, 0, _RCC_SPI6CFGR, 1),
	GATE(_CK_SPI7, CK_KER_SPI7, _CK_FLEXGEN_18, 0, _RCC_SPI7CFGR, 1),
	GATE(_CK_USART1, CK_KER_USART1, _CK_FLEXGEN_19, 0, _RCC_USART1CFGR, 1),
	GATE(_CK_USART6, CK_KER_USART6, _CK_FLEXGEN_20, 0, _RCC_USART6CFGR, 1),
	GATE(_CK_UART7, CK_KER_UART7, _CK_FLEXGEN_21, 0, _RCC_UART7CFGR, 1),
	GATE(_CK_UART8, CK_KER_UART8, _CK_FLEXGEN_21, 0, _RCC_UART8CFGR, 1),
	GATE(_CK_UART9, CK_KER_UART9, _CK_FLEXGEN_22, 0, _RCC_UART9CFGR, 1),
	GATE(_CK_MDF1, CK_KER_MDF1, _CK_FLEXGEN_23, 0, _RCC_MDF1CFGR, 1),
	GATE(_CK_SAI1, CK_KER_SAI1, _CK_FLEXGEN_23, 0, _RCC_SAI1CFGR, 1),
	GATE(_CK_SAI2, CK_KER_SAI2, _CK_FLEXGEN_24, 0, _RCC_SAI2CFGR, 1),
	GATE(_CK_SAI3, CK_KER_SAI3, _CK_FLEXGEN_25, 0, _RCC_SAI3CFGR, 1),
	GATE(_CK_SAI4, CK_KER_SAI4, _CK_FLEXGEN_25, 0, _RCC_SAI4CFGR, 1),
	GATE(_CK_FDCAN, CK_KER_FDCAN, _CK_FLEXGEN_26, 0, _RCC_FDCANCFGR, 1),

	/* WARNING 2 CLOCKS FOR ONE GATE */
	GATE(_CK_DSIBLANE, CK_KER_DSIBLANE, _CK_FLEXGEN_27, 0, _RCC_DSICFGR, 1),
	GATE(_CK_DSIPHY, CK_KER_DSIPHY, _CK_FLEXGEN_28, 0, _RCC_DSICFGR, 1),

	/* WARNING 3 CLOCKS FOR ONE GATE */
	GATE(_CK_CSI2, CK_KER_CSI2, _CK_FLEXGEN_29, 0, _RCC_CSI2CFGR, 1),
	GATE(_CK_CSI2TXESC, CK_KER_CSI2TXESC, _CK_FLEXGEN_30, 0, _RCC_CSI2CFGR, 1),
	GATE(_CK_CSI2PHY, CK_KER_CSI2PHY, _CK_FLEXGEN_31, 0, _RCC_CSI2CFGR, 1),
	GATE(_CK_LVDSPHY, CK_KER_LVDSPHY, _CK_FLEXGEN_32, 0, _RCC_LVDSCFGR, 1),
	GATE(_CK_STGEN, CK_KER_STGEN, _CK_FLEXGEN_33, 0, _RCC_STGENCFGR, 1),
	GATE(_CK_USB3PCIEPHY, CK_KER_USB3PCIEPHY, _CK_FLEXGEN_34, 0, _RCC_USB3PCIEPHYCFGR, 1),
	GATE(_CK_USBTC, CK_KER_USB3TC, _CK_FLEXGEN_35, 0, _RCC_USBTCCFGR, 1),
	GATE(_CK_I3C4, CK_KER_I3C4, _CK_FLEXGEN_36, 0, _RCC_I3C4CFGR, 1),
	GATE(_CK_SPI8, CK_KER_SPI8, _CK_FLEXGEN_37, 0, _RCC_SPI8CFGR, 1),
	GATE(_CK_I2C8, CK_KER_I2C8, _CK_FLEXGEN_38, 0, _RCC_I2C8CFGR, 1),
	GATE(_CK_LPUART1, CK_KER_LPUART1, _CK_FLEXGEN_39, 0, _RCC_LPUART1CFGR, 1),
	GATE(_CK_LPTIM3, CK_KER_LPTIM3, _CK_FLEXGEN_40, 0, _RCC_LPTIM3CFGR, 1),
	GATE(_CK_LPTIM4, CK_KER_LPTIM4, _CK_FLEXGEN_41, 0, _RCC_LPTIM4CFGR, 1),
	GATE(_CK_LPTIM5, CK_KER_LPTIM5, _CK_FLEXGEN_41, 0, _RCC_LPTIM5CFGR, 1),
	GATE(_CK_ADF1, CK_KER_ADF1, _CK_FLEXGEN_42, 0, _RCC_ADF1CFGR, 1),
	GATE(_CK_TSDBG, CK_KER_TSDBG, _CK_FLEXGEN_43, 0, _RCC_DBGCFGR, 8),
	GATE(_CK_TPIU, CK_KER_TPIU, _CK_FLEXGEN_44, 0, _RCC_DBGCFGR, 9),
	GATE(_CK_ETR, CK_BUS_ETR, _CK_FLEXGEN_45, 0, _RCC_ETRCFGR, 1),
	GATE(_CK_ADC12, CK_KER_ADC12, _CK_FLEXGEN_46, 0, _RCC_ADC12CFGR, 1),
	GATE(_CK_ADC3, CK_KER_ADC3, _CK_FLEXGEN_47, 0, _RCC_ADC3CFGR, 1),
	GATE(_CK_OSPI1, CK_KER_OSPI1, _CK_FLEXGEN_48, 0, _RCC_OSPI1CFGR, 1),
	GATE(_CK_OSPI2, CK_KER_OSPI2, _CK_FLEXGEN_49, 0, _RCC_OSPI2CFGR, 1),
	GATE(_CK_FMC, CK_KER_FMC, _CK_FLEXGEN_50, 0, _RCC_FMCCFGR, 1),
	GATE(_CK_SDMMC1, CK_KER_SDMMC1, _CK_FLEXGEN_51, 0, _RCC_SDMMC1CFGR, 1),
	GATE(_CK_SDMMC2, CK_KER_SDMMC2, _CK_FLEXGEN_52, 0, _RCC_SDMMC2CFGR, 1),
	GATE(_CK_SDMMC3, CK_KER_SDMMC3, _CK_FLEXGEN_53, 0, _RCC_SDMMC3CFGR, 1),
	GATE(_CK_ETH1, CK_KER_ETH1, _CK_FLEXGEN_54, 0, _RCC_ETH1CFGR, 5),
	GATE(_CK_ETHSW, CK_KER_ETHSW, _CK_FLEXGEN_54, 0, _RCC_ETHSWCFGR, 5),
	GATE(_CK_ETH2, CK_KER_ETH2, _CK_FLEXGEN_55, 0, _RCC_ETH2CFGR, 5),
	GATE(_CK_ETH1PTP, CK_KER_ETH1PTP, _CK_FLEXGEN_56, 0, _RCC_ETH1CFGR, 1),
	GATE(_CK_ETH2PTP, CK_KER_ETH2PTP, _CK_FLEXGEN_56, 0, _RCC_ETH2CFGR, 1),
	GATE(_CK_USB2PHY1, CK_KER_USB2PHY1, _CK_FLEXGEN_57, 0, _RCC_USB2PHY1CFGR, 1),
	GATE(_CK_USB2PHY2, CK_KER_USB2PHY2, _CK_FLEXGEN_58, 0, _RCC_USB2PHY2CFGR, 1),
	GATE(_CK_ETHSWREF, CK_KER_ETHSWREF, _CK_FLEXGEN_60, 0, _RCC_ETHSWCFGR, 21),
	GATE(_CK_MCO1, CK_MCO1, _CK_FLEXGEN_61, 0, _RCC_MCO1CFGR, 8),
	GATE(_CK_MCO2, CK_MCO2, _CK_FLEXGEN_62, 0, _RCC_MCO2CFGR, 8),
};

static unsigned int refcounts_mp25[CK_LAST];


enum clksrc_id {
	CLKSRC_CA35SS,
	CLKSRC_PLL1,
	CLKSRC_PLL2,
	CLKSRC_PLL3,
	CLKSRC_PLL4,
	CLKSRC_PLL5,
	CLKSRC_PLL6,
	CLKSRC_PLL7,
	CLKSRC_PLL8,
	CLKSRC_XBAR_CHANNEL0,
	CLKSRC_XBAR_CHANNEL1,
	CLKSRC_XBAR_CHANNEL2,
	CLKSRC_XBAR_CHANNEL3,
	CLKSRC_XBAR_CHANNEL4,
	CLKSRC_XBAR_CHANNEL5,
	CLKSRC_XBAR_CHANNEL6,
	CLKSRC_XBAR_CHANNEL7,
	CLKSRC_XBAR_CHANNEL8,
	CLKSRC_XBAR_CHANNEL9,
	CLKSRC_XBAR_CHANNEL10,
	CLKSRC_XBAR_CHANNEL11,
	CLKSRC_XBAR_CHANNEL12,
	CLKSRC_XBAR_CHANNEL13,
	CLKSRC_XBAR_CHANNEL14,
	CLKSRC_XBAR_CHANNEL15,
	CLKSRC_XBAR_CHANNEL16,
	CLKSRC_XBAR_CHANNEL17,
	CLKSRC_XBAR_CHANNEL18,
	CLKSRC_XBAR_CHANNEL19,
	CLKSRC_XBAR_CHANNEL20,
	CLKSRC_XBAR_CHANNEL21,
	CLKSRC_XBAR_CHANNEL22,
	CLKSRC_XBAR_CHANNEL23,
	CLKSRC_XBAR_CHANNEL24,
	CLKSRC_XBAR_CHANNEL25,
	CLKSRC_XBAR_CHANNEL26,
	CLKSRC_XBAR_CHANNEL27,
	CLKSRC_XBAR_CHANNEL28,
	CLKSRC_XBAR_CHANNEL29,
	CLKSRC_XBAR_CHANNEL30,
	CLKSRC_XBAR_CHANNEL31,
	CLKSRC_XBAR_CHANNEL32,
	CLKSRC_XBAR_CHANNEL33,
	CLKSRC_XBAR_CHANNEL34,
	CLKSRC_XBAR_CHANNEL35,
	CLKSRC_XBAR_CHANNEL36,
	CLKSRC_XBAR_CHANNEL37,
	CLKSRC_XBAR_CHANNEL38,
	CLKSRC_XBAR_CHANNEL39,
	CLKSRC_XBAR_CHANNEL40,
	CLKSRC_XBAR_CHANNEL41,
	CLKSRC_XBAR_CHANNEL42,
	CLKSRC_XBAR_CHANNEL43,
	CLKSRC_XBAR_CHANNEL44,
	CLKSRC_XBAR_CHANNEL45,
	CLKSRC_XBAR_CHANNEL46,
	CLKSRC_XBAR_CHANNEL47,
	CLKSRC_XBAR_CHANNEL48,
	CLKSRC_XBAR_CHANNEL49,
	CLKSRC_XBAR_CHANNEL50,
	CLKSRC_XBAR_CHANNEL51,
	CLKSRC_XBAR_CHANNEL52,
	CLKSRC_XBAR_CHANNEL53,
	CLKSRC_XBAR_CHANNEL54,
	CLKSRC_XBAR_CHANNEL55,
	CLKSRC_XBAR_CHANNEL56,
	CLKSRC_XBAR_CHANNEL57,
	CLKSRC_XBAR_CHANNEL58,
	CLKSRC_XBAR_CHANNEL59,
	CLKSRC_XBAR_CHANNEL60,
	CLKSRC_XBAR_CHANNEL61,
	CLKSRC_XBAR_CHANNEL62,
	CLKSRC_XBAR_CHANNEL63,
	CLKSRC_RTC,
	CLKSRC_MCO1,
	CLKSRC_MCO2,
	// C3SYSTICKSEL:
	// CSIFREQSEL:
	// D3PERCKSEL[1:0]:
	// ADC12KERSEL:
	// ADC3KERSEL[1:0]:
	// USB2PHY1CKREFSEL:
	// USB2PHY2CKREFSEL:
	// USB3PCIEPHYCKREFSEL:
	// DSIPHYCKREFSEL:
	// DSIBLSEL:
	// LVDSPHYCKREFSEL:
	// TMPSENSKERSEL[1:0]:

	CLKSRC_NB
};

void clk_stm32mp2_debug_display_osc_dt_cfg(struct stm32mp2_clk_platdata *pdata)
{
	int nb = pdata->npll;
	int i;

	printf("\nNumber of PLLs = %d\n", nb);

	for (i = 0; i < nb; i++) {
		struct stm32_pll_dt_cfg *pll = &pdata->pll[i];

		printf("PLL%d : %s ", i + 1, pll->enabled ? "enabled" : "disabled");

		if (pll->enabled == false) {
			goto end_display_osc_dt_cfg;
		}

		printf("cfg = < %d %d %d %d > ", pll->cfg[0], pll->cfg[1], pll->cfg[2], pll->cfg[3]);

		printf("frac = %d ", pll->frac);

		if (pll->csg_enabled == false) {
			goto end_display_osc_dt_cfg;
		}

		printf("csg = < %d %d %d > ", pll->csg[0], pll->csg[1], pll->csg[2]);

end_display_osc_dt_cfg:
		printf("\n");
	}
}

void clk_stm32mp2_debug_display_pll_dt_cfg(struct stm32mp2_clk_platdata *pdata)
{
	int nb = pdata->nosci;
	int i;

	printf("\nNumber of oscillators = %d\n", nb);

	for (i = 0; i < nb; i++) {
		struct stm32_osci_dt_cfg *osc = &pdata->osci[i];

		printf("%s %ld bypass = %d digbyp = %d css = %d drive = %d\n", stm32mp_osc_node_label[i]
				, osc->freq
				, osc->bypass
				, osc->digbyp
				, osc->css
				, osc->drive);

	}
}

void clk_stm32mp2_debug_display_pdata(void)
{
	struct stm32_clk_priv *priv = clk_stm32_get_priv();
	struct stm32mp2_clk_platdata *pdata = priv->pdata;

	clk_stm32mp2_debug_display_osc_dt_cfg(pdata);
	clk_stm32mp2_debug_display_pll_dt_cfg(pdata);
}

static uintptr_t stm32mp2_pll_cfgr(unsigned int pll_id, unsigned int idx)
{
	const struct clk_pll *pll = stm32mp2_pll_ref(pll_id);
	uintptr_t rcc_base = clk_stm32_get_rcc_base();

	assert((idx >= 1) && (idx <= ARRAY_SIZE(pll_cfgr_offsets)));

	return rcc_base + pll->pllxcfgr1 + pll_cfgr_offsets[idx - 1];
}

static void stm32mp2_clk_muxsel_on_hsi(struct stm32_clk_priv *priv)
{
	mmio_clrbits_32(priv->base + _RCC_MUXSELCFGR,
			_RCC_MUXSELCFGR_MUXSEL0_MASK |
			_RCC_MUXSELCFGR_MUXSEL1_MASK |
			_RCC_MUXSELCFGR_MUXSEL2_MASK |
			_RCC_MUXSELCFGR_MUXSEL3_MASK |
			_RCC_MUXSELCFGR_MUXSEL4_MASK |
			_RCC_MUXSELCFGR_MUXSEL5_MASK |
			_RCC_MUXSELCFGR_MUXSEL6_MASK |
			_RCC_MUXSELCFGR_MUXSEL7_MASK);
}

static void stm32mp2_clk_xbar_on_hsi(struct stm32_clk_priv *priv)
{
	uintptr_t xbar0cfgr = priv->base + _RCC_XBAR0CFGR;
	uint32_t i;

	for (i = 0; i < XBAR_CHANNEL_NB; i++) {
		mmio_clrsetbits_32(xbar0cfgr + (0x4 * i),
				   _RCC_XBAR0CFGR_XBAR0SEL_MASK,
				   XBAR_SRC_HSI);
	}
}

static void stm32mp2_pll_mux_cfg(unsigned int pll_id, unsigned int pll_src)
{
	const struct clk_pll *pll = stm32mp2_pll_ref(pll_id);
	uintptr_t rcc_base = clk_stm32_get_rcc_base();
	uintptr_t pllxcfgr1 = stm32mp2_pll_cfgr(pll_id, 1);
	uint64_t timeout;
	uint16_t muxsel_shift = pll->muxsel;

	mmio_clrsetbits_32(rcc_base + _RCC_MUXSELCFGR,
			   _RCC_MUXSELCFGR_MUXSEL0_MASK << muxsel_shift,
			   pll_src << muxsel_shift);

	/* PLL1 has no CKREFST */
	if (pll_id == _PLL1) {
		return;
	}

	/* TODO: WORKAROUND: PLL2, PLL3 CKREFST doesn't work */
	if (pll_id >= _PLL2 && pll_id <= _PLL3) {
		return;
	}

	timeout = timeout_init_us(CLKSRC_TIMEOUT);
	while ((mmio_read_32(pllxcfgr1) & _RCC_PLLxCFGR1_CKREFST) !=
	       _RCC_PLLxCFGR1_CKREFST) {
		if (timeout_elapsed(timeout)) {
			ERROR("PLL%d ref clock (%s) not started\n",
			      pll_id + 1, stm32mp_osc_node_label[pll_src]);
			//panic();
		}
	}
}

#ifdef M33_TDCID_REVIEW_NEEDED
/* TODO: MOVE THIS FUNCTION A35 ONLY */
static void stm32mp2_a35_ss_on_hsi(void)
{
	uintptr_t a35_ss_address = A35SSC_BASE;
	uintptr_t chgclkreq_reg = a35_ss_address + A35_SS_CHGCLKREQ;
	uintptr_t pll_enable_reg = a35_ss_address + A35_SS_PLL_ENABLE;
	uint64_t timeout;

	if ((mmio_read_32(chgclkreq_reg) & A35_SS_CHGCLKREQ_ARM_CHGCLKACK) ==
	    A35_SS_CHGCLKREQ_ARM_CHGCLKACK) {
		/* Nothing to do, clock source is already set on bypass clock */
		return;
	}

	mmio_setbits_32(chgclkreq_reg, A35_SS_CHGCLKREQ_ARM_CHGCLKREQ);

	timeout = timeout_init_us(CLKSRC_TIMEOUT);
	while ((mmio_read_32(chgclkreq_reg) & A35_SS_CHGCLKREQ_ARM_CHGCLKACK) !=
	       A35_SS_CHGCLKREQ_ARM_CHGCLKACK) {
		if (timeout_elapsed(timeout)) {
			ERROR("Cannot switch A35 to bypass clock\n");
			panic();
		}
	}

	mmio_clrbits_32(pll_enable_reg, A35_SS_PLL_ENABLE_NRESET_SWPLL_FF);
}

/* TODO: MOVE THIS FUNCTION A35 ONLY */
static void stm32mp2_a35_pll1_start(void)
{
	uintptr_t a35_ss_address = A35SSC_BASE;
	uintptr_t pll_enable_reg = a35_ss_address + A35_SS_PLL_ENABLE;

	mmio_setbits_32(pll_enable_reg, A35_SS_PLL_ENABLE_PD);
}

/* TODO: MOVE THIS FUNCTION A35 ONLY */
static int stm32mp2_a35_pll1_output(void)
{
	uintptr_t a35_ss_address = A35SSC_BASE;
	uintptr_t chgclkreq_reg = a35_ss_address + A35_SS_CHGCLKREQ;
	uintptr_t pll_enable_reg = a35_ss_address + A35_SS_PLL_ENABLE;
	uint64_t timeout = timeout_init_us(PLLRDY_TIMEOUT);

	/* Wait PLL lock */
	while ((mmio_read_32(pll_enable_reg) & A35_SS_PLL_ENABLE_LOCKP) == 0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("PLL1 start failed @ 0x%lx: 0x%x\n",
			      pll_enable_reg, mmio_read_32(pll_enable_reg));
			return -ETIMEDOUT;
		}
	}

	/* De-assert reset on PLL output clock path */
	mmio_setbits_32(pll_enable_reg, A35_SS_PLL_ENABLE_NRESET_SWPLL_FF);

	/* Switch CPU clock to PLL clock */
	mmio_clrbits_32(chgclkreq_reg, A35_SS_CHGCLKREQ_ARM_CHGCLKREQ);

	/* Wait for clock change acknowledge */
	timeout = timeout_init_us(CLKSRC_TIMEOUT);
	while ((mmio_read_32(chgclkreq_reg) & A35_SS_CHGCLKREQ_ARM_CHGCLKACK) !=
	       0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("CA35SS switch to PLL1 failed @ 0x%lx: 0x%x\n",
			      chgclkreq_reg, mmio_read_32(chgclkreq_reg));
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void stm32mp2_a35_pll1_config(uint32_t fbdiv, uint32_t refdiv, uint32_t postdiv1, uint32_t postdiv2)
{
	uintptr_t a35_ss_address = A35SSC_BASE;
	uintptr_t pll_freq1_reg = a35_ss_address + A35_SS_PLL_FREQ1;
	uintptr_t pll_freq2_reg = a35_ss_address + A35_SS_PLL_FREQ2;

	mmio_clrsetbits_32(pll_freq1_reg, A35_SS_PLL_FREQ1_REFDIV_MASK,
			   (refdiv << A35_SS_PLL_FREQ1_REFDIV_SHIFT) &
			   A35_SS_PLL_FREQ1_REFDIV_MASK);

	mmio_clrsetbits_32(pll_freq1_reg, A35_SS_PLL_FREQ1_FBDIV_MASK,
			   (fbdiv << A35_SS_PLL_FREQ1_FBDIV_SHIFT) &
			   A35_SS_PLL_FREQ1_FBDIV_MASK);

	mmio_clrsetbits_32(pll_freq2_reg, A35_SS_PLL_FREQ2_POSTDIV1_MASK,
			   (postdiv1 <<
			   A35_SS_PLL_FREQ2_POSTDIV1_SHIFT) &
			   A35_SS_PLL_FREQ2_POSTDIV1_MASK);

	mmio_clrsetbits_32(pll_freq2_reg, A35_SS_PLL_FREQ2_POSTDIV2_MASK,
			   (postdiv2 <<
			   A35_SS_PLL_FREQ2_POSTDIV2_SHIFT) &
			   A35_SS_PLL_FREQ2_POSTDIV2_MASK);
}
#else
static void stm32mp2_a35_pll1_config(uint32_t fbdiv, uint32_t refdiv,
		uint32_t postdiv1, uint32_t postdiv2) {}
static void stm32mp2_a35_pll1_start(void) {}
static int stm32mp2_a35_pll1_output(void) {}
static void stm32mp2_a35_ss_on_hsi(void) {}
#endif /* M33_TDCID_REVIEW_NEEDED */

static void stm32mp2_pll_start(enum pll_id pll_id)
{
	mmio_setbits_32(stm32mp2_pll_cfgr(pll_id, 1), _RCC_PLLxCFGR1_PLLEN);
}

static int stm32mp2_pll_output(enum pll_id pll_id, bool spread_spectrum)
{
	uintptr_t pllxcfgr1 = stm32mp2_pll_cfgr(pll_id, 1);
	uint64_t timeout = timeout_init_us(PLLRDY_TIMEOUT);

	/* Wait PLL lock */
	while ((mmio_read_32(pllxcfgr1) & _RCC_PLLxCFGR1_PLLRDY) == 0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("PLL%d start failed @ 0x%lx: 0x%x\n",
			      pll_id, pllxcfgr1, mmio_read_32(pllxcfgr1));
			return -ETIMEDOUT;
		}
	}

	if (spread_spectrum) {
		mmio_clrbits_32(pllxcfgr1, _RCC_PLLxCFGR1_SSMODRST);
	}

	return 0;
}

static int stm32mp2_pll_stop(enum pll_id pll_id)
{
	uintptr_t pllxcfgr1 = stm32mp2_pll_cfgr(pll_id, 1);
	uint64_t timeout;

	/* Stop PLL */
	mmio_clrbits_32(pllxcfgr1, _RCC_PLLxCFGR1_PLLEN);

	timeout = timeout_init_us(PLLRDY_TIMEOUT);
	/* Wait PLL stopped */
	while ((mmio_read_32(pllxcfgr1) & _RCC_PLLxCFGR1_PLLRDY) != 0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("PLL%d stop failed @ 0x%lx: 0x%x\n",
			      pll_id, pllxcfgr1, mmio_read_32(pllxcfgr1));
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void stm32mp2_pll_config_output(enum pll_id pll_id, uint32_t *pllcfg)
{
	uintptr_t pllxcfgr4 = stm32mp2_pll_cfgr(pll_id, 4);

	if ((pllcfg[POSTDIV1] == 0U) || (pllcfg[POSTDIV2] == 0U)) {
		/* Bypass mode */
		mmio_setbits_32(pllxcfgr4, _RCC_PLLxCFGR4_BYPASS);
		mmio_clrbits_32(pllxcfgr4, _RCC_PLLxCFGR4_FOUTPOSTDIVEN);
	} else {
		mmio_clrbits_32(pllxcfgr4, _RCC_PLLxCFGR4_BYPASS);
		mmio_setbits_32(pllxcfgr4, _RCC_PLLxCFGR4_FOUTPOSTDIVEN);
	}
}

static int stm32mp2_pll_config(enum pll_id pll_id, uint32_t *pllcfg, uint32_t fracv)
{
	struct stm32_clk_priv *priv = clk_stm32_get_priv();
	uintptr_t pllxcfgr1 = stm32mp2_pll_cfgr(pll_id, 1);
	uintptr_t pllxcfgr2 = stm32mp2_pll_cfgr(pll_id, 2);
	uintptr_t pllxcfgr3 = stm32mp2_pll_cfgr(pll_id, 3);
	uintptr_t pllxcfgr4 = stm32mp2_pll_cfgr(pll_id, 4);
	uintptr_t pllxcfgr6 = stm32mp2_pll_cfgr(pll_id, 6);
	uintptr_t pllxcfgr7 = stm32mp2_pll_cfgr(pll_id, 7);
	unsigned long refclk;

	uint32_t pll_idx[] = { _CK_PLL1, _CK_PLL2, _CK_PLL3, _CK_PLL4,
			       _CK_PLL5, _CK_PLL6, _CK_PLL7, _CK_PLL8 };

	refclk = _clk_stm32_get_rate(priv, pll_idx[pll_id]);

	if (fracv == 0U) {
		/* PLL in integer mode */

		/*
		 * No need to check max clock, as oscillator reference clocks
		 * will always be less than 1.2GHz
		 */
		if (refclk < PLL_REFCLK_MIN) {
			panic();
		}

		mmio_clrbits_32(pllxcfgr3, _RCC_PLLxCFGR3_FRACIN_MASK);
		mmio_clrbits_32(pllxcfgr4, _RCC_PLLxCFGR4_DSMEN);
		mmio_clrbits_32(pllxcfgr3, _RCC_PLLxCFGR3_DACEN);
		mmio_setbits_32(pllxcfgr3, _RCC_PLLxCFGR3_SSCGDIS);
		mmio_setbits_32(pllxcfgr1, _RCC_PLLxCFGR1_SSMODRST);
	} else {
		/* PLL in frac mode */

		/*
		 * No need to check max clock, as oscillator reference clocks
		 * will always be less than 1.2GHz
		 */
		if (refclk < PLL_FRAC_REFCLK_MIN) {
			panic();
		}

		mmio_clrsetbits_32(pllxcfgr3, _RCC_PLLxCFGR3_FRACIN_MASK,
				   fracv & _RCC_PLLxCFGR3_FRACIN_MASK);
		mmio_setbits_32(pllxcfgr3, _RCC_PLLxCFGR3_SSCGDIS);
		mmio_setbits_32(pllxcfgr4, _RCC_PLLxCFGR4_DSMEN);
	}

	assert(pllcfg[REFDIV] != 0U);

	mmio_clrsetbits_32(pllxcfgr2, _RCC_PLLxCFGR2_FBDIV_MASK,
			   (pllcfg[FBDIV] << _RCC_PLLxCFGR2_FBDIV_SHIFT) &
			   _RCC_PLLxCFGR2_FBDIV_MASK);
	mmio_clrsetbits_32(pllxcfgr2, _RCC_PLLxCFGR2_FREFDIV_MASK,
			   pllcfg[REFDIV] & _RCC_PLLxCFGR2_FREFDIV_MASK);
	mmio_clrsetbits_32(pllxcfgr6, _RCC_PLLxCFGR6_POSTDIV1_MASK,
			   pllcfg[POSTDIV1] & _RCC_PLLxCFGR6_POSTDIV1_MASK);
	mmio_clrsetbits_32(pllxcfgr7, _RCC_PLLxCFGR7_POSTDIV2_MASK,
			   pllcfg[POSTDIV2] & _RCC_PLLxCFGR7_POSTDIV2_MASK);

	stm32mp2_pll_config_output(pll_id, pllcfg);

	return 0;
}

static void stm32mp2_pll_csg(enum pll_id pll_id, uint32_t *csg)
{
	uintptr_t pllxcfgr1 = stm32mp2_pll_cfgr(pll_id, 1);
	uintptr_t pllxcfgr3 = stm32mp2_pll_cfgr(pll_id, 3);
	uintptr_t pllxcfgr4 = stm32mp2_pll_cfgr(pll_id, 4);
	uintptr_t pllxcfgr5 = stm32mp2_pll_cfgr(pll_id, 5);

	mmio_clrsetbits_32(pllxcfgr5, _RCC_PLLxCFGR5_DIVVAL_MASK,
			   csg[DIVVAL] & _RCC_PLLxCFGR5_DIVVAL_MASK);
	mmio_clrsetbits_32(pllxcfgr5, _RCC_PLLxCFGR5_SPREAD_MASK,
			   (csg[SPREAD] << _RCC_PLLxCFGR5_SPREAD_SHIFT) &
			   _RCC_PLLxCFGR5_SPREAD_MASK);

	if (csg[DOWNSPREAD] != 0) {
		mmio_setbits_32(pllxcfgr3, _RCC_PLLxCFGR3_DOWNSPREAD);
	} else {
		mmio_clrbits_32(pllxcfgr3, _RCC_PLLxCFGR3_DOWNSPREAD);
	}

	mmio_clrbits_32(pllxcfgr3, _RCC_PLLxCFGR3_SSCGDIS);

	mmio_clrbits_32(pllxcfgr1, _RCC_PLLxCFGR1_PLLEN);
	udelay(1);

	mmio_setbits_32(pllxcfgr4, _RCC_PLLxCFGR4_DSMEN);
	mmio_setbits_32(pllxcfgr3, _RCC_PLLxCFGR3_DACEN);
}

static int wait_predivsr(uint16_t channel)
{
	uintptr_t rcc_base = clk_stm32_get_rcc_base();
	uintptr_t previvsr;
	uint32_t channel_bit;
	uint64_t timeout;

	if (channel < __WORD_BIT) {
		previvsr = rcc_base + _RCC_PREDIVSR1;
		channel_bit = BIT(channel);
	} else {
		previvsr = rcc_base + _RCC_PREDIVSR2;
		channel_bit = BIT(channel - __WORD_BIT);
	}

	timeout = timeout_init_us(CLKDIV_TIMEOUT);
	while ((mmio_read_32(previvsr) & channel_bit) != 0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("Pre divider status: %x\n",
			      mmio_read_32(previvsr));
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int wait_findivsr(uint16_t channel)
{
	uintptr_t rcc_base = clk_stm32_get_rcc_base();
	uintptr_t finvivsr;
	uint32_t channel_bit;
	uint64_t timeout;

	if (channel < __WORD_BIT) {
		finvivsr = rcc_base + _RCC_FINDIVSR1;
		channel_bit = BIT(channel);
	} else {
		finvivsr = rcc_base + _RCC_FINDIVSR2;
		channel_bit = BIT(channel - __WORD_BIT);
	}

	timeout = timeout_init_us(CLKDIV_TIMEOUT);
	while ((mmio_read_32(finvivsr) & channel_bit) != 0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("Final divider status: %x\n",
			      mmio_read_32(finvivsr));
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int wait_xbar_sts(uint16_t channel)
{
	uintptr_t rcc_base = clk_stm32_get_rcc_base();
	uintptr_t xbar_cfgr = rcc_base + _RCC_XBAR0CFGR + (0x4 * channel);
	uint64_t timeout;

	timeout = timeout_init_us(CLKDIV_TIMEOUT);
	while ((mmio_read_32(xbar_cfgr) & _RCC_XBAR0CFGR_XBAR0STS) != 0U) {
		if (timeout_elapsed(timeout)) {
			ERROR("XBAR%dCFGR: %x\n", channel,
			      mmio_read_32(xbar_cfgr));
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void flexclkgen_config_channel(uint16_t channel, unsigned int clk_src,
				      unsigned int prediv, unsigned int findiv)
{
	uintptr_t rcc_base = clk_stm32_get_rcc_base();

	if (wait_predivsr(channel) != 0) {
		panic();
	}

	mmio_clrsetbits_32(rcc_base + _RCC_PREDIV0CFGR + (0x4 * channel),
			   _RCC_PREDIV0CFGR_PREDIV0_MASK,
			   prediv);

	if (wait_predivsr(channel) != 0) {
		panic();
	}

	if (wait_findivsr(channel) != 0) {
		panic();
	}

	mmio_clrsetbits_32(rcc_base + _RCC_FINDIV0CFGR + (0x4 * channel),
			   _RCC_FINDIV0CFGR_FINDIV0_MASK,
			   findiv);

	if (wait_findivsr(channel) != 0) {
		panic();
	}

	if (wait_xbar_sts(channel) != 0) {
		panic();
	}

	mmio_clrsetbits_32(rcc_base + _RCC_XBAR0CFGR + (0x4 * channel),
			   _RCC_XBARxCFGR_XBARxSEL_MASK,
			   clk_src);
	mmio_setbits_32(rcc_base + _RCC_XBAR0CFGR + (0x4 * channel),
			_RCC_XBARxCFGR_XBARxEN);

	if (wait_xbar_sts(channel) != 0) {
		panic();
	}
}

int stm32mp2_clk_flexgen_configure(struct stm32_clk_priv *priv)
{
	struct stm32mp2_clk_platdata *pdata = priv->pdata;
	uint16_t channel;
	uint32_t *clksrc;

	clksrc = &pdata->clksrc[CLKSRC_XBAR_CHANNEL0];

	for (channel = 0U; channel < XBAR_CHANNEL_NB; channel++) {
		unsigned int clk_src, prediv, prediv_val, findiv;

		assert((clksrc[channel] & XBAR_CKSRC_CHANNEL) == channel);

		clk_src = (clksrc[channel] & XBAR_CKSRC_SRC) >>
			XBAR_CKSRC_SRC_OFFSET;

		prediv_val = (clksrc[channel] & XBAR_CKSRC_PREDIV) >>
			XBAR_CKSRC_PREDIV_OFFSET;

		switch (prediv_val) {
		case 1:
			prediv = 0x00U;
			break;

		case 2:
			prediv = 0x01U;
			break;

		case 4:
			prediv = 0x02U;
			break;

		case 1024:
			prediv = 0x20U;
			break;

		default:
			ERROR("Wrong PREDIV value (%d)\n", prediv_val);
			panic();
			break;
		}

		findiv = (clksrc[channel] & XBAR_CKSRC_FINDIV) >>
			XBAR_CKSRC_FINDIV_OFFSET;

		/* TODO: check if channel can be reconfigured */
		flexclkgen_config_channel(channel, clk_src, prediv, findiv);
	}

	return 0;
}

static int __unused stm32mp2_clk_rtc_configure_src(struct stm32_clk_priv *priv)
{
	struct stm32mp2_clk_platdata *pdata=priv->pdata;
	uintptr_t address = priv->base + _RCC_BDCR;
	uint32_t clksrc = pdata->clksrc[CLKSRC_RTC];
	struct stm32_osci_dt_cfg *osci = &pdata->osci[OSC_LSE];

	if (((mmio_read_32(address) & _RCC_BDCR_RTCCKEN) == 0U) ||
	    (clksrc != (uint32_t)CLK_RTC_DISABLED)) {
		mmio_clrsetbits_32(address,
				   _RCC_BDCR_RTCSRC_MASK,
				   clksrc << _RCC_BDCR_RTCSRC_SHIFT);

		mmio_setbits_32(address, _RCC_BDCR_RTCCKEN);
	}

	clk_oscillator_set_css(priv, _CK_LSE, osci->css);

	return 0;
}

static void stm32_enable_oscillator_hse(struct stm32_clk_priv *priv)
{
	const struct clk_stm32 *clk = _clk_get(priv, _CK_HSE);
	struct clk_oscillator_data *osc_data = clk->clock_cfg;
	struct stm32mp2_clk_platdata *pdata = priv->pdata;
	struct stm32_osci_dt_cfg *osci = &pdata->osci[OSC_HSE];
	bool digbyp =  osci->digbyp;
	bool bypass = osci->bypass;
	bool css = osci->css;


	if (_clk_stm32_get_rate(priv, _CK_HSE) == 0U) {
		return;
	}

	clk_oscillator_set_bypass(priv, _CK_HSE, digbyp, bypass);

	_clk_stm32_gate_enable(priv, osc_data->gate_id);

	if (clk_oscillator_wait_ready_on(priv, _CK_HSE) != 0U) {
		panic();
	}

	clk_oscillator_set_css(priv, _CK_HSE, css);
}

static void stm32_enable_oscillator_lse(struct stm32_clk_priv *priv)
{
	const struct clk_stm32 *clk = _clk_get(priv, _CK_LSE);
	struct clk_oscillator_data *osc_data = clk->clock_cfg;
	struct stm32mp2_clk_platdata *pdata = priv->pdata;
	struct stm32_osci_dt_cfg *osci = &pdata->osci[OSC_LSE];

	bool digbyp =  osci->digbyp;
	bool bypass = osci->bypass;
	bool css = osci->css;
	uint8_t drive = osci->drive;

	if (_clk_stm32_get_rate(priv, _CK_LSE) == 0U) {
		return;
	}

	clk_oscillator_set_bypass(priv, _CK_LSE, digbyp, bypass);

	clk_oscillator_set_drive(priv, _CK_LSE, drive);

	_clk_stm32_gate_enable(priv, osc_data->gate_id);

	clk_oscillator_set_css(priv, _CK_LSE, css);
}

static void stm32mp_clk_oscillators_init(struct stm32_clk_priv *priv)
{
	struct stm32mp2_clk_platdata *pdata;

	pdata = priv->pdata;

	clk_stm32_oscillator_init(priv, _CK_HSE, pdata->osci[OSC_HSE].freq);
	clk_stm32_oscillator_init(priv, _CK_HSI, pdata->osci[OSC_HSI].freq);
	clk_stm32_oscillator_init(priv, _CK_LSI, pdata->osci[OSC_HSE].freq);
	clk_stm32_oscillator_init(priv, _CK_LSE, pdata->osci[OSC_LSI].freq);
	clk_stm32_oscillator_init(priv, _CK_CSI, pdata->osci[OSC_CSI].freq);
	clk_stm32_oscillator_init(priv, _I2SCKIN, pdata->osci[OSC_I2SCKIN].freq);
	clk_stm32_oscillator_init(priv, _SPDIFSYMB, pdata->osci[OSC_SPDIFSYMB].freq);
}

int stm32mp2_clk_pll_configure(struct stm32_clk_priv *priv)
{
	struct stm32mp2_clk_platdata *pdata = priv->pdata;
	uint32_t *clksrc = pdata->clksrc;
	struct stm32_pll_dt_cfg *pll_conf;
	bool spread_spectrum = false;
	enum pll_id i;
	int ret;

	/* PLL1 is already stopped in a35_ss_on_hsi function.*/
	for (i = _PLL2; i < _PLL_NB; i++) {
		/* TODO: check if some PLLs should be preserved */
		ret = stm32mp2_pll_stop(i);
		if (ret != 0) {
			panic();
		}
	}

	/* Configure PLLs source */
	for (i = _PLL1; i < _PLL_NB; i++) {
		stm32mp2_pll_mux_cfg(clksrc[CLKSRC_PLL1 + i] & PLL_CKSRC_NB,
			    (clksrc[CLKSRC_PLL1 + i] & PLL_CKSRC_SRC) >>
			    PLL_CKSRC_SRC_OFFSET);
	}

	/* Configure and start PLLs */
	if (clksrc[CLKSRC_CA35SS] == CLK_CA35SS_PLL1) {
		unsigned long refclk;

		refclk = _clk_stm32_get_rate(priv, _CK_PLL1);

		/*
		* No need to check max clock, as oscillator reference clocks will
		* always be less than 1.2GHz
		*/
		if (refclk < PLL_REFCLK_MIN) {
			ERROR("%s: %d\n", __func__, __LINE__);
			panic();
		}

		/* if PLL1 not defined */
		if (_PLL1 >= pdata->npll) {
			ERROR("%s: %d\n", __func__, __LINE__);
			panic();
		}

		pll_conf = &pdata->pll[_PLL1];

		stm32mp2_a35_pll1_config(pll_conf->cfg[FBDIV], pll_conf->cfg[REFDIV],
					 pll_conf->cfg[POSTDIV1], pll_conf->cfg[POSTDIV2]);

		stm32mp2_a35_pll1_start();
	}

	for (i = _PLL2; i < _PLL_NB && i < pdata->npll; i++) {
		pll_conf = &pdata->pll[i];

		if (pll_conf->csg_enabled == false) {
			continue;
		}

		ret = stm32mp2_pll_config(i, pll_conf->cfg, pll_conf->frac);
		if (ret != 0) {
			panic();
		}

		if (pll_conf->csg_enabled == true) {
			stm32mp2_pll_csg(i, pll_conf->csg);
			spread_spectrum = true;
		}

		stm32mp2_pll_start(i);
	}

	/* Wait and start PLLs ouptut when ready */
	if (clksrc[CLKSRC_CA35SS] == CLK_CA35SS_PLL1) {
		stm32mp2_a35_pll1_output();
	}

	for (i = _PLL2; i < _PLL_NB && i < pdata->npll; i++) {
		pll_conf = &pdata->pll[i];

		if (pll_conf->enabled == false) {
			continue;
		}

		ret = stm32mp2_pll_output(i, spread_spectrum);
		if (ret != 0) {
			panic();
		}
	}

	// { volatile int gab_debug = 1; while (gab_debug); }

	return 0;
}

int stm32mp2_clk_dividers_configure(struct stm32_clk_priv *priv)
{
	struct stm32mp2_clk_platdata *pdata = priv->pdata;

	enum enum_div_cfg div;

	for (div = 0; div < pdata->nclkdiv; div++) {
		clk_stm32_set_div(priv, div, pdata->clkdiv[div]);
	}

	return 0;
}

int stm32mp2_clk_switch_to_hsi(struct stm32_clk_priv *priv)
{
	stm32mp2_a35_ss_on_hsi();
	stm32mp2_clk_muxsel_on_hsi(priv);
	stm32mp2_clk_xbar_on_hsi(priv);

	return 0;
}

int __unused stm32mp2_clk_stgen_configure(struct stm32_clk_priv *priv, int id)
{
	unsigned long stgen_freq;

	stgen_freq = _clk_stm32_get_rate(priv, id);

	stm32mp_stgen_config(stgen_freq);

	return 0;
}

int __unused stm32mp2_clk_systick_configure(struct stm32_clk_priv *priv, int id)
{
	unsigned long systick_freq;

	systick_freq = _clk_stm32_get_rate(priv, id);
	stm32_systick_config(systick_freq);

	return 0;
}

int stm32mp2_clk_oscillators_wait_lse_ready(struct stm32_clk_priv *priv)
{
	int ret = 0;

	if (_clk_stm32_get_rate(priv, _CK_LSE) != 0U) {
		ret = clk_oscillator_wait_ready_on(priv, _CK_LSE);
	}

	return ret;
}

int stm32mp2_clk_oscillators_enable(struct stm32_clk_priv *priv)
{
	stm32_enable_oscillator_hse(priv);
	stm32_enable_oscillator_lse(priv);
	_clk_stm32_enable(priv, _CK_LSI);
	_clk_stm32_enable(priv, _CK_CSI);

	return 0;
}

int stm32mp2_clk_mux_configure(struct stm32_clk_priv *priv)
{
	return 0;
}

int stm32mp2_clk_probe(struct stm32_clk_priv *priv)
{
	int ret;

#ifndef CORE_CM33
	ret = stm32mp2_clk_stgen_configure(priv, _CK_HSI);
	if (ret != 0) {
		panic();
	}
#else
	ret = stm32mp2_clk_systick_configure(priv, CK_ICN_HS_MCU);
	if (ret != 0) {
		panic();
	}
#endif

#ifdef STM32_M33TDCID
	ret = stm32mp2_clk_oscillators_enable(priv);
	if (ret != 0) {
		panic();
	}

	/* Come back to HSI */
	ret = stm32mp2_clk_switch_to_hsi(priv);
	if (ret != 0) {
		panic();
	}

	ret = stm32mp2_clk_dividers_configure(priv);
	if (ret != 0) {
		panic();
	}

	ret = stm32mp2_clk_pll_configure(priv);
	if (ret != 0) {
		panic();
	}

	/* Wait LSE ready before to use it */
	stm32mp2_clk_oscillators_wait_lse_ready(priv);
	if (ret != 0) {
		panic();
	}

	ret = stm32mp2_clk_flexgen_configure(priv);
	if (ret != 0) {
		panic();
	}

#ifndef CORE_CM33
	ret = stm32mp2_clk_stgen_configure(priv, _CK_STGEN);
	if (ret != 0) {
		panic();
	}
#endif

	ret = stm32mp2_clk_rtc_configure_src(priv);
	if (ret != 0) {
		panic();
	}

	ret = stm32mp2_clk_mux_configure(priv);
	if (ret != 0) {
		panic();
	}
#endif	/* endif STM32_M33TDCID */

	return 0;
}

#ifdef LIBFDT
/*
 * Get the oscillator config from its name in device tree.
 * @param name: oscillator name
 * @param osci: reference on stm32 oscillator config
 * @return: 0 on success, and a negative FDT/ERRNO error code on failure.
 */
int fdt_osc_read(const char *name, struct stm32_osci_dt_cfg* osci)
{
	int node, subnode;
	void *fdt;

	if (fdt_get_address(&fdt) == 0) {
		return -ENOENT;
	}

	node = fdt_path_offset(fdt, "/clocks");
	if (node < 0) {
		return -FDT_ERR_NOTFOUND;
	}

	fdt_for_each_subnode(subnode, fdt, node) {
		const char *cchar;
		const fdt32_t *cuint;
		int ret;

		cchar = fdt_get_name(fdt, subnode, &ret);
		if (cchar == NULL)
			return ret;

		if (strncmp(cchar, name, (size_t)ret) ||
				fdt_get_status(subnode) == DT_DISABLED)
			continue;

		cuint = fdt_getprop(fdt, subnode, "clock-frequency", &ret);
		if (cuint == NULL)
			return ret;

		osci->freq = fdt32_to_cpu(*cuint);

		if (fdt_getprop(fdt, subnode, "st,bypass", NULL) != NULL)
			osci->bypass = true;

		if (fdt_getprop(fdt, subnode, "st,digbypass", NULL) != NULL)
			osci->digbyp = true;

		if (fdt_getprop(fdt, subnode, "st,css", NULL) != NULL)
			osci->css = true;

		osci->drive = fdt_read_uint32_default(fdt, subnode, "st,drive",
				LSEDRV_MEDIUM_HIGH);

		return 0;
	}

	/* Oscillator not found, freq=0 */
	osci->freq = 0;
	return 0;
}

/*
 * Get the frequency of an oscillator from its name in device tree.
 * @param name: oscillator name
 * @param osci: reference on stm32 oscillator config
 * @return: 0 on success, and a negative FDT/ERRNO error code on failure.
 */
int fdt_pll_read(const char *name, struct stm32_pll_dt_cfg* pll)
{
	int err, subnode;
	void *fdt;

	if (fdt_get_address(&fdt) == 0) {
		return -ENOENT;
	}

	subnode = fdt_rcc_subnode_offset(name);

	if (!fdt_check_node(subnode)) {
		return 0;
		// return -FDT_ERR_NOTFOUND;
	}


	err = fdt_read_uint32_array(fdt, subnode, "cfg", (int)PLLCFG_NB, pll->cfg);
	if (err)
		return err;

	err = fdt_read_uint32_array(fdt, subnode, "csg", (uint32_t)PLLCSG_NB, pll->csg);
	pll->csg_enabled = (err == 0);

	if (err == -FDT_ERR_NOTFOUND) {
		err = 0;
	}

	if (err)
		return err;

	pll->enabled = true;

	pll->frac = fdt_read_uint32_default(fdt, subnode, "frac", 0);

	return 0;
}


static int stm32mp2_clk_parse_fdt(struct stm32mp2_clk_platdata *pdata)
{
	int i, err;

	for (i = 0; i < NB_OSCILLATOR && i < pdata->nosci; i++) {
		struct stm32_osci_dt_cfg *osci = &pdata->osci[i];

		fdt_osc_read(stm32mp_osc_node_label[i], osci);
	}

	err = fdt_rcc_read_uint32_array("st,clksrc", (uint32_t)pdata->nclksrc, pdata->clksrc);
	if (err)
		panic();

	err = fdt_rcc_read_uint32_array("st,clkdiv", (uint32_t)pdata->nclkdiv, pdata->clkdiv);
	if (err)
		panic();

	for (i = 0; i < _PLL_NB && i < pdata->npll; i++) {
		struct stm32_pll_dt_cfg* pll = pdata->pll + i;
		char name[12];

		snprintf(name, sizeof(name), "st,pll@%d", i);
		err = fdt_pll_read(name, pll);

		if (err)
			panic();
	}

	return 0;
}

struct stm32_osci_dt_cfg mp2_dt_osci[NB_OSCILLATOR];
struct stm32_pll_dt_cfg mp2_dt_pll[_PLL_NB];
uint32_t mp2_dt_clksrc[CLKSRC_NB];
uint32_t mp2_dt_clkdiv[CLKDIV_NB];

__attribute__((weak))
int stm32_clk_get_platdata(struct stm32mp2_clk_platdata *pdata)
{
	pdata->rcc_base = stm32mp_rcc_base();

	pdata->osci = mp2_dt_osci;
	pdata->nosci = ARRAY_SIZE(mp2_dt_osci);

	pdata->pll = mp2_dt_pll;
	pdata->npll = ARRAY_SIZE(mp2_dt_pll);

	pdata->clksrc = mp2_dt_clksrc;
	pdata->nclksrc = ARRAY_SIZE(mp2_dt_clksrc);

	pdata->clkdiv = mp2_dt_clkdiv;
	pdata->nclkdiv = ARRAY_SIZE(mp2_dt_clkdiv);

	return 0;
}
#else
static int stm32mp2_clk_parse_fdt(struct stm32mp2_clk_platdata *pdata)
{
	return -ENOTSUP;
}

__attribute__((weak))
int stm32_clk_get_platdata(struct stm32mp2_clk_platdata *pdata)
{
	return -ENODEV;
}
#endif

static struct stm32_clk_priv stm32mp25_clock_data = {
	.num		= ARRAY_SIZE(stm32mp25_clk),
	.clks		= stm32mp25_clk,
	.parents	= parent_mp25,
	.gates		= gates_mp25,
	.div		= dividers_mp25,
	.gate_refcounts	= refcounts_mp25,
};

struct stm32_clk_priv *stm32_clk_get_driverdata(struct stm32mp2_clk_platdata *pdata)
{
	struct stm32_clk_priv *priv;

	priv = &stm32mp25_clock_data;
	priv->pdata = pdata;
	priv->base = pdata->rcc_base;

	return priv;
}

int stm32mp2_clk_init(void)
{
	struct stm32_clk_priv *priv;
	int err;

	err = stm32_clk_get_platdata(&clk_pdata);
	if (err)
		return err;

	err = stm32mp2_clk_parse_fdt(&clk_pdata);
	if (err && err != -ENOTSUP) {
		return err;
	}

	priv = stm32_clk_get_driverdata(&clk_pdata);
	stm32_init_clocks(priv);

	stm32mp_clk_oscillators_init(priv);

	err = stm32mp2_clk_probe(priv);
	if (err != 0) {
		return err;
	}

	stm32_clk_register();

	return 0;
}

