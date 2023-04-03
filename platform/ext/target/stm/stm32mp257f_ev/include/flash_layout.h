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

#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__

#include "region_defs.h"
#include "device_cfg.h"

/* This header file is included from linker scatter file as well, where only a
 * limited C constructs are allowed. Therefore it is not possible to include
 * here the platform_base_address.h to access flash related defines. To resolve
 * this some of the values are redefined here with different names, these are
 * marked with comment.
 */
#define S_RETRAM_ALIAS_BASE		(0x0E080000)
#define NS_RETRAM_ALIAS_BASE		(0x0A080000)
#define RETRAM_SZ			(0x20000)		/* 128KB */

#define S_SRAM2_ALIAS_BASE		(0x0E060000)
#define NS_SRAM2_ALIAS_BASE		(0x0A060000)
#define SRAM2_SZ			(0x20000)		/* 128KB */

#define S_SRAM1_ALIAS_BASE		(0x0E040000)
#define NS_SRAM1_ALIAS_BASE		(0x0A040000)
#define SRAM1_SZ			(0x20000)		/* 128KB */

#define S_SYSRAM_ALIAS_BASE		(0x0E000000)
#define NS_SYSRAM_ALIAS_BASE		(0x0A000000)
#define SYSRAM_SZ			(0x40000)		/* 256KB */

#define S_BKPSRAM_ALIAS_BASE		(0x52000000)
#define NS_BKPSRAM_ALIAS_BASE		(0x42000000)
#define BKPSRAM_SZ			(0x2000)		/* 8KB */

#define S_BKPREG_ALIAS_BASE		(0x56010000 + 0x100)	/* tamp base + bkpreg offset */
#define NS_BKPREG_ALIAS_BASE		(0x46010000 + 0x100)
#define BKPREG_SZ			(0x80)			/* 128B */

#define OSPI_ALIAS_BASE			(0x50430000)
#define OSPI_MEM_BASE			(0x60000000)

#define NS_DDR_ALIAS_BASE		(0x80000000)

/* 3 areas are available to define all regions of cache */
#define NS_REMAP3_ALIAS_BASE		(0x18000000)
#define NS_REMAP2_ALIAS_BASE		(0x10000000)
#define NS_REMAP1_ALIAS_BASE		(0x00000000)

/* Offset and size definition in flash area used by assemble.py */
#define SECURE_IMAGE_OFFSET		0x0
#define SECURE_IMAGE_MAX_SIZE		IMAGE_S_CODE_SIZE

#define NON_SECURE_IMAGE_OFFSET		IMAGE_S_CODE_SIZE
#define NON_SECURE_IMAGE_MAX_SIZE	IMAGE_NS_CODE_SIZE

#define DDR_RAM_OFFSET			0x0

/*
 * cortex m33 interface:
 * C-AHB for Instruction
 * S-AHB for Data
 *
 * For ddr The C-AHB bus is remap and not S-AHB
 */
#if defined(STM32_DDR_CACHED)
/* icache: map cache area on ddr ram offset */
#define DDR_CAHB_OFFSET			0x0
#define DDR_CAHB_ALIAS(x)		NS_REMAP_ALIAS(2, x)
#define DDR_CAHB2PHY_ALIAS(x)		NS_DDR_ALIAS(DDR_RAM_OFFSET + x)
#else
/* map = phy */
#define DDR_CAHB_OFFSET			DDR_RAM_OFFSET
#define DDR_CAHB_ALIAS(x)		NS_DDR_ALIAS(x)
#define DDR_CAHB2PHY_ALIAS(x)		NS_DDR_ALIAS(x)
#endif

#define DDR_SAHB_OFFSET			DDR_RAM_OFFSET
#define DDR_SAHB_ALIAS(x)		NS_DDR_ALIAS(x)
#define DDR_SAHB2PHY_ALIAS(x)		NS_DDR_ALIAS(x)

/* Internal Trusted Storage (ITS) Service definitions
 * Note: Further documentation of these definitions can be found in the
 * TF-M ITS Integration Guide. The ITS should be in the internal flash, but is
 * allocated in the external flash just for development platforms that don't
 * have internal flash available.
 *
 * constraints:
 * - nb blocks (minimal): 2
 * - block size >= (file size + metadata) aligned on power of 2
 * - file size = config ITS_MAX_ASSET_SIZE
 */
