/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cmsis.h>
#include <region.h>
#include <lib/utils_def.h>

#include <sau_armv8m_drv.h>

#include <stm32_ospi.h>
#include <spi_mem.h>
#include <spi_nor.h>
#include <stm32_icache.h>
#include <stm32_dcache.h>

#include <region_defs.h>

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

/* ospi */
struct spi_slave flash0 = {
	.cs = 0,
	.max_hz = 50000000,
	.mode = SPI_RX_QUAD | SPI_TX_QUAD,
};

int stm32_ospi_get_platdata(struct stm32_ospi_platdata *pdata)
{
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

/*
 * TODO: setup risab5 (retram), to use otp (via PLATFORM_DEFAULT_OTP)
 * stored in BL2_OTP_Const section.
 */
int __maybe_unused stm32_platform_s_init(void)
{
	int __maybe_unused err;

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
	return 0;
}

int __maybe_unused stm32_platform_ns_init(void)
{
	return 0;
}
