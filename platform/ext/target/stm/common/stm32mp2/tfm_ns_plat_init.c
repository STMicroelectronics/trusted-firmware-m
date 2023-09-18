/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <Driver_Common.h>
#include <plat_device.h>
#include <uart_stdout.h>
#include <init.h>

int32_t tfm_ns_platform_init (void)
{
	sys_init_run_level(INIT_LEVEL_PRE_CORE);
	sys_init_run_level(INIT_LEVEL_CORE);

	if (stm32_platform_ns_init())
		return ARM_DRIVER_ERROR;

	stdio_init();

	sys_init_run_level(INIT_LEVEL_POST_CORE);

	return ARM_DRIVER_OK;
}
