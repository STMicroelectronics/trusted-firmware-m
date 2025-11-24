/*
 * Copyright (c) 2024, STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include <cmsis.h>
#include <spm.h>
#include <tfm_hal_interrupt.h>
#include <tfm_peripherals_def.h>
#include <interrupt.h>
#include <load/interrupt_defs.h>

struct irq_t timer0_irq = {0};

void TFM_TIMER0_IRQ_Handler(void)
{
	spm_handle_interrupt(timer0_irq.p_pt, timer0_irq.p_ildi);
}

enum tfm_hal_status_t tfm_timer0_irq_init(void *p_pt,
					  const struct irq_load_info_t *p_ildi)
{
	timer0_irq.p_ildi = p_ildi;
	timer0_irq.p_pt = p_pt;

	NVIC_SetPriority(p_ildi->source, 1);
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);

	return TFM_HAL_SUCCESS;
}

void  TIM2_IRQHandler(void)
{
	TFM_TIMER0_IRQ_Handler(); /* Call the TFM handler. */
}
struct irq_t ipcc_irq = {0};
#if IPCC_LEGACY
extern enum tfm_hal_status_t ipcc_irq_init(void *p_pt,
					   const struct irq_load_info_t
					   *p_ildi)
{
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);
	return TFM_HAL_SUCCESS;
}
extern enum tfm_hal_status_t ipcc_irq_legacy_init(void *p_pt,
					   const struct irq_load_info_t
					   *p_ildi)
#else
extern enum tfm_hal_status_t ipcc_irq_init(void *p_pt,
					   const struct irq_load_info_t
					   *p_ildi)
#endif
{
	ipcc_irq.p_ildi = p_ildi;
	ipcc_irq.p_pt = p_pt;

	return TFM_HAL_SUCCESS;
}
#if !IPCC_LEGACY
extern enum tfm_hal_status_t ipcc_irq_legacy_init(void *p_pt,
					   const struct irq_load_info_t
					   *p_ildi)
{
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);
	return TFM_HAL_SUCCESS;
}
#endif 
void IPCC1_RX_S_IRQHandler(void)
{
	spm_handle_interrupt(ipcc_irq.p_pt, ipcc_irq.p_ildi);
}
struct irq_t test_sec_priv_irq = {0};

extern enum tfm_hal_status_t test_sec_priv_irq_init(void *p_pt,
					   const struct irq_load_info_t
					   *p_ildi)
{
	test_sec_priv_irq.p_ildi = p_ildi;
	test_sec_priv_irq.p_pt = p_pt;
	NVIC_SetPriority(p_ildi->source, 1);
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);

	return TFM_HAL_SUCCESS;
}

struct irq_t test_sec_npriv_irq = {0};

extern enum tfm_hal_status_t test_sec_npriv_irq_init(void *p_pt,
					   const struct irq_load_info_t
					   *p_ildi)
{
	test_sec_npriv_irq.p_ildi = p_ildi;
	test_sec_npriv_irq.p_pt = p_pt;
	NVIC_SetPriority(p_ildi->source, 1);
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);

	return TFM_HAL_SUCCESS;
}




void RESERVED_284_IRQHandler(void)
{
	spm_handle_interrupt(test_sec_priv_irq.p_pt, test_sec_priv_irq.p_ildi);
}

void RESERVED_285_IRQHandler(void)
{
	spm_handle_interrupt(test_sec_npriv_irq.p_pt, test_sec_npriv_irq.p_ildi);
}

struct irq_t dbg_auth_host_irq = {0};

enum tfm_hal_status_t dbg_auth_host_irq_init(void *p_pt,
					     const struct irq_load_info_t *p_ildi)
{
	dbg_auth_host_irq.p_ildi = p_ildi;
	dbg_auth_host_irq.p_pt = p_pt;
	NVIC_SetPriority(p_ildi->source, 1);
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);

	return TFM_HAL_SUCCESS;
}

void DBG_AUTH_HOST_IRQHandler(void)
{
	spm_handle_interrupt(dbg_auth_host_irq.p_pt, dbg_auth_host_irq.p_ildi);
}
