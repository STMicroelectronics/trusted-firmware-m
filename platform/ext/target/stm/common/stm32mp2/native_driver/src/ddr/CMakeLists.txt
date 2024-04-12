#-------------------------------------------------------------------------------
# Copyright (c) 2020, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)

set(STM_DDR_DIR ${CMAKE_CURRENT_LIST_DIR})

fetch_remote_library(
    LIB_NAME                ddr_phy_bin
    LIB_SOURCE_PATH_VAR     DDR_PHY_BIN_SRC_PATH
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      https://github.com/STMicroelectronics/stm32-ddr-phy-binary
	GIT_TAG             ${DDR_PHY_BIN_VERSION}
        GIT_SHALLOW         FALSE
        GIT_PROGRESS        TRUE
	GIT_SUBMODULES      ""
)

target_include_directories(platform_bl2
	PUBLIC
		${STM_DDR_DIR}/include
		${STM_DDR_DIR}/phy/firmware/include
		${STM_DDR_DIR}/phy/phyinit/include
)

target_sources(platform_bl2
	PRIVATE
		${STM_DDR_DIR}/stm32mp2_ddr.c
		${STM_DDR_DIR}/stm32mp_ddr.c
		${STM_DDR_DIR}/stm32mp2_ddr_helpers.c
		${STM_DDR_DIR}/stm32mp2_ram.c
		${STM_DDR_DIR}/stm32mp_ddr_test.c

		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_c_initphyconfig.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_calcmb.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_globals.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_i_loadpieimage.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_initstruct.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_isdbytedisabled.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_loadpieprodcode.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_mapdrvstren.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_progcsrskiptrain.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_reginterface.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_restore_sequence.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_sequence.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_softsetmb.c
		${STM_DDR_DIR}/phy/phyinit/usercustom/ddrphy_phyinit_usercustom_customposttrain.c
		${STM_DDR_DIR}/phy/phyinit/usercustom/ddrphy_phyinit_usercustom_custompretrain.c
		${STM_DDR_DIR}/phy/phyinit/usercustom/ddrphy_phyinit_usercustom_saveretregs.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_d_loadimem.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_f_loaddmem.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_g_execfw.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_h_readmsgblock.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_storemsgblk.c
		${STM_DDR_DIR}/phy/phyinit/src/ddrphy_phyinit_writeoutmem.c
		${STM_DDR_DIR}/phy/phyinit/usercustom/ddrphy_phyinit_usercustom_h_readmsgblock.c
		${STM_DDR_DIR}/phy/phyinit/usercustom/ddrphy_phyinit_usercustom_g_waitfwdone.c
)

target_compile_definitions(platform_bl2
	PUBLIC
		$<$<STREQUAL:${STM32_DDR_PHY_FILE},ddr4_pmu_train.bin>:STM32MP_DDR4_TYPE=1>
)

install(PROGRAMS
	${DDR_PHY_BIN_SRC_PATH}/stm32mp2/${STM32_DDR_PHY_FILE}
	DESTINATION $<TARGET_FILE_DIR:bl2>
	RENAME ddr_phy.bin
)
