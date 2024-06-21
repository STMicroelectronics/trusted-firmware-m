/*
 * Copyright (C) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
/*
 * Allow to overwrite functions defined in platform/ext/common/boot_hal.c
 * for specific needs of stm32mp2
 */
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <cmsis.h>
#include <region.h>
#include <region_defs.h>
#include <Driver_Flash.h>
#include <bootutil/bootutil_log.h>
#include <init.h>
#include <debug.h>
#include <partition.h>
#include "flash_map/flash_map.h"

#include <stm32_icache.h>
#include <stm32_bsec3.h>

extern ARM_DRIVER_FLASH FLASH_DEV_FW_DDR_NAME;

REGION_DECLARE(Image$$, ER_DATA, $$Base)[];
REGION_DECLARE(Image$$, ARM_LIB_HEAP, $$ZI$$Limit)[];

__attribute__((naked)) void boot_clear_bl2_ram_area(void)
{
    __ASM volatile(
        "mov     r0, #0                              \n"
        "subs    %1, %1, %0                          \n"
        "Loop:                                       \n"
        "subs    %1, #4                              \n"
        "itt     ge                                  \n"
        "strge   r0, [%0, %1]                        \n"
        "bge     Loop                                \n"
        "bx      lr                                  \n"
        :
        : "r" (REGION_NAME(Image$$, ER_DATA, $$Base)),
          "r" (REGION_NAME(Image$$, ARM_LIB_HEAP, $$ZI$$Limit))
        : "r0", "memory"
    );
}

int stm32_icache_remap(void)
{
	struct stm32_icache_region icache_reg;
	int err;

	icache_reg.n_region = 0;
	icache_reg.icache_addr = DDR_CAHB_ALIAS(DDR_CAHB_OFFSET);
	icache_reg.device_addr = DDR_CAHB2PHY_ALIAS(DDR_CAHB_OFFSET);
	icache_reg.size = 0x200000;
	icache_reg.slow_c_bus = true;

	err = stm32_icache_region_enable(&icache_reg);
	if (err)
		return err;

	return 0;
}

int stm32mp2_init_debug(void)
{
#if defined(DAUTH_NONE)
#elif defined(DAUTH_NS_ONLY)
#elif defined(DAUTH_FULL)
	BOOT_LOG_WRN("\033[1;31m*******************************\033[0m");
	BOOT_LOG_WRN("\033[1;31m* The debug port is full open *\033[0m");
	BOOT_LOG_WRN("\033[1;31m* wait debugger interrupt     *\033[0m");
	BOOT_LOG_WRN("\033[1;31m*******************************\033[0m");
	stm32_bsec_write_debug_conf(DBG_FULL);
        __WFI();
#else
#if !defined(DAUTH_CHIP_DEFAULT)
#error "No debug authentication setting is provided."
#endif
#endif
	return 0;
}
SYS_INIT(stm32mp2_init_debug, CORE, 11);

#ifdef STM32_BOOT_DEV_OSPI
static int stm32mp2_prepare_ddr_fw(void)
{
	int err, count;

	if (FLASH_DEV_FW_DDR_NAME.Initialize(NULL) != ARM_DRIVER_OK) {
		err = -ENODEV;
		goto error;
	}

	count = FLASH_DEV_FW_DDR_NAME.ReadData(FLASH_DEV_FW_DDR_OFFSET,
					(void*) DDR_FW_DEST_ADDR,
					DDR_FW_SIZE);
	if (count != DDR_FW_SIZE) {
		err = -EIO;
		goto error;
	}

	return 0;

error:
	EMSG("%s fail", __func__);
	return err;
}
SYS_INIT(stm32mp2_prepare_ddr_fw, CORE, 15);
#endif

#if defined(STM32_BOOT_DEV_SDMMC1) || defined(STM32_BOOT_DEV_SDMMC2)
extern struct flash_area flash_map[];
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

static int stm32mp2_prepare_fw(void)
{
	int count;
	const partition_entry_t *tfm_entry;

        tfm_entry = get_partition_entry("m33-fwa");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition tfm primary partition");
		return -EINVAL;
	}

	flash_map[0].fa_off = tfm_entry->start;
	flash_map[0].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33-fwb");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition tfm secondary partition");
		return -EINVAL;
	}

	flash_map[1].fa_off = tfm_entry->start;
	flash_map[1].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33-ddra");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition ddr fw primary partition");
		return -EINVAL;
	}

	count = FLASH_DEV_NAME.ReadData(tfm_entry->start,
					(void*) DDR_FW_DEST_ADDR,
					DDR_FW_SIZE);
	if (count != DDR_FW_SIZE) {
		BOOT_LOG_ERR("Failed to load ddr fw primary partition");
		return -EINVAL;
	}

	return 0;
}
SYS_INIT(stm32mp2_prepare_fw, CORE, 15);
#endif

/**
  * @brief  Platform init
  * @param  None
  * @retval status
  */
int32_t boot_platform_init(void)
{
	sys_init_run_level(INIT_LEVEL_PRE_CORE);
	sys_init_run_level(INIT_LEVEL_CORE);

	BOOT_LOG_INF("welcome");
	BOOT_LOG_INF("mcu sysclk: %d", SystemCoreClock);

#if defined(STM32_DDR_CACHED)
	{
		int err;

		err = stm32_icache_remap();
		if (err)
			return err;
	}
#endif

	return 0;
}

int32_t boot_platform_post_init(void)
{
	sys_init_run_level(INIT_LEVEL_POST_CORE);
	sys_init_run_level(INIT_LEVEL_REST);

	return 0;
}
