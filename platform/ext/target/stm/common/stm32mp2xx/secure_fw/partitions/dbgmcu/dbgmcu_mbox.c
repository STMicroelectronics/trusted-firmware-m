/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025, STMicroelectronics
 *
 */

#include <debug.h>
#include <device.h>
#include <psa/error.h>
#include <psa/service.h>
#include <load/interrupt_defs.h>

extern void DBG_AUTH_HOST_HANDLE();
extern struct irq_t dbg_auth_host_irq;

psa_status_t tfm_dbgmcu_entry(void)
{
	psa_irq_enable(dbg_auth_host_irq.p_ildi->signal);
	do
	{
		psa_wait(dbg_auth_host_irq.p_ildi->signal, PSA_BLOCK);
		DBG_AUTH_HOST_HANDLE();
		psa_eoi(dbg_auth_host_irq.p_ildi->signal);
	} while(1);
}
