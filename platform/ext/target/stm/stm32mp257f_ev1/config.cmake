#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

########################## Dependencies ################################
set(MBEDCRYPTO_BUILD_TYPE               minsizerel		CACHE STRING    "Build type of Mbed Crypto library")

# set board specific config
########################## STM32 #######################################
#Before Soc config
set(STM32_BOARD_MODEL		"stm32mp257f eval1"		CACHE STRING	"Define board model name" FORCE)
set(STM32_STM32MP257f_EV1_REV	"revA"				CACHE STRING	"Select ev1 board revision: revA")
set(STM32_STM32MP2_SOC_REV	"revB"				CACHE STRING	"Set soc revision: revA, revB" FORCE)

SET(DTS_BOARD_BASE "arm/stm/stm32mp257f-ev1")

if (NOT STM32_STM32MP257f_EV1_REV STREQUAL "revA")
	message(FATAL_ERROR "Board revision not defined by st")
	string(APPEND DTS_BOARD_BASE "-${STM32_STM32MP257f_EV1_REV}")
endif()

if (STM32_M33TDCID)
	string(APPEND DTS_BOARD_BASE "-cm33tdcid")
endif()

set(DTS_BOARD_BL2	"${DTS_BOARD_BASE}-bl2.dts"		CACHE STRING	"set bl2 board devicetree file")
set(DTS_BOARD_S		"${DTS_BOARD_BASE}-s.dts"	        CACHE STRING	"set s board devicetree file")
set(DTS_BOARD_NS	"${DTS_BOARD_BASE}-ns.dts"	        CACHE STRING	"set ns board devicetree file")

# set common platform config
if (EXISTS ${STM_SOC_DIR}/config.cmake)
	include(${STM_SOC_DIR}/config.cmake)
endif()

# set specific borad config
set(STM32_IPC				ON         CACHE BOOL     "Use IPC (rpmsg) to communicate with main processor" FORCE)
set(TFM_DUMMY_PROVISIONING              ON         CACHE BOOL     "Provision with dummy values. NOT to be used in production" FORCE)
set(STM32_DDR_CACHED			OFF        CACHE BOOL     "Enable cache for ddr" FORCE)
set(STM32_PROV_FAKE			ON         CACHE BOOL     "Provisioning with dummy values. NOT to be used in production" FORCE)
