/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef  __STM32_SYSTICK_H__
#define  __STM32_SYSTICK_H__

#include <stdint.h>
#include <cmsis.h>

/*
 * In a implementation with Security Extension, there are two 24-bit system timers,
 * a Non-secure SysTick timer and a Secure SysTick timer.
 * The Non-secure registers can be accessed in Secure state by using an aliased
 * address at offset 0x00020000 from the normal register address.
 * In Non-Secure state, only the NS systick is accessible and not by aliased address.
 */
#if (__DOMAIN_NS == 1)
#define systick_NS SysTick
#else
#define systick_S  SysTick
#define systick_NS SysTick_NS
#endif

struct stm32_systick_platdata {
	SysTick_Type *systick;
	unsigned long clk_id;
	uint32_t tick_hz;
};

int stm32_systick_init(void);
void stm32_systick_config(unsigned long rate);
void stm32_systick_enable(void);
void stm32_systick_disable(void);
uint64_t stm32_systick_get_tick(void);
#endif   /* ----- #ifndef __STM32_SYSTICK_H__  ----- */
