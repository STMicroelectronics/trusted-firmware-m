/*
 * Copyright (c) 2024, STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>

#include "cmsis.h"
#include "spm_ipc.h"
#include "tfm_hal_interrupt.h"
#include "tfm_peripherals_def.h"
#include "ffm/interrupt.h"
#include "load/interrupt_defs.h"

struct irq_t mailbox_irq = {0};

extern enum tfm_hal_status_t mailbox_irq_init(void *p_pt,
					      const struct irq_load_info_t
					      *p_ildi)
{
	mailbox_irq.p_ildi = p_ildi;
	mailbox_irq.p_pt = p_pt;
	NVIC_SetPriority(p_ildi->source, 1);
	NVIC_ClearTargetState(p_ildi->source);
	NVIC_DisableIRQ(p_ildi->source);

	return TFM_HAL_SUCCESS;
}

void IPCC1_RX_S_IRQHandler(void)
{
	spm_handle_interrupt(mailbox_irq.p_pt, mailbox_irq.p_ildi);
}