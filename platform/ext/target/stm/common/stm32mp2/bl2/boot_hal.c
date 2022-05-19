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
#include <template/flash_otp_nv_counters_backend.h>

#include <plat_device.h>
#include <stm32_icache.h>
#include <stm32_bsec3.h>
#include <stm32_syscfg.h>
#include <stm32_pwr.h>
#include <stm32_ddr.h>

extern ARM_DRIVER_FLASH FLASH_DEV_NAME;
extern  struct flash_otp_nv_counters_region_t otp_stm_provision;

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

int init_debug(void)
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

int backup_domain_init(void)
{
	int err;

	stm32_pwr_backupd_wp(false);

	err = rstctrl_assert_to(STM32_RSTCTRL_REF(VSW_R), 1000);
	if (err)
		return err;

	err = rstctrl_deassert_to(STM32_RSTCTRL_REF(VSW_R), 1000);
	if (err)
		return err;

	return 0;
}

/**
  * @brief  Platform init
  * @param  None
  * @retval status
  */
int32_t boot_platform_init(void)
{
	int err;
	__IO uint32_t otp;

	/*  Place here to force linker to keep provision and init const */
	otp = otp_stm_provision.init_value;

	err = stm32_platform_bl2_init();
	if (err)
		return err;

	/* safe reset connector enabled */
	stm32_syscfg_safe_rst(true);

	/* Init for log */
#if MCUBOOT_LOG_LEVEL > MCUBOOT_LOG_LEVEL_OFF
	stdio_init();
#endif

	init_debug();

	BOOT_LOG_INF("welcome");
	BOOT_LOG_INF("mcu sysclk: %d", SystemCoreClock);

	err = stm32_ddr_init();
	if (err)
		return err;

#if defined(STM32_DDR_CACHED)
	err = stm32_icache_remap();
	if (err)
		return err;
#endif

	err = backup_domain_init();
	if (err)
		return err;

	err = FLASH_DEV_NAME.Initialize(NULL);
	if (err != ARM_DRIVER_OK)
		return err;

	return 0;
}
