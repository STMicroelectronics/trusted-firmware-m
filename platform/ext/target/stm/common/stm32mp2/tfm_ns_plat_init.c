/*
 * Copyright (C) 2020, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <Driver_Common.h>
#include <plat_device.h>
#include <uart_stdout.h>

int32_t tfm_ns_platform_init (void)
{
	if (stm32_platform_ns_init())
		return ARM_DRIVER_ERROR;

	stdio_init();

	return ARM_DRIVER_OK;
}
