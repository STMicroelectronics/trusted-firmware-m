/*
 * Copyright (C) 2020 STMicroelectronics.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DEVICE_CFG_H__
#define __DEVICE_CFG_H__

#include <flash_layout.h>

#include <dt-bindings/clock/stm32mp25-clks.h>

/**
 * \file device_cfg.h
 * \brief Configuration file driver re-targeting
 *
 * \details This file can be used to add driver specific macro
 *          definitions to select which peripherals are available in the build.
 *
 * This is a default device configuration file with all peripherals enabled.
 */
/* uart */
#define STM_USART5			1
#define DEFAULT_UART_BAUDRATE		115200

/* storage */
#define STM32_FLASH_DDR
#define FLASH_DDR_BASE			NS_DDR_ALIAS_BASE
#define FLASH_DDR_CLK			CLK_UNDEF
#define FLASH_DDR_SIZE			(0x4000000)	/* limit to 64MB */
#define FLASH_DDR_SECTOR_SIZE		(0x00001000)	/* 4 kB */
#define FLASH_DDR_PROGRAM_UNIT		(0x1)		/* Minimum write size */

#undef STM32_FLASH_RETRAM
#define FLASH_RETRAM_BASE		S_RETRAM_ALIAS_BASE
#define FLASH_RETRAM_CLK		CK_BUS_RETRAM
#define FLASH_RETRAM_SIZE		RETRAM_SZ	/* 128 kB */
#define FLASH_RETRAM_SECTOR_SIZE	(0x0000100)	/* 256 B */
#define FLASH_RETRAM_PROGRAM_UNIT	(0x1)		/* Minimum write size */

#define STM32_FLASH_BKPSRAM
#define FLASH_BKPSRAM_BASE		S_BKPSRAM_ALIAS_BASE
#define FLASH_BKPSRAM_CLK		CK_BUS_BKPSRAM
#define FLASH_BKPSRAM_SIZE		BKPSRAM_SZ	/* 8 kB */
#define FLASH_BKPSRAM_SECTOR_SIZE	(0x0000100)	/* 256 B */
#define FLASH_BKPSRAM_PROGRAM_UNIT	(0x1)		/* Minimum write size */

#define STM32_FLASH_BKPREG
#define FLASH_BKPREG_BASE		S_BKPREG_ALIAS_BASE
#define FLASH_BKPREG_CLK		CK_BUS_RTC
#define FLASH_BKPREG_SIZE		BKPREG_SZ	/* 128 B */
#define FLASH_BKPREG_SECTOR_SIZE	(0x4)		/* 32-bit backup registers */
#define FLASH_BKPREG_PROGRAM_UNIT	(0x4)		/* Minimum write size */

#define SPI_NOR_FLASH_SIZE		(0x4000000)	/* 64 MB */
#define SPI_NOR_FLASH_SECTOR_SIZE	(0x1000)	/* 4KB */
#define SPI_NOR_FLASH_PAGE_SIZE		(256)		/* 256B */

#endif  /* __DEVICE_CFG_H__ */
