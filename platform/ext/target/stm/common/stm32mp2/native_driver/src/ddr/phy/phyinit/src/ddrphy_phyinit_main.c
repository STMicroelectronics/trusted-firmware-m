/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit.h>

extern FILE *outfileptr; /* defined in the ddrphy_phyinit_globals.c */
extern char *cmntstr; /* defined in the ddrphy_phyinit_globals.c */
extern char *apbstr; /* defined in the ddrphy_phyinit_globals.c */

/*
 * Main function of PhyInit standalone executable.
 *
 * Only used for the purpose of generating output.txt file. Parses input
 * arguments and makes a call to ddrphy_phyinit_sequence(for single PHY instance)
 *
 *  ### Required Arguments
 *
 *  - train2d <0|1>
 *    - if set to 0 only 1D training is executed in the sequence.
 *    - if set to 1 both 1D and 2D training is executed
 *    - ignored if skip_train !=0
 *
 *  - output filename
 *    - filename of the output txt file.
 *
 *  - skip_train <0|1|2>
 *    - if set to 0 firmware training is executed.
 *    - if set to 1 training is skipped and registers are programed to work with
 *      zero board delay.
 *    - if set to 2 training is used to skip training but execute the firmware to
 *      initialize the DRAM state while registers are programed to work with zero
 *      board delay.
 *
 *  #### Optional Arguments
 *
 *  - debug \<level\>
 *    - useful for generating additional print statements for debug purpose.
 *      Currently only level must be 0. default=0.
 *
 *  - comment_string
 *    - a custom comment string to place on non-system verilog compatible lines in
 *      the output txt files. default is an empty string.
 *
 *  - apb_string
 *    - a custom comment string to place on register write commands in
 *      the output txt files. default is an empty string.
 *
 *  - retention_exit
 *    - if set to 1 creates additional output_retention_exit.txt file with sequence for IO
 *      retention exit.
 *
 * Example: generating output txt file for 1D/2D training.
 * \code{.sh}
 * phyinit -train2d 1 -skip_train 0
 * \endcode
 *
 * @param argc number of input arguments.
 * @param argv char array of input arguments.
 */
