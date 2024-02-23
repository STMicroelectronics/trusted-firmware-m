/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>
#include <lib/mmio.h>

#include <ddrphy_phyinit.h>

/*
 * Start of the Micro Controller based Save/Restore function
 *
 * Start of SR function.
 * Load Universal Resident Retention FW image and kick off Micro Controller,
 * Wait for save operation to be completed. The exact steps of the functions
 * are:
 */
void ddrphy_phyinit_sr_start_function(void)
{
	DDR_VERBOSE("%s Start\n", __func__);

	/*
	 * - Program MicroContMuxSel to gain control of the APB bus
	 * - Program UcclkHclkEnables to have PHY micro controller and training
	 *   hardware clocks enabled
	 */

	/* Enable access to the internal CSRs by setting the MicroContMuxSel CSR to 0 */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICROCONTMUXSEL_ADDR)), 0x0U);

	/* Enable Ucclk (PMU) and Hclk (training hardware) */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TDRTUB | CSR_UCCLKHCLKENABLES_ADDR)), 0x3U);

	/*
	 * - To load IMEM and DMEM with universal resident retention FW image by
	 *   calling ddrphy_phyinit_load_sr_fw().
	 */
	ddrphy_phyinit_load_sr_fw();

	/*
	 * - Program MicroContMuxSel:
	 *   - To give micro controller control of the APB bus
	 * - Program MicroReset :
	 *   - To execute universal resident retention FW image
	 */

	/*
	 * Allow Micro Controller to gain control of the APB bus by setting the MicroContMuxSel
	 * CSR to 1.
	 */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICROCONTMUXSEL_ADDR)), 0x1U);

	/* Halt and reset Micro Controller */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICRORESET_ADDR)), 0x9U);
	/* Halt Micro Controller */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICRORESET_ADDR)), 0x1U);
	/* Kick off Micro Controller and execute universal resident retention FW */
	mmio_write_16((uintptr_t)(DDRPHYC_BASE + 4 * (TAPBONLY | CSR_MICRORESET_ADDR)), 0x0U);

	/*
	 * - Wait for a SAVE_NUM_CYCLE of DFI clock cycles by calling
	 *   ddrphy_phyinit_usercustom_sr_wait().
	 */
	ddrphy_phyinit_usercustom_sr_wait();

	DDR_VERBOSE("%s End\n", __func__);
}
