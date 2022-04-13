/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <stm32_pwr.h>

#define _PWR_CR1					U(0x00)
#define _PWR_CR2					U(0x04)
#define _PWR_CR3					U(0x08)
#define _PWR_CR4					U(0x0C)
#define _PWR_CR5					U(0x10)
#define _PWR_CR6					U(0x14)
#define _PWR_CR7					U(0x18)
#define _PWR_CR8					U(0x1C)
#define _PWR_CR9					U(0x20)
#define _PWR_CR10					U(0x24)
#define _PWR_CR11					U(0x28)
#define _PWR_CR12					U(0x2C)
#define _PWR_UCPDR					U(0x30)
#define _PWR_BDCR1					U(0x38)
#define _PWR_BDCR2					U(0x3C)
#define _PWR_CPU1CR					U(0x40)
#define _PWR_CPU2CR					U(0x44)
#define _PWR_CPU3CR					U(0x48)
#define _PWR_D1CR					U(0x4C)
#define _PWR_D2CR					U(0x50)
#define _PWR_D3CR					U(0x54)
#define _PWR_WKUPCR1				U(0x60)
#define _PWR_WKUPCR2				U(0x64)
#define _PWR_WKUPCR3				U(0x68)
#define _PWR_WKUPCR4				U(0x6C)
#define _PWR_WKUPCR5				U(0x70)
#define _PWR_WKUPCR6				U(0x74)
#define _PWR_D3WKUPENR				U(0x98)
#define _PWR_RSECCFGR				U(0x100)
#define _PWR_RPRIVCFGR				U(0x104)
#define _PWR_R0CIDCFGR				U(0x108)
#define _PWR_R1CIDCFGR				U(0x10C)
#define _PWR_R2CIDCFGR				U(0x110)
#define _PWR_R3CIDCFGR				U(0x114)
#define _PWR_R4CIDCFGR				U(0x118)
#define _PWR_WIOSECCFGR				U(0x180)
#define _PWR_WIOPRIVCFGR			U(0x184)
#define _PWR_WIO1CIDCFGR			U(0x188)
#define _PWR_WIO1SEMCR				U(0x18C)
#define _PWR_WIO2CIDCFGR			U(0x190)
#define _PWR_WIO2SEMCR				U(0x194)
#define _PWR_WIO3CIDCFGR			U(0x198)
#define _PWR_WIO3SEMCR				U(0x19C)
#define _PWR_WIO4CIDCFGR			U(0x1A0)
#define _PWR_WIO4SEMCR				U(0x1A4)
#define _PWR_WIO5CIDCFGR			U(0x1A8)
#define _PWR_WIO5SEMCR				U(0x1AC)
#define _PWR_WIO6CIDCFGR			U(0x1B0)
#define _PWR_WIO6SEMCR				U(0x1B4)
#define _PWR_CPU1D1SR				U(0x200)
#define _PWR_CPU2D2SR				U(0x204)
#define _PWR_CPU3D3SR				U(0x208)
#define _PWR_VERR					U(0x3F4)
#define _PWR_IPIDR					U(0x3F8)
#define _PWR_SIDR					U(0x3FC)

/* PWR_CR2 register fields */
#define _PWR_CR2_MONEN				BIT(0)
#define _PWR_CR2_VBATL				BIT(8)
#define _PWR_CR2_VBATH				BIT(9)
#define _PWR_CR2_TEMPL				BIT(10)
#define _PWR_CR2_TEMPH				BIT(11)

/* PWR_CR3 register fields */
#define _PWR_CR3_PVDEN				BIT(0)
#define _PWR_CR3_PVDO				BIT(8)

/* PWR_CR5 register fields */
#define _PWR_CR5_VCOREMONEN			BIT(0)
#define _PWR_CR5_VCOREL				BIT(8)
#define _PWR_CR5_VCOREH				BIT(9)

/* PWR_CR6 register fields */
#define _PWR_CR6_VCPUMONEN			BIT(0)
#define _PWR_CR6_VCPULLS			BIT(4)
#define _PWR_CR6_VCPUL				BIT(8)
#define _PWR_CR6_VCPUH				BIT(9)