#if ITS_RAM_FS
/* Internal Trusted Storage emulated on RAM FS (ITS_RAM_FS)
 * which use an internal variable (its_block_data in TFM_DATA),
 * Driver_Flash_DDR is not used.
 */
#define TFM_HAL_ITS_FLASH_DRIVER	Driver_FLASH_DDR
#define TFM_HAL_ITS_FLASH_AREA_ADDR	0x0
#define TFM_HAL_ITS_FLASH_AREA_SIZE	4 * FLASH_DDR_SECTOR_SIZE
#define TFM_HAL_ITS_SECTORS_PER_BLOCK	(0x1)
#define TFM_HAL_ITS_PROGRAM_UNIT	FLASH_DDR_PROGRAM_UNIT
#define ITS_RAM_FS_SIZE			TFM_HAL_ITS_FLASH_AREA_SIZE
#else
#define TFM_HAL_ITS_FLASH_DRIVER	Driver_FLASH_BKPSRAM
#define TFM_HAL_ITS_FLASH_AREA_ADDR	0x1000
#define TFM_HAL_ITS_FLASH_AREA_SIZE	0x1000
#define TFM_HAL_ITS_SECTORS_PER_BLOCK	(0x2)
#define TFM_HAL_ITS_PROGRAM_UNIT	FLASH_BKPSRAM_PROGRAM_UNIT
#define ITS_RAM_FS_SIZE			TFM_HAL_ITS_FLASH_AREA_SIZE
#endif

#ifndef OTP_NV_COUNTERS_RAM_EMULATION
/* NV Counters: valide & backup (Backup register) */
#define NV_COUNTERS_FLASH_DRIVER	Driver_FLASH_BKPREG
#define NV_COUNTERS_AREA_SIZE		(0x1C) /* 28 Bytes */
#define NV_COUNTERS_AREA_ADDR		0x60
#endif

/* Protected Storage (PS) Service definitions
 * Note: Further documentation of these definitions can be found in the
 * TF-M PS Integration Guide.
 *
 * constraints:
 *  - nb blocks (minimal): 2
 *  - block size >= (file size + metadata) aligned on power of 2
 *  - file size = config PS_MAX_ASSET_SIZE
 */
#if PS_RAM_FS
/* Protected Storage emulated on RAM FS (PS_RAM_FS)
 * which use an internal variable (ps_block_data in TFM_DATA),
 * Driver_Flash_DDR is not used.
 */
#define TFM_HAL_PS_FLASH_DRIVER		Driver_FLASH_DDR
#define TFM_HAL_PS_FLASH_AREA_ADDR	0x0
#define TFM_HAL_PS_FLASH_AREA_SIZE	4 * FLASH_DDR_SECTOR_SIZE
#define TFM_HAL_PS_SECTORS_PER_BLOCK	(0x1)
#define TFM_HAL_PS_PROGRAM_UNIT		FLASH_DDR_PROGRAM_UNIT
#define PS_RAM_FS_SIZE			TFM_HAL_PS_FLASH_AREA_SIZE
#else
#define TFM_HAL_PS_FLASH_DRIVER		Driver_OSPI_SPI_NOR_FLASH
#define TFM_HAL_PS_FLASH_AREA_ADDR	0x0
#define TFM_HAL_PS_FLASH_AREA_SIZE	(0x100000)			/* 1 MB */
#define TFM_HAL_PS_SECTORS_PER_BLOCK	(0x1)
#define TFM_HAL_PS_PROGRAM_UNIT		(0x1)
#define PS_SECTOR_SIZE			FLASH_AREA_IMAGE_SECTOR_SIZE
#endif

#define TOTAL_RETRAM_SZ			(RETRAM_SZ)			/* 128 KB */
#define TOTAL_SRAM_SZ			(SRAM1_SZ + SRAM2_SZ)		/* 256 KB */
#define TOTAL_SYSRAM_SZ			(SYSRAM_SZ)			/* 256 KB */

#endif /* __FLASH_LAYOUT_H__ */
