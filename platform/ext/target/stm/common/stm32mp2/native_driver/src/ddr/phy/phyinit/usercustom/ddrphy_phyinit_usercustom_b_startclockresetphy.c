/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit_usercustom.h>

/*
 * The user can use this function to initiate the clocks and reset the
 * PHY.
 *
 * The default behavior of this function is to print comments relating to this
 * process. A function call of the same name will be printed in the output text
 * file. The user can choose to leave this function as is, or implement
 * mechanism within this function to trigger start clock and reset events in
 * simulation.
 *
 * Following is one possible sequence to reset the PHY. Other sequences are also
 * possible. See section "Clocks, Reset, Initialization" of the PUB for other
 * possible reset sequences.
 *
 * -# Drive PwrOkIn to 0. Note: Reset, DfiClk, and APBCLK can be X.
 * -# Start DfiClk and APBCLK
 * -# Drive Reset to 1 and PRESETn_APB to 0.
 *    Note: The combination of PwrOkIn=0 and Reset=1 signals a cold reset to the PHY.
 * -# Wait a minimum of 8 cycles.
 * -# Drive PwrOkIn to 1. Once the PwrOkIn is asserted (and Reset is still asserted),
 *    DfiClk synchronously switches to any legal input frequency.
 * -# Wait a minimum of 64 cycles. Note: This is the reset period for the PHY.
 * -# Drive Reset to 0. Note: All DFI and APB inputs must be driven at valid
 *    reset states before the de-assertion of Reset.
 * -# Wait a minimum of 1 Cycle.
 * -# Drive PRESETn_APB to 1 to de-assert reset on the ABP bus.
 * -# The PHY is now in the reset state and is ready to accept APB transactions.
 *
 * \return Void
 */
void ddrphy_phyinit_usercustom_b_startclockresetphy(void)
{
	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);
}