/* PWR_CR7 register fields */

/* PWR_CR8 register fields */
#define _PWR_CR8_IOEMMCVMEN			BIT(0)
#define _PWR_CR8_IOOCTOSPI1VMEN		BIT(1)
#define _PWR_CR8_IOOCTOSPI2VMEN		BIT(2)
#define _PWR_CR8_IOSDVMEN			BIT(3)
#define _PWR_CR8_USB3V3VMEN			BIT(4)
#define _PWR_CR8_USBCCVMEN			BIT(5)
#define _PWR_CR8_AVMEN				BIT(6)
#define _PWR_CR8_IOEMMCSV			BIT(8)
#define _PWR_CR8_IOOCTOSPI1SV		BIT(9)
#define _PWR_CR8_IOOCTOSPI2SV		BIT(10)
#define _PWR_CR8_IOSDSV				BIT(11)
#define _PWR_CR8_USB3V3SV			BIT(12)
#define _PWR_CR8_USBCCSV			BIT(13)
#define _PWR_CR8_ASV				BIT(14)
#define _PWR_CR8_IOEMMCRDY			BIT(16)
#define _PWR_CR8_IOOCTOSPI1RDY		BIT(17)
#define _PWR_CR8_IOOCTOSPI2RDY		BIT(18)
#define _PWR_CR8_IOSDRDY			BIT(19)
#define _PWR_CR8_USB3V3RDY			BIT(20)
#define _PWR_CR8_USBCCRDY			BIT(21)
#define _PWR_CR8_ARDY				BIT(22)
#define _PWR_CR8_GPVMO				BIT(31)

/* PWR_CR9 register fields */
#define _PWR_CR9_BKPRBSEN			BIT(0)
#define _PWR_CR9_LPR1BSEN			BIT(4)

/* PWR_CR10 register fields */
#define _PWR_CR10_RETRBSEN_MASK		GENMASK(1, 0)
#define _PWR_CR10_RETRBSEN_SHIFT	0

/* PWR_CR11 register fields */
#define _PWR_CR11_DDRRETDIS			BIT(0)

/* PWR_CR12 register fields */
#define _PWR_CR12_GPUPDEN			BIT(0)
#define _PWR_CR12_GPUPDRDY			BIT(31)

/* PWR_UCPDR register fields */
#define _PWR_UCPDR_UCPD_DBDIS		BIT(0)
#define _PWR_UCPDR_UCPD_STBY		BIT(1)

/* PWR_BDCR1 register fields */
#define _PWR_BDCR1_DBD3P			BIT(0)

/* PWR_BDCR2 register fields */
#define _PWR_BDCR2_DBP				BIT(0)

/* PWR_CPU1CR register fields */
#define _PWR_CPU1CR_PDDS_D2			BIT(0)
#define _PWR_CPU1CR_PDDS_D1			BIT(1)
#define _PWR_CPU1CR_VBF				BIT(4)
#define _PWR_CPU1CR_STOPF			BIT(5)
#define _PWR_CPU1CR_SBF				BIT(6)
#define _PWR_CPU1CR_SBF_D1			BIT(7)
#define _PWR_CPU1CR_SBF_D3			BIT(8)
#define _PWR_CPU1CR_CSSF			BIT(9)
#define _PWR_CPU1CR_STANDBYWFIL2	BIT(15)
#define _PWR_CPU1CR_LPDS_D1			BIT(16)
#define _PWR_CPU1CR_LVDS_D1			BIT(17)

/* PWR_CPU2CR register fields */
#define _PWR_CPU2CR_PDDS_D2			BIT(0)
#define _PWR_CPU2CR_VBF				BIT(4)
#define _PWR_CPU2CR_STOPF			BIT(5)
#define _PWR_CPU2CR_SBF				BIT(6)
#define _PWR_CPU2CR_SBF_D2			BIT(7)
#define _PWR_CPU2CR_SBF_D3			BIT(8)
#define _PWR_CPU2CR_CSSF			BIT(9)
#define _PWR_CPU2CR_DEEPSLEEP		BIT(15)
#define _PWR_CPU2CR_LPDS_D2			BIT(16)
#define _PWR_CPU2CR_LVDS_D2			BIT(17)

