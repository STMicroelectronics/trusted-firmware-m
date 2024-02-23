/*
 * Copyright (C) 2021-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include <stm32mp_ddr_debug.h>

#include <ddrphy_phyinit.h>

/*
 * Reads PhyInit inputs structures and sets relevant message block
 * parameters.
 *
 * This function sets Message Block parameters based on user_input_basic and
 * user_input_advanced. user changes in these files takes precedence
 * over this function call.
 *
 * MessageBlock fields set ::
 *
 *  - dramtype
 *  - pstate
 *  - dramfreq
 *  - pllbypassen
 *  - dfifreqratio
 *  - phyodtimpedance
 *  - phydrvimpedance
 *  - bpznresval
 *  - enableddqscha (LPDDR4)
 *  - cspresentcha (LPDDR4)
 *  - enableddqsChb (LPDDR4)
 *  - cspresentchb (LPDDR4)
 *  - enableddqs (DDR3/DDR4)
 *  - phycfg (DDR3/DDR4)
 *  - x16present (DDR4)
 *
 * \return 0 on success.
 */
int ddrphy_phyinit_calcmb(void)
{
	int myps = 0;
	int nad0 = userinputbasic.numactivedbytedfi0;
	int nad1 = 0;
	uint16_t mr4 __unused;

	DDR_VERBOSE("%s Start\n", __func__);

#if STM32MP_LPDDR4_TYPE
	nad1 = userinputbasic.numactivedbytedfi1;
#endif /* STM32MP_LPDDR4_TYPE */

	/* A few checks to make sure valid programming */
	if (nad0 <= 0 || nad1 < 0 || userinputbasic.numdbyte <= 0) {
		DDR_ERROR("%s numactivedbytedfi0, numactivedbytedfi0, NumByte out of range.\n",
		      __func__);
		return -1;
	}

	if ((nad0 + nad1) > userinputbasic.numdbyte) {
		DDR_ERROR("%s numactivedbytedfi0+numactivedbytedfi1 is larger than numdbyteDfi0\n",
		      __func__);
		return -1;
	}

	if (userinputbasic.dfi1exists == 0 && nad1 != 0) {
		DDR_ERROR("%s dfi1exists==0 but numdbyteDfi0 != 0\n", __func__);
		return -1;
	}

#if STM32MP_DDR4_TYPE
	/* OR all mr4 masked values, to help check in next loop */
	mr4 = 0;
	for (myps = 0; myps < userinputbasic.numpstates; myps++) {
		mr4 |= mb_ddr_1d[myps].mr4 & 0x1C0U;
	}
#endif /* STM32MP_DDR4_TYPE */

	/* 1D message block defaults */
	for (myps = 0; myps < userinputbasic.numpstates; myps++) {
		uint16_t disableddbyte __unused;
		int dbyte __unused;
		int ret;

#if STM32MP_DDR4_TYPE
		if (mr4 != 0x0) {
			DDR_ERROR("%s Setting DRAM CAL mode is not supported by the PHY.\n", __func__);
			DDR_ERROR("Memory controller may set CAL mode after PHY has entered mission\n");
			DDR_ERROR("mode. Please check value programmed in mb_ddr_1d[*].mr4\n");
			DDR_ERROR("and unset A8:6\n");
			return -1;
		}
#endif /* STM32MP_DDR4_TYPE */

#if STM32MP_DDR3_TYPE
		if (userinputbasic.dimmtype == NODIMM) {
			ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_DRAMTYPE, 0x1);
			if (ret != 0) {
				return ret;
			}
		}
#elif STM32MP_DDR4_TYPE
		if (userinputbasic.dimmtype == NODIMM) {
			ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_DRAMTYPE, 0x2);
			if (ret != 0) {
				return ret;
			}
		}
#endif /* STM32MP_DDR4_TYPE */

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_PSTATE, myps);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_DRAMFREQ,
					       userinputbasic.frequency[myps] * 2);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_PLLBYPASSEN,
					       userinputbasic.pllbypass[myps]);
		if (ret != 0) {
			return ret;
		}

		if (userinputbasic.dfifreqratio[myps] == 1) {
			ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_DFIFREQRATIO, 0x2);
			if (ret != 0) {
				return ret;
			}
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_PHYODTIMPEDANCE, 0);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_PHYDRVIMPEDANCE, 0);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_BPZNRESVAL, 0);
		if (ret != 0) {
			return ret;
		}

#if STM32MP_DDR3_TYPE || STM32MP_DDR4_TYPE
		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_ENABLEDDQS, nad0 * 8);
		if (ret != 0) {
			return ret;
		}

		disableddbyte = 0x0U;

		for (dbyte = 0; dbyte < userinputbasic.numdbyte && dbyte < 8; dbyte++) {
			disableddbyte |= (ddrphy_phyinit_isdbytedisabled(dbyte) ?
									(0x1U << dbyte) : 0x0U);
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_DISABLEDDBYTE, disableddbyte);
		if (ret != 0) {
			return ret;
		}

#if STM32MP_DDR3_TYPE
		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_PHYCFG,
					       userinputadvanced.is2ttiming[myps]);
		if (ret != 0) {
			return ret;
		}
#else
		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_PHYCFG,
					       (mb_ddr_1d[myps].mr3 & 0x8U) ?
					       0 : userinputadvanced.is2ttiming[myps]);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_X16PRESENT,
					       (0x10 == userinputbasic.dramdatawidth) ?
					       mb_ddr_1d[myps].cspresent : 0x0);
		if (ret != 0) {
			return ret;
		}
#endif /* STM32MP_DDR3_TYPE */
#elif STM32MP_LPDDR4_TYPE
		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_ENABLEDDQSCHA, nad0 * 8);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_CSPRESENTCHA,
					       (2 == userinputbasic.numrank_dfi0) ?
					       0x3 : userinputbasic.numrank_dfi0);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_ENABLEDDQSCHB, nad1 * 8);
		if (ret != 0) {
			return ret;
		}

		ret = ddrphy_phyinit_softsetmb(myps, MB_FIELD_CSPRESENTCHB,
					       (2 == userinputbasic.numrank_dfi1) ?
					       0x3 : userinputbasic.numrank_dfi1);
		if (ret != 0) {
			return ret;
		}
#endif /* STM32MP_LPDDR4_TYPE */
	} /* myps */

	DDR_VERBOSE("%s End\n", __func__);

	return 0;
}
