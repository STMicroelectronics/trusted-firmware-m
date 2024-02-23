/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ddrphy_phyinit.h>

/*
 * Implements a wait function to Save-Retention firmware to finish.
 *
 * The purpose of this function is to wait for the Save-Retention (SR) Firmware to finish.
 * The number of DfiClk cycles to wait is specific in the statements below.
 *
 * The default behavior of ddrphy_phyinit_usercustom_sr_wait() is to print the comments
 * relating to its functions.
 *
 * User can edit this function to print differently, or implement a mechanism
 * to wait for the SR event to finish in simulation
 *
 *   - Save operation depends on PHY configurations.
 *   - Users may use the following table values as reference
 *
 *     Num of DBYTE |  DDR4  | LPDDR4
 *     ------------ | ------ | ------
 *     1            |  43k   | n/a
 *     4            |  85k   | 62k
 *     8            | 140k   | 96k
 *     9            | 155k   | 104k
 *
 * @returns void
 */
void ddrphy_phyinit_usercustom_sr_wait(void)
{
	/* - Wait for a SAVE_NUM_CYCLE of DFI clock cycles */

	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);
}
