/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */
#include <stdint.h>
#include "cmsis.h"
#include "spm_ipc.h"
#include "tfm_hal_interrupt.h"
#include "ffm/interrupt.h"
#include "load/interrupt_defs.h"

extern enum tfm_hal_status_t mailbox_irq_init(void *p_pt,
                                              const struct irq_load_info_t
					      *p_ildi)
{
	return   TFM_HAL_SUCCESS;
}


