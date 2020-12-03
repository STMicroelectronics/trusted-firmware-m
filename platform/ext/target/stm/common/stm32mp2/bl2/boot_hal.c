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

#include <cmsis.h>
#include <region.h>
#include <region_defs.h>
#include <Driver_Flash.h>
#include <uart_stdout.h>
#include <bootutil/bootutil_log.h>

#include <plat_device.h>

extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

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

/**
  * @brief  Platform init
  * @param  None
  * @retval status
  */
int32_t boot_platform_init(void)
{
	int err;

	err = stm32_platform_bl2_init();
	if (err)
		return err;

	/* Init for log */
#if MCUBOOT_LOG_LEVEL > MCUBOOT_LOG_LEVEL_OFF
	stdio_init();
#endif

	BOOT_LOG_INF("welcome");
	BOOT_LOG_INF("mcu sysclk: %d", SystemCoreClock);

	err = FLASH_DEV_NAME.Initialize(NULL);
	if (err != ARM_DRIVER_OK)
		return err;

	return 0;
}