/* PWR_CPU3CR register fields */
#define _PWR_CPU3CR_VBF				BIT(4)
#define _PWR_CPU3CR_SBF_D3			BIT(8)
#define _PWR_CPU3CR_CSSF			BIT(9)
#define _PWR_CPU3CR_DEEPSLEEP		BIT(15)

/* PWR_D1CR register fields */
#define _PWR_D1CR_LPCFG_D1			BIT(0)
#define _PWR_D1CR_POPL_D1_MASK		GENMASK(12, 8)
#define _PWR_D1CR_POPL_D1_SHIFT		8

/* PWR_D2CR register fields */
#define _PWR_D2CR_LPCFG_D2			BIT(0)
#define _PWR_D2CR_POPL_D2_MASK		GENMASK(12, 8)
#define _PWR_D2CR_POPL_D2_SHIFT		8
#define _PWR_D2CR_LPLVDLY_D2_MASK	GENMASK(18, 16)
#define _PWR_D2CR_LPLVDLY_D2_SHIFT	16

/* PWR_D3CR register fields */
#define _PWR_D3CR_PDDS_D3			BIT(0)
#define _PWR_D3CR_D3RDY				BIT(31)

/* PWR_WKUPCR1 register fields */
#define _PWR_WKUPCR1_WKUPC			BIT(0)
#define _PWR_WKUPCR1_WKUPP			BIT(8)
#define _PWR_WKUPCR1_WKUPPUPD_MASK	GENMASK(13, 12)
#define _PWR_WKUPCR1_WKUPPUPD_SHIFT	12
#define _PWR_WKUPCR1_WKUPENCPU1		BIT(16)
#define _PWR_WKUPCR1_WKUPENCPU2		BIT(17)
#define _PWR_WKUPCR1_WKUPF			BIT(31)

/* PWR_WKUPCR2 register fields */
#define _PWR_WKUPCR2_WKUPC			BIT(0)
#define _PWR_WKUPCR2_WKUPP			BIT(8)
#define _PWR_WKUPCR2_WKUPPUPD_MASK	GENMASK(13, 12)
#define _PWR_WKUPCR2_WKUPPUPD_SHIFT	12
#define _PWR_WKUPCR2_WKUPENCPU1		BIT(16)
#define _PWR_WKUPCR2_WKUPENCPU2		BIT(17)
#define _PWR_WKUPCR2_WKUPF			BIT(31)

/* PWR_WKUPCR3 register fields */
#define _PWR_WKUPCR3_WKUPC			BIT(0)
#define _PWR_WKUPCR3_WKUPP			BIT(8)
#define _PWR_WKUPCR3_WKUPPUPD_MASK	GENMASK(13, 12)
#define _PWR_WKUPCR3_WKUPPUPD_SHIFT	12
#define _PWR_WKUPCR3_WKUPENCPU1		BIT(16)
#define _PWR_WKUPCR3_WKUPENCPU2		BIT(17)
#define _PWR_WKUPCR3_WKUPF			BIT(31)

/* PWR_WKUPCR4 register fields */
#define _PWR_WKUPCR4_WKUPC			BIT(0)
#define _PWR_WKUPCR4_WKUPP			BIT(8)
#define _PWR_WKUPCR4_WKUPPUPD_MASK	GENMASK(13, 12)
#define _PWR_WKUPCR4_WKUPPUPD_SHIFT	12
#define _PWR_WKUPCR4_WKUPENCPU1		BIT(16)
#define _PWR_WKUPCR4_WKUPENCPU2		BIT(17)
#define _PWR_WKUPCR4_WKUPF			BIT(31)

