// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2026, STMicroelectronics
 */
#include <cmsis.h>
#include <lib/utils_def.h>
#include <remoteproc_driver.h>
#include <stddef.h>
#include <soc_config.h>

static remoteproc_crash_cb remoteproc_callback;
static int remoteproc_driver_init(void);
static int remoteproc_driver_deinit(void);
static int remoteproc_driver_register_crash_cb(remoteproc_crash_cb cb);
static int remoteproc_driver_unregister_crash_cb(remoteproc_crash_cb cb);

remoteprocdriver_t remoteproc_driver = {
	.init = remoteproc_driver_init,
	.deinit = remoteproc_driver_deinit,
	.register_crash_callback = remoteproc_driver_register_crash_cb,
	.unregister_crash_callback = remoteproc_driver_unregister_crash_cb,
};

void IWDG1_RST_IRQHandler(void)
{
	if (EXTI2_NS->FPR2 == EXTI2_IWDG_1)
		EXTI2_NS->FPR2 = EXTI2_IWDG_1;
	EXTI2_NS->FTSR2 |= EXTI2_IWDG_1;

	NVIC_ClearPendingIRQ(IWDG1_RST_IRQn);
	if (remoteproc_callback) {
		remoteproc_callback(0);
	}
}

static int remoteproc_driver_init(void)
{
	EXTI2_NS->FPR2 = EXTI2_IWDG_1;
	EXTI2_NS->FTSR2 |= EXTI2_IWDG_1;
	EXTI2_NS->C2IMR2 |= EXTI2_IWDG_1;

	NVIC_SetPriority(IWDG1_RST_IRQn, 1);
	NVIC_EnableIRQ(IWDG1_RST_IRQn);
	if (NVIC_GetEnableIRQ(IWDG1_RST_IRQn))
		return 0;

	return -1;
}

static int remoteproc_driver_deinit(void)
{
	NVIC_DisableIRQ(IWDG1_RST_IRQn);
	EXTI2_NS->FTSR2 &= ~EXTI2_IWDG_1;
	EXTI2_NS->C2IMR2 &= ~EXTI2_IWDG_1;
	remoteproc_callback = NULL;

	return 0;
}

static int remoteproc_driver_register_crash_cb(remoteproc_crash_cb cb)
{
	remoteproc_callback = cb;

	return 0;
}

static int remoteproc_driver_unregister_crash_cb(remoteproc_crash_cb cb)
{
	if (remoteproc_callback == cb) {
		remoteproc_callback = NULL;
		return 0;
	}
	return -1;
}
