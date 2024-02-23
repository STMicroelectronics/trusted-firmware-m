/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit_usercustom.h>

/*
 * This function can be used to apply power to the PHY.
 *
 * This function entry point in the initialization sequence can be used to
 * implement Step (A) of the initialization sequence described in PUB the
 * databook.
 *
 * The User can choose to leave this function as is, or implement a mechanism
 * to trigger power ramp-up event in simulation. In the output text file, this
 * function by default prints a call to a function of the same name.
 *
 * If the user chooses to use this function, the PHY expects all power pins
 * (VDD, VDDQ and VAA) to transition from off to on stage as part of this step.
 *
 * The power supplies can come up and stabilize in any order. While the power
 * supplies are coming up, all outputs will be unknown and the values of the
 * inputs are don't cares.
 *
 * @return Void
 *
 */
void ddrphy_phyinit_usercustom_a_bringuppower(void)
{
	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);
}