/* PWR_WKUPCR5 register fields */
#define _PWR_WKUPCR5_WKUPC			BIT(0)
#define _PWR_WKUPCR5_WKUPP			BIT(8)
#define _PWR_WKUPCR5_WKUPPUPD_MASK	GENMASK(13, 12)
#define _PWR_WKUPCR5_WKUPPUPD_SHIFT	12
#define _PWR_WKUPCR5_WKUPENCPU1		BIT(16)
#define _PWR_WKUPCR5_WKUPENCPU2		BIT(17)
#define _PWR_WKUPCR5_WKUPF			BIT(31)

/* PWR_WKUPCR6 register fields */
#define _PWR_WKUPCR6_WKUPC			BIT(0)
#define _PWR_WKUPCR6_WKUPP			BIT(8)
#define _PWR_WKUPCR6_WKUPPUPD_MASK	GENMASK(13, 12)
#define _PWR_WKUPCR6_WKUPPUPD_SHIFT	12
#define _PWR_WKUPCR6_WKUPENCPU1		BIT(16)
#define _PWR_WKUPCR6_WKUPENCPU2		BIT(17)
#define _PWR_WKUPCR6_WKUPF			BIT(31)

/* PWR_D3WKUPENR register fields */
#define _PWR_D3WKUPENR_TAMP_WKUPEN_D3		BIT(0)

/* PWR_RSECCFGR register fields */
#define _PWR_RSECCFGR_RSEC0			BIT(0)
#define _PWR_RSECCFGR_RSEC1			BIT(1)
#define _PWR_RSECCFGR_RSEC2			BIT(2)
#define _PWR_RSECCFGR_RSEC3			BIT(3)
#define _PWR_RSECCFGR_RSEC4			BIT(4)

/* PWR_RPRIVCFGR register fields */
#define _PWR_RPRIVCFGR_RPRIV0		BIT(0)
#define _PWR_RPRIVCFGR_RPRIV1		BIT(1)
#define _PWR_RPRIVCFGR_RPRIV2		BIT(2)
#define _PWR_RPRIVCFGR_RPRIV3		BIT(3)
#define _PWR_RPRIVCFGR_RPRIV4		BIT(4)

/* PWR_R0CIDCFGR register fields */
#define _PWR_R0CIDCFGR_CFEN			BIT(0)
#define _PWR_R0CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_R0CIDCFGR_SCID_SHIFT	4
#define _PWR_R0CIDCFGR_PRDEN		BIT(14)

/* PWR_R1CIDCFGR register fields */
#define _PWR_R1CIDCFGR_CFEN			BIT(0)
#define _PWR_R1CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_R1CIDCFGR_SCID_SHIFT	4
#define _PWR_R1CIDCFGR_PRDEN		BIT(14)

/* PWR_R2CIDCFGR register fields */
#define _PWR_R2CIDCFGR_CFEN			BIT(0)
#define _PWR_R2CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_R2CIDCFGR_SCID_SHIFT	4
#define _PWR_R2CIDCFGR_PRDEN		BIT(14)

/* PWR_R3CIDCFGR register fields */
#define _PWR_R3CIDCFGR_CFEN			BIT(0)
#define _PWR_R3CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_R3CIDCFGR_SCID_SHIFT	4
#define _PWR_R3CIDCFGR_PRDEN		BIT(14)

/* PWR_R4CIDCFGR register fields */
#define _PWR_R4CIDCFGR_CFEN			BIT(0)
#define _PWR_R4CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_R4CIDCFGR_SCID_SHIFT	4
#define _PWR_R4CIDCFGR_PRDEN		BIT(14)

/* PWR_WIOSECCFGR register fields */
#define _PWR_WIOSECCFGR_WIOSEC1		BIT(0)
#define _PWR_WIOSECCFGR_WIOSEC2		BIT(1)
#define _PWR_WIOSECCFGR_WIOSEC3		BIT(2)
#define _PWR_WIOSECCFGR_WIOSEC4		BIT(3)
#define _PWR_WIOSECCFGR_WIOSEC5		BIT(4)
#define _PWR_WIOSECCFGR_WIOSEC6		BIT(5)

