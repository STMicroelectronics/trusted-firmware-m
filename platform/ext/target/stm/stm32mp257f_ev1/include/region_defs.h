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

#define IMAGE_S_CODE_SIZE		0x100000 /* S partition:  1MB */
#define IMAGE_NS_CODE_SIZE		0x800000 /* NS partition: 8MB */

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
#define IMAGE_LOAD_ADDRESS		DDR_CAHB_ALIAS(DDR_CAHB_OFFSET)

/* Define where executable memory for the images starts and ends */
#define IMAGE_EXECUTABLE_RAM_START      IMAGE_LOAD_ADDRESS
#define IMAGE_EXECUTABLE_RAM_SIZE       (IMAGE_S_CODE_SIZE + IMAGE_NS_CODE_SIZE)

#define S_IMAGE_RAM_OFFSET		DDR_CAHB_OFFSET
#define S_IMAGE_RAM_LIMIT		S_IMAGE_RAM_OFFSET + IMAGE_S_CODE_SIZE - 1

#define NS_IMAGE_RAM_OFFSET		DDR_CAHB_OFFSET + IMAGE_S_CODE_SIZE
#define NS_IMAGE_RAM_LIMIT		NS_IMAGE_RAM_OFFSET + IMAGE_NS_CODE_SIZE -1

#define S_DATA_RAM_OFFSET		DDR_SAHB_OFFSET + IMAGE_EXECUTABLE_RAM_SIZE
#define S_DATA_SIZE			0x100000
#define S_DATA_RAM_LIMIT		S_DATA_RAM_OFFSET + S_DATA_SIZE - 1

#define NS_DATA_RAM_OFFSET		S_DATA_RAM_OFFSET + S_DATA_SIZE
#define NS_DATA_SIZE			0x800000
#define NS_DATA_RAM_LIMIT		NS_DATA_RAM_OFFSET + NS_DATA_SIZE - 1

#define NS_IPC_SHMEM_OFFSET		NS_DATA_RAM_OFFSET + NS_DATA_SIZE
#define NS_IPC_SHMEM_SIZE		0x100000

#define S_CODE_START			DDR_CAHB_ALIAS(DDR_CAHB_OFFSET + BL2_HEADER_SIZE)
#define S_CODE_SIZE			(IMAGE_S_CODE_SIZE)
#define S_CODE_LIMIT			(S_CODE_START + S_CODE_SIZE - 1)

/* Non-secure regions */
#define NS_CODE_START			(DDR_CAHB_ALIAS(NS_IMAGE_RAM_OFFSET))
#define NS_CODE_SIZE			(IMAGE_NS_CODE_SIZE)
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

/* Shared data area between bootloader and runtime firmware.
 * M33 copro: not used
 */
#define BOOT_TFM_SHARED_DATA_BASE	(NS_DATA_START + NS_DATA_SIZE)
#define BOOT_TFM_SHARED_DATA_SIZE	(0x0)
#define BOOT_TFM_SHARED_DATA_LIMIT	(BOOT_TFM_SHARED_DATA_BASE)

/* OTP shadow regionBOOT_TFM_SHARED_DATA_SIZE
 * In Copro Mode, the TDCID loader copies bsec otp on shadow memory */
#define OTP_SHADOW_START		S_SRAM1_ALIAS(0x0)
#define OTP_SHADOW_SIZE			(0x1000)

#endif /* __REGION_DEFS_H__ */
