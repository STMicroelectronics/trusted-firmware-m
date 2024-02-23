/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ddrphy_phyinit_usercustom.h>

/*
 * Initialize the PHY to Mission Mode through DFI Initialization
 *
 * The default behavior of this function is to print comments relating to this
 * process. User can choose to leave this function as is, or implement mechanism
 * to trigger DFI Initialization in simulation.
 *
 * Initialize the PHY to mission mode as follows:
 * -# Set the PHY input clocks to the desired frequency.
 * -# Initialize the PHY to mission mode by performing DFI Initialization. See
 * PUB Databook section on "DFI Frequency Change" for details on this step.
 *
 * \note to ensure DRAM MR state matches the destination frequency, the first
 * dfi_freq[4:0] must be to a PState matching the last trained PState.  For
 * Example 1) if 3 PStates are used and only 1D training is run, the first
 * dfi_freq[4:0] must be 0x3 on the first dfi_init_start transaction.
 * PState selected via dfi_freq[4:0] must match the
 * Example 2) if 3 PStates are used with 2D training enabled, the first
 * dfi_freq[4:0] must be 0x0 on the first dfi_init_start transaction.
 *
 * \note The PHY training firmware initializes the DRAM state. if skip
 * training is used, the DRAM state is not initialized.
 *
 *
 *
 * \returns void
 */
void ddrphy_phyinit_usercustom_j_entermissionmode(void)
{
	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);
}