/* PWR_WIOPRIVCFGR register fields */
#define _PWR_WIOPRIVCFGR_WIOPRIV1	BIT(0)
#define _PWR_WIOPRIVCFGR_WIOPRIV2	BIT(1)
#define _PWR_WIOPRIVCFGR_WIOPRIV3	BIT(2)
#define _PWR_WIOPRIVCFGR_WIOPRIV4	BIT(3)
#define _PWR_WIOPRIVCFGR_WIOPRIV5	BIT(4)
#define _PWR_WIOPRIVCFGR_WIOPRIV6	BIT(5)

/* PWR_WIO1CIDCFGR register fields */
#define _PWR_WIO1CIDCFGR_CFEN		BIT(0)
#define _PWR_WIO1CIDCFGR_SEM_EN		BIT(1)
#define _PWR_WIO1CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_WIO1CIDCFGR_SCID_SHIFT	4
#define _PWR_WIO1CIDCFGR_PRDEN		BIT(14)
#define _PWR_WIO1CIDCFGR_SEMWLC0	BIT(16)
#define _PWR_WIO1CIDCFGR_SEMWLC1	BIT(17)
#define _PWR_WIO1CIDCFGR_SEMWLC2	BIT(18)
#define _PWR_WIO1CIDCFGR_SEMWLC3	BIT(19)
#define _PWR_WIO1CIDCFGR_SEMWLC4	BIT(20)
#define _PWR_WIO1CIDCFGR_SEMWLC5	BIT(21)
#define _PWR_WIO1CIDCFGR_SEMWLC6	BIT(22)
#define _PWR_WIO1CIDCFGR_SEMWLC7	BIT(23)

/* PWR_WIO1SEMCR register fields */
#define _PWR_WIO1SEMCR_SEM_MUTEX	BIT(0)
#define _PWR_WIO1SEMCR_SEMCID_MASK	GENMASK(6, 4)
#define _PWR_WIO1SEMCR_SEMCID_SHIFT	4

/* PWR_WIO2CIDCFGR register fields */
#define _PWR_WIO2CIDCFGR_CFEN		BIT(0)
#define _PWR_WIO2CIDCFGR_SEM_EN		BIT(1)
#define _PWR_WIO2CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_WIO2CIDCFGR_SCID_SHIFT	4
#define _PWR_WIO2CIDCFGR_PRDEN		BIT(14)
#define _PWR_WIO2CIDCFGR_SEMWLC0	BIT(16)
#define _PWR_WIO2CIDCFGR_SEMWLC1	BIT(17)
#define _PWR_WIO2CIDCFGR_SEMWLC2	BIT(18)
#define _PWR_WIO2CIDCFGR_SEMWLC3	BIT(19)
#define _PWR_WIO2CIDCFGR_SEMWLC4	BIT(20)
#define _PWR_WIO2CIDCFGR_SEMWLC5	BIT(21)
#define _PWR_WIO2CIDCFGR_SEMWLC6	BIT(22)
#define _PWR_WIO2CIDCFGR_SEMWLC7	BIT(23)

/* PWR_WIO2SEMCR register fields */
#define _PWR_WIO2SEMCR_SEM_MUTEX	BIT(0)
#define _PWR_WIO2SEMCR_SEMCID_MASK	GENMASK(6, 4)
#define _PWR_WIO2SEMCR_SEMCID_SHIFT	4

/* PWR_WIO3CIDCFGR register fields */
#define _PWR_WIO3CIDCFGR_CFEN		BIT(0)
#define _PWR_WIO3CIDCFGR_SEM_EN		BIT(1)
#define _PWR_WIO3CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_WIO3CIDCFGR_SCID_SHIFT	4
#define _PWR_WIO3CIDCFGR_PRDEN		BIT(14)
#define _PWR_WIO3CIDCFGR_SEMWLC0	BIT(16)
#define _PWR_WIO3CIDCFGR_SEMWLC1	BIT(17)
#define _PWR_WIO3CIDCFGR_SEMWLC2	BIT(18)
#define _PWR_WIO3CIDCFGR_SEMWLC3	BIT(19)
#define _PWR_WIO3CIDCFGR_SEMWLC4	BIT(20)
#define _PWR_WIO3CIDCFGR_SEMWLC5	BIT(21)
#define _PWR_WIO3CIDCFGR_SEMWLC6	BIT(22)
#define _PWR_WIO3CIDCFGR_SEMWLC7	BIT(23)

