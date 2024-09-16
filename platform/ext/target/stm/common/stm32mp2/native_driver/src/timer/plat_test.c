/*
 * Copyright (c) 2024, STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#define DT_DRV_COMPAT st_stm32mp25_timer

#include <device.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>

#include <clk.h>
#include "tfm_plat_defs.h"
#include "tfm_plat_test.h"
#include "tfm_peripherals_def.h"
#include "psa/service.h"

#include "region_defs.h"
#include "psa_manifest/pid.h"
#include "spm_ipc.h"
#include "tfm_hal_interrupt.h"
#include "tfm_peripherals_def.h"
#include "load/interrupt_defs.h"
#include <stdint.h>

/* TIM TIMER register define */
#define  TIM_CR1			U(0x0)
#define  TIM_CR2                        U(0x04)
#define  TIM_SCMR                       U(0x08)
#define  TIM_DIER                       U(0x0C)
#define  TIM_SR                         U(0x10)
#define  TIM_EGR                        U(0x14)
#define  TIM_CNT                        U(0x24)
#define  TIM_ARR                        U(0x2C)
#define  TIM_PSC                        U(0x28)
#define  TIM_RCR                        U(0x30)

/* highest prescaler */
#define TIMER_PRESCALER 64000

#if (IS_ENABLED(STM32_SEC))

/* relies on unique timer instance */
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct timer_config {
	uintptr_t base;
	const struct device *clk_dev;
	clk_subsys_t clk_subsys;
	uint32_t irq;
};

/*  struct and variable used in tf-m-tests for interrupt test */
struct platform_data_t {};
const struct platform_data_t tfm_peripheral_timer2 = {0};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
"only one timer node is supported for this test");

const struct timer_config tim_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
        .clk_subsys = (clk_subsys_t)DT_INST_CLOCKS_CELL(0, bits),
	.irq = DT_INST_IRQN(0),
};

void tfm_plat_test_secure_timer_start(void)
{
	uint32_t reload_val;
	struct clk *clk;

	clk = clk_get(tim_cfg.clk_dev, tim_cfg.clk_subsys);
	if (!clk)
		psa_panic();

	clk_enable(clk);
	/* compute a timeout around  1 second  */
	reload_val = clk_get_rate(clk) / TIMER_PRESCALER;
	mmio_write_32(tim_cfg.base + TIM_CR1, 0);
	mmio_write_32(tim_cfg.base + TIM_SCMR, 0);
	mmio_write_32(tim_cfg.base + TIM_ARR, reload_val);
	mmio_write_32(tim_cfg.base + TIM_PSC, TIMER_PRESCALER);
	mmio_write_32(tim_cfg.base + TIM_RCR, 0);

	mmio_setbits_32(tim_cfg.base + TIM_EGR, TIM_EGR_UG);
	mmio_write_32(tim_cfg.base + TIM_SR, ~(TIM_SR_UIF));
	mmio_setbits_32(tim_cfg.base + TIM_DIER, TIM_DIER_UIE);
	mmio_setbits_32(tim_cfg.base + TIM_CR1, TIM_CR1_CEN);
	/*  enable intrerupt */
	NVIC_EnableIRQ(tim_cfg.irq);
}

void tfm_plat_test_secure_timer_stop(void)
{
	struct clk *clk = NULL;

	clk = clk_get(tim_cfg.clk_dev, tim_cfg.clk_subsys);
	if (!clk)
		psa_panic();

	mmio_clrbits_32(tim_cfg.base + TIM_CR1, TIM_CR1_CEN);
	mmio_write_32(tim_cfg.base + TIM_SR, ~(TIM_SR_UIF));

	clk_disable(clk);
	NVIC_DisableIRQ(tim_cfg.irq);
}

void tfm_plat_test_secure_timer_clear_intr(void)
{
	mmio_write_32(tim_cfg.base + TIM_SR, ~(TIM_SR_UIF));
}

#endif

#endif /* STM32_SEC */