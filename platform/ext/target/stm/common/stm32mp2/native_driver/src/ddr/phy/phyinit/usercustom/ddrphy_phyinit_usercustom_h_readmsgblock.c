/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit.h>

/*
 * Reads training results
 *
 * Read the Firmware Message Block via APB read commands to the DMEM address to
 * obtain training results.
 *
 * The default behavior of this function is to print comments relating to this
 * process. An example pseudo code for implementing this function is as follows:
 *
 * @code{.c}
 * {
 *   _read_1d_message_block_outputs_
 * }
 * @endcode
 *
 * A function call of the same name will be printed in the output text file.
 * User can choose to leave this function as is, or implement mechanism to
 * trigger message block read events in simulation.
 *
 * \return void
 */
void ddrphy_phyinit_usercustom_h_readmsgblock(void)
{
	DDR_VERBOSE(" 2. Read the Firmware Message Block to obtain the results from the training.\n");
	DDR_VERBOSE(" This can be accomplished by issuing APB read commands to the DMEM addresses.\n");
	DDR_VERBOSE(" Example:\n");
	DDR_VERBOSE("   _read_1d_message_block_outputs_\n");

	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);
}
