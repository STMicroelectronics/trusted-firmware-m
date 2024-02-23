/*
 * Copyright (C) 2021-2023, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit.h>

/*
 * Reads firmware image incv file
 *
 * Routine to read an incv file into an internal mem array.
 *
 * \return the address of the first apb write in the incv file to use as the mem
 * offset.
 */
int ddrphy_phyinit_storeincvfile(char *incv_file_name, int mem[],
				 enum return_offset_lastaddr return_type)
{
	FILE *incvfile_ptr;
	char *p;
	char instr[255];
	int adr, dat, x, first, offset = 0;

	/* die if can't open incv file */
	if ((incvfile_ptr = fopen(incv_file_name, "r")) == NULL) {
		DDR_ERROR("%s:  Error opening input file %s/\n\n", __func__, incv_file_name);
	} else {
		DDR_INFO("%s Reading input file: %s\n\n", __func__, incv_file_name);
	}

	/*
	 * Assume entire incv file is made of lines that look like
	 * apb_wr(32'haaaa,16'hdddd);
	 * and capture the aaaa and dddd values to load array
	 */

	first = 0;
	while (fgets(instr, 255, incvfile_ptr) != NULL) {
		p = strtok(instr, "(");
		x = 0;
		do {
			p = strtok(NULL, "h,)");
			if (p) {
				if (x == 1) {
					sscanf(p, "%x", &adr);
				} else if (x == 3) {
					sscanf(p, "%x", &dat);
					if (first == 0) {
						offset = adr;
						first = 1;
					}
					mem[adr - offset] = dat; /* load array */
				}
			}
			x++;
		} while (p);
	}
	fclose(incvfile_ptr);

	if (return_type == RETURN_LASTADDR) {
		offset = adr; /*return the last addr */
	}

	return offset;
}
