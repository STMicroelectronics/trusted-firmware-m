/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cmsis.h>
#include <region.h>
#include <lib/timeout.h>
#include <clk.h>

#include <sau_armv8m_drv.h>

#include <stm32mp2xx.h>
#include <stm32mp2_clk.h>
#include <stm32_syscfg.h>
#include <stm32_gpio.h>
#include <stm32_uart.h>
#include <stm32_ospi.h>
#include <stm32_systick.h>
#include <stm32_icache.h>
#include <stm32_dcache.h>
#include <stm32_reset.h>
#include <stm32_ddr.h>
#include <stm32_bsec3.h>
#include <spi_mem.h>
#include <spi_nor.h>

#include <region_defs.h>

#include <plat_device.h>
#include <dt-bindings/clock/stm32mp2-clks.h>
#include <dt-bindings/clock/stm32mp2-clksrc.h>
#include <dt-bindings/pinctrl/stm32-pinfunc.h>


#if (STM32_NSEC == 1)
#define BASE_DOMAIN(_name) _name##_NS
#else
#define BASE_DOMAIN(_name) _name##_S
#endif

/* sau device */
#ifdef STM32_SEC
/* The section names come from the scatter file */
REGION_DECLARE(Load$$LR$$, LR_NS_PARTITION, $$Base);
REGION_DECLARE(Image$$, ER_VENEER, $$Base);
REGION_DECLARE(Image$$, VENEER_ALIGN, $$Limit);

#define NS_SECURE_BASE  (uint32_t)&REGION_NAME(Load$$LR$$, LR_NS_PARTITION, $$Base)
#define NS_SECURE_LIMIT NS_SECURE_BASE + NS_PARTITION_SIZE -1
#define VENEER_BASE	(uint32_t)&REGION_NAME(Image$$, ER_VENEER, $$Base)
#define VENEER_LIMIT	(uint32_t)&REGION_NAME(Image$$, VENEER_ALIGN, $$Limit)

const struct sau_region_cfg_t sau_region[] = {
	// non secure code
	SAU_REGION_CFG(NS_SECURE_BASE, NS_SECURE_LIMIT, SAU_EN),
	// non secure data
	SAU_REGION_CFG(NS_DATA_START, NS_DATA_LIMIT, SAU_EN),
	// veneer region to be non-secure callable
	SAU_REGION_CFG(VENEER_BASE, VENEER_LIMIT, SAU_EN | SAU_NSC),
	//non secure peripheral
	SAU_REGION_CFG(PERIPH_BASE_NS, PERIPH_BASE_S - 1, SAU_EN),
#ifdef STM32_IPC
	//ipc share memory
	SAU_REGION_CFG(NS_IPC_SHMEM_START, NS_IPC_SHMEM_LIMIT, SAU_EN),
#endif
};

int sau_get_platdata(struct sau_platdata *pdata)
{
	pdata->base = SAU_BASE;
	pdata->sau_region = sau_region;
	pdata->nr_region = ARRAY_SIZE(sau_region);

	return 0;
}
#endif /* STM32_SEC */

/* clk device */
struct stm32_osci_dt_cfg stm32_osci[] = {
	STM32_OSCI_CFG(OSC_LSI, 32000, false, false, false, 0),
	STM32_OSCI_CFG(OSC_LSE, 32768, false, false, false, 0),
	STM32_OSCI_CFG(OSC_HSI, 64000000, false, false, false, 0),
	STM32_OSCI_CFG(OSC_HSE, 40000000, false, false, false, 0),
	STM32_OSCI_CFG(OSC_CSI, 16000000, false, false, false, 0),
};

#define STM32_PLL1_CFG {30, 1 ,1 ,1}
#define STM32_PLL2_CFG {30, 1 ,1 ,2}
#define STM32_PLL3_CFG {20, 1 ,1 ,1}
#define STM32_PLL4_CFG {30, 1 ,1 ,1}
#define STM32_PLL5_CFG {133, 5 ,1 ,2}
#define STM32_PLL6_CFG {25, 1 ,1 ,2}
#define STM32_PLL7_CFG {167, 4, 1, 2}
#define STM32_PLL8_CFG {297, 5 ,1 ,4}

