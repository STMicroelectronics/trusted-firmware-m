/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024, STMicroelectronics
 *
 */

#include <device.h>
#include <mbox.h>
#include "region_defs.h"
#include "psa_manifest/pid.h"
#include <psa/service.h>
#include "spm.h"
#include "tfm_hal_interrupt.h"
#include "tfm_peripherals_def.h"
#include "load/interrupt_defs.h"
#include "psa_manifest/tfm_ipcc.h"


extern void IPCC_HANDLE_0();
extern struct irq_t ipcc_irq;

psa_flih_result_t ipcc_flih(void)
{
	IPCC_HANDLE_0();
	return PSA_FLIH_NO_SIGNAL;
}

psa_status_t tfm_ipcc_entry(void)
{
	psa_irq_enable(ipcc_irq.p_ildi->signal);
	do {
		psa_wait(ipcc_irq.p_ildi->signal, PSA_BLOCK);
#if IPCC_LEGACY
		ipcc_flih();
		psa_eoi(ipcc_irq.p_ildi->signal);
#else
		psa_panic();
#endif
	} while(1);

}