/* PWR_WIO3SEMCR register fields */
#define _PWR_WIO3SEMCR_SEM_MUTEX	BIT(0)
#define _PWR_WIO3SEMCR_SEMCID_MASK	GENMASK(6, 4)
#define _PWR_WIO3SEMCR_SEMCID_SHIFT	4

/* PWR_WIO4CIDCFGR register fields */
#define _PWR_WIO4CIDCFGR_CFEN		BIT(0)
#define _PWR_WIO4CIDCFGR_SEM_EN		BIT(1)
#define _PWR_WIO4CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_WIO4CIDCFGR_SCID_SHIFT	4
#define _PWR_WIO4CIDCFGR_PRDEN		BIT(14)
#define _PWR_WIO4CIDCFGR_SEMWLC0	BIT(16)
#define _PWR_WIO4CIDCFGR_SEMWLC1	BIT(17)
#define _PWR_WIO4CIDCFGR_SEMWLC2	BIT(18)
#define _PWR_WIO4CIDCFGR_SEMWLC3	BIT(19)
#define _PWR_WIO4CIDCFGR_SEMWLC4	BIT(20)
#define _PWR_WIO4CIDCFGR_SEMWLC5	BIT(21)
#define _PWR_WIO4CIDCFGR_SEMWLC6	BIT(22)
#define _PWR_WIO4CIDCFGR_SEMWLC7	BIT(23)

/* PWR_WIO4SEMCR register fields */
#define _PWR_WIO4SEMCR_SEM_MUTEX	BIT(0)
#define _PWR_WIO4SEMCR_SEMCID_MASK	GENMASK(6, 4)
#define _PWR_WIO4SEMCR_SEMCID_SHIFT	4

/* PWR_WIO5CIDCFGR register fields */
#define _PWR_WIO5CIDCFGR_CFEN		BIT(0)
#define _PWR_WIO5CIDCFGR_SEM_EN		BIT(1)
#define _PWR_WIO5CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_WIO5CIDCFGR_SCID_SHIFT	4
#define _PWR_WIO5CIDCFGR_PRDEN		BIT(14)
#define _PWR_WIO5CIDCFGR_SEMWLC0	BIT(16)
#define _PWR_WIO5CIDCFGR_SEMWLC1	BIT(17)
#define _PWR_WIO5CIDCFGR_SEMWLC2	BIT(18)
#define _PWR_WIO5CIDCFGR_SEMWLC3	BIT(19)
#define _PWR_WIO5CIDCFGR_SEMWLC4	BIT(20)
#define _PWR_WIO5CIDCFGR_SEMWLC5	BIT(21)
#define _PWR_WIO5CIDCFGR_SEMWLC6	BIT(22)
#define _PWR_WIO5CIDCFGR_SEMWLC7	BIT(23)

/* PWR_WIO5SEMCR register fields */
#define _PWR_WIO5SEMCR_SEM_MUTEX	BIT(0)
#define _PWR_WIO5SEMCR_SEMCID_MASK	GENMASK(6, 4)
#define _PWR_WIO5SEMCR_SEMCID_SHIFT	4

/* PWR_WIO6CIDCFGR register fields */
#define _PWR_WIO6CIDCFGR_CFEN		BIT(0)
#define _PWR_WIO6CIDCFGR_SEM_EN		BIT(1)
#define _PWR_WIO6CIDCFGR_SCID_MASK	GENMASK(6, 4)
#define _PWR_WIO6CIDCFGR_SCID_SHIFT	4
#define _PWR_WIO6CIDCFGR_PRDEN		BIT(14)
#define _PWR_WIO6CIDCFGR_SEMWLC0	BIT(16)
#define _PWR_WIO6CIDCFGR_SEMWLC1	BIT(17)
#define _PWR_WIO6CIDCFGR_SEMWLC2	BIT(18)
#define _PWR_WIO6CIDCFGR_SEMWLC3	BIT(19)
#define _PWR_WIO6CIDCFGR_SEMWLC4	BIT(20)
#define _PWR_WIO6CIDCFGR_SEMWLC5	BIT(21)
#define _PWR_WIO6CIDCFGR_SEMWLC6	BIT(22)
#define _PWR_WIO6CIDCFGR_SEMWLC7	BIT(23)

