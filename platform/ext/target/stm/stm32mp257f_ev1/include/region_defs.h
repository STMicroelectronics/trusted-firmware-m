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

#ifndef __REGION_DEFS_H__
#define __REGION_DEFS_H__

#include <flash_layout.h>

#define BL2_HEAP_SIZE			(0x0001000)
#define BL2_MSP_STACK_SIZE		(0x0001800)

#define S_HEAP_SIZE			(0x0001000)
#define S_MSP_STACK_SIZE_INIT		(0x0000400)
#define S_MSP_STACK_SIZE		(0x0000800)
#define S_PSP_STACK_SIZE		(0x0000800)

#define NS_HEAP_SIZE			(0x0001000)
#define NS_STACK_SIZE			(0x0001000)

/* This size of buffer is big enough to store an attestation
 * token produced by initial attestation service
 */
#define PSA_INITIAL_ATTEST_TOKEN_MAX_SIZE   (0x250)


/*
 * defined by reserved memory of device tree
 * */
#define IMAGE_S_CODE_SIZE		DT_REG_SIZE(DT_NODELABEL(tfm_code))
#define IMAGE_NS_CODE_SIZE		DT_REG_SIZE(DT_NODELABEL(cm33_cube_fw))
#define IMAGE_S_DATA_SIZE		DT_REG_SIZE(DT_NODELABEL(tfm_data))
#define IMAGE_NS_DATA_SIZE		DT_REG_SIZE(DT_NODELABEL(cm33_cube_data))

#if STM32_M33TDCID
#define BL2_DATA_START			DT_REG_ADDR(DT_NODELABEL(cm33_sram1))
#define BL2_DATA_SIZE			(DT_REG_SIZE(DT_NODELABEL(cm33_sram1)) - \
					 BL2_MSP_STACK_SIZE)
#define BL2_DATA_LIMIT			(BL2_DATA_START + BL2_DATA_SIZE - 1)
#define BL2_MSP_STACK_START		(BL2_DATA_START + BL2_DATA_SIZE)
#endif

/*
 * Size of vector table:
 *  - core interrupts including MPS initial value: 16
 *  - vendor interrupts: MAX_IRQ_n: 320
 */
#define S_CODE_VECTOR_TABLE_SIZE ((320 + 16) * 4)

/* Alias definitions for secure and non-secure areas*/
#define S_RETRAM_ALIAS(x)		S_RETRAM_ALIAS_BASE + (x)
#define NS_RETRAM_ALIAS(x)		NS_RETRAM_ALIAS_BASE + (x)

#define S_SRAM1_ALIAS(x)		S_SRAM1_ALIAS_BASE + (x)
#define NS_SRAM1_ALIAS(x)		NS_SRAM1_ALIAS_BASE + (x)

#define S_SRAM2_ALIAS(x)		S_SRAM2_ALIAS_BASE + (x)
#define NS_SRAM2_ALIAS(x)		NS_SRAM2_ALIAS_BASE + (x)

#define S_SYSRAM_ALIAS(x)		S_SYSRAM_ALIAS_BASE + (x)
#define NS_SYSRAM_ALIAS(x)		NS_SYSRAM_ALIAS_BASE + (x)

#define S_BKPSRAM_ALIAS(x)		S_BKPSRAM_ALIAS_BASE + (x)
#define NS_BKPSRAM_ALIAS(x)		NS_BKPSRAM_ALIAS_BASE + (x)

/* Non-Secure not aliased, managed by SAU */
#define NS_DDR_ALIAS(x)			NS_DDR_ALIAS_BASE + (x)
#define NS_OSPI_MEM_ALIAS(x)		OSPI_MEM_BASE + (x)
#define NS_REMAP_ALIAS(area, x)		(NS_REMAP##area##_ALIAS_BASE + (x))

/* Image load address used by imgtool.py */
#define S_IMAGE_LOAD_ADDRESS		DDR_CAHB_ALIAS(DDR_CAHB_OFFSET)

/* Define where executable memory for the images starts and ends */
#define IMAGE_EXECUTABLE_RAM_START      S_IMAGE_LOAD_ADDRESS
#define IMAGE_EXECUTABLE_RAM_SIZE       (IMAGE_S_CODE_SIZE + IMAGE_NS_CODE_SIZE)

#define S_IMAGE_RAM_OFFSET		DDR_CAHB_OFFSET
#define S_IMAGE_RAM_LIMIT		S_IMAGE_RAM_OFFSET + IMAGE_S_CODE_SIZE - 1

#define NS_IMAGE_RAM_OFFSET		DDR_CAHB_OFFSET + IMAGE_S_CODE_SIZE
#define NS_IMAGE_RAM_LIMIT		NS_IMAGE_RAM_OFFSET + IMAGE_NS_CODE_SIZE -1

