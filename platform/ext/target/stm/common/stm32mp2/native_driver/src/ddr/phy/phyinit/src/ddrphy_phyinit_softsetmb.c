/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit.h>

/*
 * Set messageBlock variable only if not set by user
 *
 * This function is used by ddrphy_phyinit_calcmb() to set calculated
 * messageBlock variables only when the user has not directly programmed them.
 *
 * @param[in]   ps      integer between 0-3. Specifies the PState for which the messageBlock field should be set.
 * @param[in]   field   A string representing the messageBlock field to be programed.
 * @param[in]   value   filed value
 *
 * @return 0 on success.
 * On error  returns the following values based on error:
 * - -1 : message block field specified by the input \c field string is not
 * found in the message block data structure.
 * - -2 : when dramtype does not support 2D training but a 2D training field is
 * programmed.
 */
int ddrphy_phyinit_softsetmb(int ps, enum message_block_field field, int value)
{
	int ret = 0;

	switch (field) {
	case MB_FIELD_PSTATE:
		if (shdw_ddr_1d[ps].pstate == 0) {
			mb_ddr_1d[ps].pstate = value;
		}
		break;
	case MB_FIELD_PLLBYPASSEN:
		if (shdw_ddr_1d[ps].pllbypassen == 0) {
			mb_ddr_1d[ps].pllbypassen = value;
		}
		break;
	case MB_FIELD_DRAMFREQ:
		if (shdw_ddr_1d[ps].dramfreq == 0) {
			mb_ddr_1d[ps].dramfreq = value;
		}
		break;
	case MB_FIELD_DFIFREQRATIO:
		if (shdw_ddr_1d[ps].dfifreqratio == 0) {
			mb_ddr_1d[ps].dfifreqratio = value;
		}
		break;
	case MB_FIELD_BPZNRESVAL:
		if (shdw_ddr_1d[ps].bpznresval == 0) {
			mb_ddr_1d[ps].bpznresval = value;
		}
		break;
	case MB_FIELD_PHYODTIMPEDANCE:
		if (shdw_ddr_1d[ps].phyodtimpedance == 0) {
			mb_ddr_1d[ps].phyodtimpedance = value;
		}
		break;
	case MB_FIELD_PHYDRVIMPEDANCE:
		if (shdw_ddr_1d[ps].phydrvimpedance == 0) {
			mb_ddr_1d[ps].phydrvimpedance = value;
		}
		break;
#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
	case MB_FIELD_DRAMTYPE:
		if (shdw_ddr_1d[ps].dramtype == 0) {
			mb_ddr_1d[ps].dramtype = value;
		}
		break;
	case MB_FIELD_DISABLEDDBYTE:
		if (shdw_ddr_1d[ps].disableddbyte == 0) {
			mb_ddr_1d[ps].disableddbyte = value;
		}
		break;
	case MB_FIELD_ENABLEDDQS:
		if (shdw_ddr_1d[ps].enableddqs == 0) {
			mb_ddr_1d[ps].enableddqs = value;
		}
		break;
	case MB_FIELD_PHYCFG:
		if (shdw_ddr_1d[ps].phycfg == 0) {
			mb_ddr_1d[ps].phycfg = value;
		}
		break;
#endif /* STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE */
#if STM32MP_DDR4_TYPE
	case MB_FIELD_X16PRESENT:
		if (shdw_ddr_1d[ps].x16present == 0) {
			mb_ddr_1d[ps].x16present = value;
		}
		break;
#endif /* STM32MP_DDR4_TYPE */
#if STM32MP_LPDDR4_TYPE
	case MB_FIELD_ENABLEDDQSCHA:
		if (shdw_ddr_1d[ps].enableddqscha == 0) {
			mb_ddr_1d[ps].enableddqscha = value;
		}
		break;
	case MB_FIELD_CSPRESENTCHA:
		if (shdw_ddr_1d[ps].cspresentcha == 0) {
			mb_ddr_1d[ps].cspresentcha = value;
		}
		break;
	case MB_FIELD_ENABLEDDQSCHB:
		if (shdw_ddr_1d[ps].enableddqschb == 0) {
			mb_ddr_1d[ps].enableddqschb = value;
		}
		break;
	case MB_FIELD_CSPRESENTCHB:
		if (shdw_ddr_1d[ps].cspresentchb == 0) {
			mb_ddr_1d[ps].cspresentchb = value;
		}
		break;
#endif /* STM32MP_LPDDR4_TYPE */
	default:
		ERROR("unknown message block field %u\n", field);
		ret = -1;
		break;
	}

	return ret;
}
