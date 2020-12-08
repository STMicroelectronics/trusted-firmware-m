/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>

#include <cmsis.h>

#include <lib/utils_def.h>

#include <clk.h>
#include <stm32_systick.h>

#define COUNTER_MAX SysTick_LOAD_RELOAD_Msk
#define CYC_PER_TICK (SystemCoreClock / pdata.tick_hz)

static struct stm32_systick_platdata pdata;

__attribute__((weak))
int stm32_systick_get_platdata(struct stm32_systick_platdata *pdata)
{
	return -ENODEV;
}

static uint64_t ticks = 0U;

/*
 * calcul the next ticks value (current ticks + us timeout)
 */
uint64_t timeout_init_us(uint64_t us)
{
	ticks = stm32_systick_get_tick();
	return ticks + div_round_up(us * pdata.tick_hz, 1000000);
}

bool timeout_elapsed(uint64_t timeout)
{
	return stm32_systick_get_tick() > timeout;
}

void udelay(unsigned long us)
{
	uint64_t delayticks;

	delayticks = timeout_init_us(us);
	while (stm32_systick_get_tick() < delayticks);
}

/*
 * With 64bits, the ticks can't be roll while product lifecycle,
 * even if tick resolution is 1us
 */
uint64_t stm32_systick_get_tick(void)
{
	static uint32_t t1 = 0U, tdelta = 0U;
	uint32_t t2;

	if (!CYC_PER_TICK)
		goto out;

	t2 =  pdata.systick->VAL;

	if (t2 <= t1)
		tdelta += t1 - t2;
	else
		tdelta += t1 + pdata.systick->LOAD - t2;

	if (tdelta > CYC_PER_TICK){
		ticks += tdelta / CYC_PER_TICK;
		tdelta = tdelta % CYC_PER_TICK;
	}

	t1 = t2;
out:
	return ticks;
}

void stm32_systick_enable(void)
{
	pdata.systick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void stm32_systick_disable(void)
{
	pdata.systick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

int stm32_systick_probe(void)
{
/*    unsigned long clockfreq;*/
	/*
	 * not possible with old stm32mp2 driver.
	 * Gaby rework this part in next clk driver delivery
	 */
/*    clockfreq = clk_get_rate(pdata.clk_id);*/
/*    if (!clockfreq)*/
/*        return -EINVAL;*/

/*    SystemCoreClock = clockfreq;*/
	stm32_systick_enable();

	return 0;
}

void stm32_systick_config(unsigned long rate)
{
	SystemCoreClock = rate;
	stm32_systick_enable();
}

int stm32_systick_init(void)
{
	int err;

	err = stm32_systick_get_platdata(&pdata);
	if (err)
		return err;

	if (!pdata.tick_hz)
		return -EINVAL;

	pdata.systick->LOAD  = COUNTER_MAX;
	/* Load the SysTick Counter Value */
	pdata.systick->VAL   = 0UL;
	pdata.systick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk;

	return 0;
}
