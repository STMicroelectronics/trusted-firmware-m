/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit_usercustom.h>

/*
 * This function must be used to trigger setting DfiClk to the
 * frequency associated with the input PState.
 *
 * The purpose of this function is to change DfiClk to the desired frequency for
 * the input PState before proceeding to the next step. The default behavior of
 * this function is to print comments relating to this process.  A function call
 * of the same name will be printed in the output text file. The PhyInit
 * ddrphy_phyinit_sequence() function calls this function multiple times in
 * order to set DfiClk before triggering training firmware execution for
 * different PStates. The User can edit this function to their needs in order to
 * implement this functionality.
 *
 * the clock should be stable at the new frequency. For more information on
 * clocking requirements, see "Clocks" section in the PUB documentation.
 *
 * \note this routine implies other items such as dfifreqratio, DfiCtlClk are
 * also set properly.  Because the clocks are controlled in the SOC, external to
 * the software and PHY, this step is intended to be replaced by the user with
 * the necessary SOC operations to achieve the new input frequency to the PHY.
 *
 * \return integer value = Pstate
 */
int ddrphy_phyinit_usercustom_e_setdficlk(int pstate) {
	/*!< Input Pstate indicating associated DfiClk requency*/

	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);

	return pstate;
}
