/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stm32mp_ddr_debug.h>
#include <lib/mmio.h>

#include <ddrphy_phyinit_usercustom.h>

/*
 * This function is called after the execution of training firmware is
 * complete. The user can use this function to override any CSR value programmed
 * by PhyInit or training firmware.
 *
 * The purpose of this function is to override any register values programmed
 * by the training firmware or ddrphy_phyinit_progcsrskiptrain() when
 * skip_train is used. This function is executed after training firmware has
 * completed.
 *
 * \note **Warning!**
 * This function must not override any values in userinputbasic,
 * userinputadvanced or message block data structures.  Only PHY register values
 * are to be modified with this function. To override PHY register values
 * programmed by PhyInit, users must use mmio_write_16() function calls placed
 * within this function.
 *
 * Sequence of Events in this function must be:
 * -# Enable APB access
 * -# Issue register writes
 * -# Isolate APB access
 *
 * A \ref examples/simple/ddrphy_phyinit_usercustom_customposttrain.c example of this function
 *
 * \return void
 */
void ddrphy_phyinit_usercustom_customposttrain(void)
{
	DDR_VERBOSE("%s Start\n", __func__);

	DDR_VERBOSE("%s End\n", __func__);
}