struct stm32_pll_dt_cfg stm32_pll[] = {
	STM32_PLL_CFG(_PLL1, STM32_PLL1_CFG, 0),
	STM32_PLL_CFG(_PLL2, STM32_PLL2_CFG, 0),
	STM32_PLL_CFG(_PLL3, STM32_PLL3_CFG, 0),
	STM32_PLL_CFG(_PLL4, STM32_PLL4_CFG, 0),
	STM32_PLL_CFG(_PLL5, STM32_PLL5_CFG, 0),
	STM32_PLL_CFG(_PLL6, STM32_PLL6_CFG, 0),
	STM32_PLL_CFG(_PLL7, STM32_PLL7_CFG, 0),
	STM32_PLL_CFG(_PLL8, STM32_PLL8_CFG, 0),
};

uint32_t stm32_clksrc [] = {
	CLK_CA35SS_PLL1,
	PLL_CFG(1, PLL_SRC_HSE),
	PLL_CFG(2, PLL_SRC_HSE),
	PLL_CFG(3, PLL_SRC_HSE),
	PLL_CFG(4, PLL_SRC_HSE),
	PLL_CFG(5, PLL_SRC_HSE),
	PLL_CFG(6, PLL_SRC_HSE),
	PLL_CFG(7, PLL_SRC_HSE),
	PLL_CFG(8, PLL_SRC_HSE),
	XBAR_CFG(0, XBAR_SRC_PLL4, 0, 2),
	XBAR_CFG(1, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(2, XBAR_SRC_PLL4, 0, 1),
	XBAR_CFG(3, XBAR_SRC_PLL4, 0, 2),
	XBAR_CFG(4, XBAR_SRC_PLL4, 0, 3),
	XBAR_CFG(5, XBAR_SRC_PLL4, 0, 2),
	XBAR_CFG(6, XBAR_SRC_PLL4, 0, 1),
	XBAR_CFG(7, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(8, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(9, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(10, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(11, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(12, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(13, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(14, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(15, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(16, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(17, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(18, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(19, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(20, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(21, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(22, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(23, XBAR_SRC_PLL7, 0, 16),
	XBAR_CFG(24, XBAR_SRC_PLL7, 0, 16),
	XBAR_CFG(25, XBAR_SRC_PLL7, 0, 16),
	XBAR_CFG(26, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(27, XBAR_SRC_PLL8, 0, 3),
	XBAR_CFG(28, XBAR_SRC_PLL8, 0, 21),
	XBAR_CFG(29, XBAR_SRC_PLL5, 0, 1),
	XBAR_CFG(30, XBAR_SRC_HSE, 0, 1),
	XBAR_CFG(31, XBAR_SRC_PLL5, 0, 19),
	XBAR_CFG(32, XBAR_SRC_PLL5, 0, 19),
	XBAR_CFG(33, XBAR_SRC_PLL4, 0, 23),
	XBAR_CFG(34, XBAR_SRC_PLL4, 0, 59),
	XBAR_CFG(35, XBAR_SRC_HSI, 0, 3),
	XBAR_CFG(36, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(37, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(38, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(39, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(40, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(41, XBAR_SRC_PLL4, 0, 11),
	XBAR_CFG(42, XBAR_SRC_PLL7, 0, 6),
	XBAR_CFG(43, XBAR_SRC_PLL4, 0, 23),
	XBAR_CFG(44, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(45, XBAR_SRC_PLL4, 0, 2),
	XBAR_CFG(46, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(47, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(48, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(49, XBAR_SRC_PLL5, 0, 3),
	XBAR_CFG(50, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(51, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(52, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(53, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(54, XBAR_SRC_PLL6, 0, 3),
	XBAR_CFG(55, XBAR_SRC_PLL6, 0, 3),
	XBAR_CFG(56, XBAR_SRC_PLL4, 0, 5),
	XBAR_CFG(57, XBAR_SRC_HSE, 0, 1),
	XBAR_CFG(58, XBAR_SRC_HSE, 0, 1),
	XBAR_CFG(59, XBAR_SRC_PLL4, 0, 1),
	XBAR_CFG(60, XBAR_SRC_PLL6, 0, 3),
	XBAR_CFG(61, XBAR_SRC_PLL4, 0, 7),
	XBAR_CFG(62, XBAR_SRC_PLL4, 0, 7),
	XBAR_CFG(63, XBAR_SRC_PLL4, 0, 2),
};

uint32_t stm32_clkdiv [] = {
	1, /*APB1*/
	1, /*APB2*/
	1, /*APB3*/
	1, /*APB4*/
	1, /*APBDBG*/
	23, /*RTC*/
};

int stm32_clk_get_platdata(struct stm32mp2_clk_platdata *pdata)
{
	pdata->rcc_base = BASE_DOMAIN(RCC_BASE);

	pdata->osci = stm32_osci;
	pdata->nosci = ARRAY_SIZE(stm32_osci);

	pdata->pll = stm32_pll;
	pdata->npll = ARRAY_SIZE(stm32_pll);

	pdata->clksrc = stm32_clksrc;
	pdata->nclksrc = ARRAY_SIZE(stm32_clksrc);

	pdata->clkdiv = stm32_clkdiv;
	pdata->nclkdiv = ARRAY_SIZE(stm32_clkdiv);

	return 0;
}

/* rcc reset */
int stm32_reset_get_platdata(struct stm32_reset_platdata *pdata)
{
	pdata->base = BASE_DOMAIN(RCC_BASE);
	return 0;
}

/* systick */
int stm32_systick_get_platdata(struct stm32_systick_platdata *pdata)
{
	pdata->systick = BASE_DOMAIN(systick);
	pdata->clk_id = CK_ICN_HS_MCU;
	pdata->tick_hz = USEC_PER_SEC;
	return 0;
}

/* gpio device */
const struct bank_cfg stm32_banks[] = {
	STM32_BANK_CFG('B', BASE_DOMAIN(GPIOB_BASE), CK_BUS_GPIOB),
	STM32_BANK_CFG('D', BASE_DOMAIN(GPIOD_BASE), CK_BUS_GPIOD),
	STM32_BANK_CFG('F', BASE_DOMAIN(GPIOF_BASE), CK_BUS_GPIOF),
	STM32_BANK_CFG('G', BASE_DOMAIN(GPIOG_BASE), CK_BUS_GPIOG),
};

int stm32_gpio_get_platdata(struct stm32_gpio_platdata *pdata)
{
	pdata->banks = stm32_banks;
	pdata->nbanks = ARRAY_SIZE(stm32_banks);

	return 0;
}

/* uart 5 device */
uint32_t uart5_pins1_pinmux[] = {STM32_PINMUX('G', 9, AF5)};  /* UART5_TX */
uint32_t uart5_pins2_pinmux[] = {STM32_PINMUX('G', 10, AF5)};  /* UART5_RX */

struct pin_cfg uart5_pins_a [] = {
	STM32_PINCFG(uart5_pins1_pinmux, 0, GPIO_NO_PULL, GPIO_PUSH_PULL),
	STM32_PINCFG(uart5_pins2_pinmux, 0, GPIO_NO_PULL, GPIO_OPEN_DRAIN),
};

PINCTRL_CFG_DEFINE(uart5_pctrl, uart5_pins_a)

int stm32_uart_get_platdata(struct stm32_uart_platdata *pdata)
{
	pdata->base = BASE_DOMAIN(UART5_BASE);
	pdata->clk_id = CK_KER_UART5;
	pdata->pinctrl = &uart5_pctrl;

	return 0;
}

/* ospi */
uint32_t ospi_clk_pinmux[] = {
	STM32_PINMUX('D', 0, AF10), /* OSPI_CLK */
};

uint32_t ospi_data_pinmux[] = {
	STM32_PINMUX('D', 4, AF10), /* OSPI_IO0 */
	STM32_PINMUX('D', 5, AF10), /* OSPI_IO1 */
	STM32_PINMUX('D', 6, AF10), /* OSPI_IO2 */
	STM32_PINMUX('D', 7, AF10), /* OSPI_IO3 */
};

uint32_t ospi_cs0_pinmux[] = {
	STM32_PINMUX('D', 3, AF10), /* OSPI_NCS1 */
};

struct pin_cfg ospi_pinctrl_0 [] = {
	STM32_PINCFG(ospi_clk_pinmux, 3, GPIO_NO_PULL, GPIO_PUSH_PULL),
	STM32_PINCFG(ospi_data_pinmux, 1, GPIO_NO_PULL, GPIO_PUSH_PULL),
	STM32_PINCFG(ospi_cs0_pinmux, 1, GPIO_PULL_UP, GPIO_PUSH_PULL),
};

PINCTRL_CFG_DEFINE(ospi_pctrl, ospi_pinctrl_0)

struct spi_slave flash0 = {
	.cs = 0,
	.max_hz = 50000000,
	.mode = SPI_RX_QUAD | SPI_TX_QUAD,
};

int stm32_ospi_get_platdata(struct stm32_ospi_platdata *pdata)
{
	pdata->reg_base = BASE_DOMAIN(OCTOSPI2_BASE);
	pdata->mm_base = 0x60000000;
	pdata->mm_size = SPI_NOR_FLASH_SIZE;
	pdata->clock_id = CK_KER_OSPI2;
	pdata->reset_id[0] = STM32_RSTCTRL_REF(OSPI2_R);
	pdata->reset_id[1] = STM32_RSTCTRL_REF(OSPI2DLL_R);
	pdata->pinctrl = &ospi_pctrl;
	pdata->spi_slave = &flash0;

	return 0;
}

/* nor */
int spi_nor_get_config(struct nor_device *device)
{
	device->size = SPI_NOR_FLASH_SIZE;
	device->write_size = SPI_NOR_FLASH_PAGE_SIZE;
	device->erase_size = SPI_NOR_FLASH_SECTOR_SIZE;

	return 0;
}

int __maybe_unused stm32_icache_get_platdata(struct stm32_icache_platdata *pdata)
{
	pdata->base = BASE_DOMAIN(ICACHE_BASE);
	pdata->irq = ICACHE_IRQn;

	return 0;
}

int __maybe_unused stm32_dcache_get_platdata(struct stm32_dcache_platdata *pdata)
{
	pdata->base = BASE_DOMAIN(DCACHE_BASE);
	pdata->irq = DCACHE_IRQn;

	return 0;
}

int __maybe_unused stm32_otp_shadow_get_platdata(struct stm32_otp_shadow_platdata *pdata)
{
	pdata->base = OTP_SHADOW_START;
	pdata->size = OTP_SHADOW_SIZE;

	return 0;
}

/*
 * TODO: setup risab5 (retram), to use otp (via PLATFORM_DEFAULT_OTP)
 * stored in BL2_OTP_Const section.
 */
int __maybe_unused stm32_platform_s_init(void)
{
	int err;

	err = stm32_systick_init();
	if (err)
		return err;

	err = stm32_reset_init();
	if (err)
		return err;

	err = stm32mp2_clk_init();
	if (err)
		return err;

#if defined(STM32_DDR_CACHED)
	err = stm32_icache_init();
	if (err)
		return err;

#if defined(STM32_SEC)
	err = stm32_dcache_init();
	if (err)
		return err;
#endif
#endif

	err = stm32_gpio_init();
	if (err)
		return err;

	err = stm32_otp_shadow_init();
	if (err)
		return err;

	return 0;
}

int __maybe_unused stm32_platform_ns_init(void)
{
	int err;

	err = stm32_systick_init();
	if (err)
		return err;

	err = stm32mp2_clk_init();
	if (err)
		return err;

	err = stm32_gpio_init();
	if (err)
		return err;

	return 0;
}