/* PWR_WIO6SEMCR register fields */
#define _PWR_WIO6SEMCR_SEM_MUTEX	BIT(0)
#define _PWR_WIO6SEMCR_SEMCID_MASK	GENMASK(6, 4)
#define _PWR_WIO6SEMCR_SEMCID_SHIFT	4

/* PWR_CPUXDXSR register fields */
#define _PWR_CPUXDXSR_HOLD_BOOT		BIT(0)
#define _PWR_CPUXDXSR_WFBEN		BIT(1)
#define _PWR_CPUXDXSR_CSTATE_MASK	GENMASK(3, 2)
#define _PWR_CPUXDXSR_CSTATE_SHIFT	2
#define _PWR_CPUXDXSR_DSTATE_MASK	GENMASK(10, 8)
#define _PWR_CPUXDXSR_DSTATE_SHIFT	8

/* PWR_VERR register fields */
#define _PWR_VERR_MINREV_MASK		GENMASK(3, 0)
#define _PWR_VERR_MINREV_SHIFT		0
#define _PWR_VERR_MAJREV_MASK		GENMASK(7, 4)
#define _PWR_VERR_MAJREV_SHIFT		4


#define _PWR_FLD_PREP(field, value)	(((uint32_t)(value) << (field ## _SHIFT)) & (field ## _MASK))
#define _PWR_FLD_GET(field, value)	(((uint32_t)(value) & (field ## _MASK)) >> (field ## _SHIFT))


#define _PWR_CPU_MIN 1
#define _PWR_CPU_MAX 2

static struct stm32_pwr_platdata pdata;

__attribute__((weak))
int stm32_pwr_get_platdata(struct stm32_pwr_platdata *pdata)
{
	return 0;
}

static uint32_t _cpux_base(uint32_t cpu)
{
	uint32_t offset = _PWR_CPU1D1SR;

	if (cpu < _PWR_CPU_MIN || cpu > _PWR_CPU_MAX)
		return 0;

	offset += sizeof(uint32_t) * (cpu - _PWR_CPU_MIN);
	return pdata.base + offset;
}

static int _cpu_state(uint32_t cpu, uint32_t *state)
{
	uint32_t cpux_base;

	cpux_base = _cpux_base(cpu);
	if (!cpux_base) {
		IMSG("cpu:%d not valid");
		return -1;
	}

	*state = mmio_read_32(cpux_base);
	return 0;
}

enum c_state stm32_pwr_cpu_get_cstate(uint32_t cpu)
{
	uint32_t state;

	if (_cpu_state(cpu, &state))
		return CERR;

	return _PWR_FLD_GET(_PWR_CPUXDXSR_CSTATE, state);
}

enum d_state stm32_pwr_cpu_get_dstate(uint32_t cpu)
{
	uint32_t state;

	if (_cpu_state(cpu, &state))
		return DERR;

	return _PWR_FLD_GET(_PWR_CPUXDXSR_DSTATE, state);
}

void stm32_pwr_backupd_wp(bool enable)
{
	if (!enable) {
		mmio_setbits_32(pdata.base + _PWR_BDCR1, _PWR_BDCR1_DBD3P);
		while ((mmio_read_32(pdata.base + _PWR_BDCR1) & _PWR_BDCR1_DBD3P) == 0U) {
			;
		}
	}
}

int stm32_pwr_init(void)
{
	int err;

	err = stm32_pwr_get_platdata(&pdata);
	if (err)
		return err;

	return 0;
}