int main(int argc, char *argv[])
{
	int skip_train = -1;
	int train2d = -1;
	int debug = 0;
	int retexit = 0;
	int i;
	char *outfilename = NULL;

	char *usage = "\
\n \
    phyinit execuable \n \
\n \
    This executable will generate a txt file output with register write instructions\n \
    to initialize the DDRPHY phy. \n \
    phyinit <required arguments> [options] \n \
\n \
    <required arguments> \n \
    -train2d <0|1> \n \
        if set to 0 only 1D training is executed in the sequenct. \n \
        if set to 1 both 1D and 2D training is executed \n \
        ignored if skip_train !=0 \n \
\n \
    -output filename \n \
        filename of the output txt file.\n \
\n \
    -skip_train <0|1|2> \n \
        if set to 0 firmware training is executed.\n \
        if set to 1 training is skipped and registers are programed to work with\n \
        zero board delay. \n \
        if set to 2 training is used to skip training but execute the firmware to\n \
        initialize the DRAM state while registers are programed to work with zero\n \
        board delay. \n \
\n \
    [optional arguments] \n \
    -debug <level> \n \
        useful for generating additional print statements for debug purpose.\n \
        Currently only level must be 0.\n \
\n \
    -comment_string \n \
        a custom comment string to place on non-system verilog compatible lines in\n \
        the output txt files. default is an empty string.\n \
\n \
    -apb_string \n \
        a custom comment string to place on register write commands in\n \
        the output txt files. default is an empty string.\n \
\n \
    - retention_exit <0|1> \n \
        if set to 1 creates additional output_retention_exit.txt file with sequence for IO retention exit.  \n \
\n \
   Example:\n \
   phyinit -train2d 1 -skip_train 0 -debug 1\n \
";

	for (i = 1; i < argc; i = i + 2) {
		if (strcmp("-skip_train", argv[i]) == 0) {
			skip_train = atoi(argv[i + 1]);
		} else if (strcmp("-train2d", argv[i]) == 0) {
			train2d = atoi(argv[i + 1]);
		} else if (strcmp("-debug", argv[i]) == 0) {
			debug = atoi(argv[i + 1]);
		} else if (strcmp("-comment_string", argv[i]) == 0) {
			cmntstr = argv[i + 1];
		} else if (strcmp("-apb_string", argv[i]) == 0) {
			apbstr = argv[i + 1];
		} else if (strcmp("-retention_exit", argv[i]) == 0) {
			retexit = atoi(argv[i + 1]);
		} else if (strcmp("-output", argv[i]) == 0) {
			outfilename = argv[i + 1];
		} else {
			DDR_ERROR("Unsupported argument %s is supplied.\n", argv[i]);
		}
	}

	if (train2d != 0 && train2d != 1) {
		DDR_ERROR("train2d(%d) no set or invalid input. See usage.\n%s\n", train2d, usage);
	}

	if (skip_train != 0 && skip_train != 1 && skip_train != 2) {
		DDR_ERROR("skip_train(%d) no set or invalid input. See usage.\n%s\n",
		      skip_train, usage);
	}

	if (outfilename == NULL) {
		DDR_ERROR("output file not specified. See usage.\n%s\n", usage);
	}

	if (cmntstr == NULL) {
		VDDR_ERROR("Comments String is NULL. See usage.\n%s\n", usage);
	}

	if (apbstr == NULL) {
		DDR_ERROR("abp_strings is NULL. See usage.\n%s\n", usage);
	}

	DDR_INFO("Running with values of skip_train = %0d, train2d = %0d, debug = %0d output=%s\n",
	     skip_train, train2d, debug, outfilename);

	/* Registering function inputs */
	runtimeconfig.skip_train = skip_train;
	runtimeconfig.debug = debug;
	runtimeconfig.reten = retexit;

	/* Function calls to generate output files when only one PHY instance is present */
	if (DDRPHY_NUM_PHY == 1) {
		/* Open Txt output Stream */
		if ((outfileptr = fopen(outfilename, "w")) == NULL) {
			DDR_ERROR("Can't open file for writing %s/\n\n", outfilename);
		} else {
			DDR_INFO("writing output file: %s\n\n", outfilename);
		}

		DDR_VERBOSE("Start of ddrphy_phyinit_main()\n");

		/* Execute phyinit sequence */
		ddrphy_phyinit_sequence(skip_train, train2d, debug);

		DDR_VERBOSE("End of ddrphy_phyinit_main()\n");

		fclose(outfileptr);

		if (retexit) {
			/* Printing retention exit sequence txt files */
			strcat(outfilename, "_retention_exit");

			/* Open Txt output Stream */
			if ((outfileptr = fopen(outfilename, "w")) == NULL) {
				DDR_ERROR("Can't open file for writing %s/\n\n", outfilename);
			} else {
				DDR_INFO("writing output file: %s\n\n", outfilename);
			}

			DDR_VERBOSE("Start of ddrphy_phyinit_retention_sequence()\n");

			/* Execute PhyInit retention exit sequence */
			ddrphy_phyinit_restore_sequence();

			DDR_VERBOSE("End of ddrphy_phyinit_retention_sequence()\n");

			fclose(outfileptr);
		}
	} else if (DDRPHY_NUM_PHY < 5 && DDRPHY_NUM_PHY > 1) {
		DDR_INFO("Start of multi sequence()\n");

		/* Multiple PHY instances. */

		DDR_ERROR("This release of PhyInit does not support multiple PHY instances.\n");
		DDR_ERROR("Please contact Support.\n");
		DDR_INFO("End of ddrphy_phyinit_multi_sequence()\n");
	} else {
		DDR_ERROR("invalid value for DDRPHY_NUM_PHY= %s\n", DDRPHY_NUM_PHY);
		return EXIT_FAILURE;
	}

	fflush(stdout);
	return EXIT_SUCCESS;
}
