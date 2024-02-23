/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>
#include <lib/mmio.h>

#include <ddrphy_phyinit.h>

/*
 * End of Micro-Controller based Save/Restore function.
 *
 * This function Halts the Micro Controller, Gate clocks in prepration for
 * mission mode during for the Micro Controller based Save/Restore sequence.
 *
 * \return void
 */
void ddrphy_phyinit_sr_complete_function(void)
{
	DDR_VERBOSE("%s Start\n", __func__);

	/*
	 * ##############################################################
	 *
	 * / To Halt Micro Controller after save operation completed
	 * CSRs to program:
	 *   MicroReset
	 *
	 * ##############################################################
	 */

	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICRORESET_ADDR)), 0x1U);

	/*
	 * ##############################################################
	 *
	 * To gain control of the APB bus
	 * To gate the micro controller clock and/or training hardware clock accordingly
	 * To isolate the APB bus access
	 * CSRs to program:
	 *   MicroContMuxSel UcclkHclkEnables
	 *
	 * ##############################################################
	 */

	/* Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0 */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICROCONTMUXSEL_ADDR)), 0x0U);

	/*
	 * For DDR3 and DDR4, gate-off Ucclk and Hclk after training is done
	 * LPDDR4 will need ACSM for PPT, and thus not gating Hclk (only gates Ucclk)
	 */
	if (userinputbasic.dramtype == DDR4 || userinputbasic.dramtype == DDR3) {
		/* Disabling Ucclk (PMU) and Hclk (training hardware) */
		mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TDRTUB | CSR_UCCLKHCLKENABLES_ADDR)),
			      0x0U);
	} else {
		/* Disabling Ucclk (PMU) */
		mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TDRTUB | CSR_UCCLKHCLKENABLES_ADDR)),
			      0x2U);
	}

	/* Isolate the APB access from the internal CSRs by setting the MicroContMuxSel CSR to 1 */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICROCONTMUXSEL_ADDR)), 0x1U);

	DDR_VERBOSE("%s End\n", __func__);
}

