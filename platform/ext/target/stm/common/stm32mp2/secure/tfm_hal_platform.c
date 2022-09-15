/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <tfm_hal_platform.h>
#include <tfm_plat_defs.h>
#include <plat_device.h>
#include <target_cfg.h>

#include <uart_stdout.h>
#include <stm32_icache.h>
#include <stm32_dcache.h>
#include <debug.h>
#include <board_model.h>

extern const struct memory_region_limits memory_regions;

enum tfm_hal_status_t tfm_hal_platform_init(void)
{
	enum tfm_plat_err_t plat_err = TFM_PLAT_ERR_SYSTEM_ERR;

	plat_err = enable_fault_handlers();
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
		return TFM_HAL_ERROR_GENERIC;
	}

	plat_err = system_reset_cfg();
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
		return TFM_HAL_ERROR_GENERIC;
	}

	if (stm32_platform_s_init())
		return TFM_HAL_ERROR_GENERIC;

#if defined(STM32_DDR_CACHED)
	if (stm32_icache_enable(true, true))
		return TFM_HAL_ERROR_GENERIC;

	if (stm32_dcache_enable(true, true))
		return TFM_HAL_ERROR_GENERIC;
#endif

	__enable_irq();
	stdio_init();

	IMSG("welcome to "MODEL_FULLSTR);

	plat_err = nvic_interrupt_target_state_cfg();
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
		return TFM_HAL_ERROR_GENERIC;
	}

	plat_err = nvic_interrupt_enable();
	if (plat_err != TFM_PLAT_ERR_SUCCESS) {
		return TFM_HAL_ERROR_GENERIC;
	}

	return TFM_HAL_SUCCESS;
}

uint32_t tfm_hal_get_ns_VTOR(void)
{
    return memory_regions.non_secure_code_start;
}

uint32_t tfm_hal_get_ns_MSP(void)
{
    return *((uint32_t *)memory_regions.non_secure_code_start);
}

uint32_t tfm_hal_get_ns_entry_point(void)
{
    return *((uint32_t *)(memory_regions.non_secure_code_start + 4));
}
