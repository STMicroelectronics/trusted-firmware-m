/*
 * Copyright (C) 2021-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit.h>

/*
 * Loads Save/Restore firmware IMEM and DMEM image
 *
 * This function is used when the Micro-Controller is used to Save/Restore PHY registers.
 * \return void
 */
void ddrphy_phyinit_load_sr_fw(void)
{
	int i;
	int imem[SR_IMEM_SIZE];
	int imem_offset = 0;
	enum return_offset_lastaddr return_type = RETURN_OFFSET;

	DDR_VERBOSE("Start of loading Universal Resident Retention IMEM\n");

	/* Initialize the dmem structure */
	for (i = 0; i < SR_IMEM_SIZE; i++) {
		imem[i] = 0;
	}

	/* Start of loading Universal Resident Retention IMEM */
	imem_offset = ddrphy_phyinit_storeincvfile(SR_IMEM_INCV_FILENAME, imem, return_type);
	/* Write local imem array */
	ddrphy_phyinit_writeoutmem(imem, imem_offset, SR_IMEM_SIZE);

	DDR_VERBOSE("End of loading Universal Resident Retention IMEM\n");
}