#define S_DATA_RAM_OFFSET		DDR_SAHB_OFFSET + IMAGE_EXECUTABLE_RAM_SIZE
#define S_DATA_SIZE			IMAGE_S_DATA_SIZE
#define S_DATA_RAM_LIMIT		S_DATA_RAM_OFFSET + S_DATA_SIZE - 1

#define NS_DATA_RAM_OFFSET		S_DATA_RAM_OFFSET + S_DATA_SIZE
#define NS_DATA_SIZE			IMAGE_NS_DATA_SIZE
#define NS_DATA_RAM_LIMIT		NS_DATA_RAM_OFFSET + NS_DATA_SIZE - 1

#define NS_IPC_SHMEM_OFFSET		NS_DATA_RAM_OFFSET + NS_DATA_SIZE
#define NS_IPC_SHMEM_SIZE		0x100000

#define S_CODE_START			DDR_CAHB_ALIAS(S_IMAGE_RAM_OFFSET + BL2_HEADER_SIZE)
#define S_CODE_SIZE			(IMAGE_S_CODE_SIZE - BL2_HEADER_SIZE)
#define S_CODE_LIMIT			(S_CODE_START + S_CODE_SIZE - 1)

/* Non-secure regions */
#define NS_CODE_START			(DDR_CAHB_ALIAS(NS_IMAGE_RAM_OFFSET))
#define NS_CODE_SIZE			(IMAGE_NS_CODE_SIZE - BL2_TRAILER_SIZE)
#define NS_CODE_LIMIT			(NS_CODE_START + NS_CODE_SIZE - 1)

#define S_DATA_START			(DDR_SAHB_ALIAS(S_DATA_RAM_OFFSET))
#define S_DATA_LIMIT			(S_DATA_START + S_DATA_SIZE - 1)

#define NS_DATA_START			(DDR_SAHB_ALIAS(NS_DATA_RAM_OFFSET))
#define NS_DATA_LIMIT			(NS_DATA_START + NS_DATA_SIZE - 1)

#define NS_IPC_SHMEM_START		(DDR_SAHB_ALIAS(NS_IPC_SHMEM_OFFSET))
#define NS_IPC_SHMEM_LIMIT		(NS_IPC_SHMEM_START + NS_IPC_SHMEM_SIZE - 1)

/* NS partition information is used for MPC and SAU configuration */
#define NS_PARTITION_START		(NS_CODE_START)
#define NS_PARTITION_SIZE		(NS_CODE_SIZE)

/* Bootloader regions */
/* On stm32mp2 the bl2 is loaded in retram by bootrom */
#define BL2_CODE_OFFSET			0x0
#define BL2_CODE_START			(S_RETRAM_ALIAS(BL2_CODE_OFFSET))
#define BL2_CODE_SIZE			FLASH_AREA_BL2_SIZE - BL2_OTP_AREA_SIZE
#define BL2_CODE_LIMIT			(BL2_CODE_START + BL2_CODE_SIZE - 1)
#define BL2_OTP_AREA_OFFSET		BL2_CODE_OFFSET + BL2_CODE_SIZE
#define BL2_OTP_AREA_BASE		(S_RETRAM_ALIAS(BL2_OTP_AREA_OFFSET))
#define BL2_OTP_AREA_SIZE		(0x0000)

#define STM32MP_DDR_FW_BASE		(S_SRAM2_ALIAS(0x0))
#define STM32MP_DDR_FW_DMEM_OFFSET	U(0x400)
#define STM32MP_DDR_FW_IMEM_OFFSET	U(0x800)
#define STM32MP_DDR_FW_MAX_SIZE		U(0x8800)

/* Shared data area between bootloader and runtime firmware. */
#if STM32_M33TDCID
#define BOOT_TFM_SHARED_DATA_BASE	DT_REG_ADDR(DT_NODELABEL(tfm_shared_data))
#define BOOT_TFM_SHARED_DATA_SIZE	DT_REG_SIZE(DT_NODELABEL(tfm_shared_data))
#define BOOT_TFM_SHARED_DATA_LIMIT	(BOOT_TFM_SHARED_DATA_BASE + BOOT_TFM_SHARED_DATA_SIZE - 1)
#else
/* copro: Not yet used */
#define BOOT_TFM_SHARED_DATA_BASE	(NS_DATA_START + NS_DATA_SIZE)
#define BOOT_TFM_SHARED_DATA_SIZE	(0x0)
#define BOOT_TFM_SHARED_DATA_LIMIT	(BOOT_TFM_SHARED_DATA_BASE)
#endif

#endif /* __REGION_DEFS_H__ */
