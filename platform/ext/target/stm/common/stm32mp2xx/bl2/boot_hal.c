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

#include <boot_hal.h>
#include <cmsis.h>
#include <region.h>
#include <region_defs.h>
#include <Driver_Flash.h>
#include <bootutil/bootutil_log.h>
#include <init.h>
#include <debug.h>
#include <partition.h>
#include "flash_map/flash_map.h"

#include <stm32_bsec3.h>
#include <stm32_dcache.h>

#ifdef CRYPTO_HW_ACCELERATOR
#include "crypto_hw.h"
#endif /* CRYPTO_HW_ACCELERATOR */

extern ARM_DRIVER_FLASH FLASH_DEV_FW_DDR_NAME;
#ifdef STM32_BOOT_DEV_OSPI
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_0;
extern ARM_DRIVER_FLASH FLASH_DEV_NAME_2;
#endif

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

        tfm_entry = get_partition_entry("m33fw-a");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition tfm primary partition");
		return -EINVAL;
	}

	flash_map[0].fa_off = tfm_entry->start;
	flash_map[0].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33fw-b");
	if (tfm_entry == NULL) {
		BOOT_LOG_ERR("Could not find partition tfm secondary partition");
		return -EINVAL;
	}

	flash_map[1].fa_off = tfm_entry->start;
	flash_map[1].fa_size = tfm_entry->length;

	tfm_entry = get_partition_entry("m33ddr-a");
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

	if (IS_ENABLED(STM32_CACHE_ENABLED)) {
		int err;

		err = stm32_dcache_enable(true, true);
		if (err)
			return err;
	}

	sys_init_run_level(INIT_LEVEL_CORE);

	BOOT_LOG_INF("welcome");
	BOOT_LOG_INF("mcu sysclk: %d", SystemCoreClock);

	return 0;
}

int32_t boot_platform_post_init(void)
{
	sys_init_run_level(INIT_LEVEL_POST_CORE);
	sys_init_run_level(INIT_LEVEL_REST);

	return 0;
}

#if defined(STM32_CACHE_ENABLED)
/* Override for cache operations */
void boot_platform_quit(struct boot_arm_vector_table *vt)
{
	/*
	 * Clang at O0, stores variables on the stack with SP relative addressing.
	 * When manually set the SP then the place of reset vector is lost.
	 * Static variables are stored in 'data' or 'bss' section, change of SP has
	 * no effect on them.
	 */
	static struct boot_arm_vector_table *vt_cpy;
	int32_t result;

	#ifdef CRYPTO_HW_ACCELERATOR
	result = crypto_hw_accelerator_finish();
	if (result) {
		while (1){}
	}
	#endif /* CRYPTO_HW_ACCELERATOR */

	#ifdef FLASH_DEV_NAME
	result = FLASH_DEV_NAME.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME */
	#ifdef FLASH_DEV_NAME_2
	result = FLASH_DEV_NAME_2.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_2 */
	#ifdef FLASH_DEV_NAME_3
	result = FLASH_DEV_NAME_3.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_3 */
	#ifdef FLASH_DEV_NAME_SCRATCH
	result = FLASH_DEV_NAME_SCRATCH.Uninitialize();
	if (result != ARM_DRIVER_OK) {
		while(1) {}
	}
	#endif /* FLASH_DEV_NAME_SCRATCH */

	vt_cpy = vt;
	#if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) \
	|| defined(__ARM_ARCH_8_1M_MAIN__)
	/*
	 * Restore the Main Stack Pointer Limit register's reset value
	 * before passing execution to runtime firmware to make the
	 * bootloader transparent to it.
	 */
	__set_MSPLIM(0);
	#endif /* defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__) \
	|| defined(__ARM_ARCH_8_1M_MAIN__) */

	/* Invalidate all the memory range accessible by the M-33 */
	if (stm32_dcache_clean(0x0, 0xFFFFFFFF))
		panic();
	if (stm32_dcache_full_inv())
		panic();
	if (stm32_dcache_disable())
		panic();

	__set_MSP(vt_cpy->msp);
	__DSB();
	__ISB();

	boot_jump_to_next_image(vt_cpy->reset);
}
#endif
