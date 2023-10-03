#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# set common platform config
if (EXISTS ${STM_SOC_DIR}/config.cmake)
	include(${STM_SOC_DIR}/config.cmake)
endif()

########################## Dependencies ################################
set(MBEDCRYPTO_BUILD_TYPE               minsizerel		CACHE STRING    "Build type of Mbed Crypto library")

# set board specific config
########################## STM32 #######################################

set(STM32_BOARD_MODEL	"stm32mp257f eval1"			CACHE STRING	  "Define board model name" FORCE)
set(DTS_EXT_DIR		""					CACHE STRING	  "If not empty, set external dts directory")
set(DTS_BOARD_S		"arm/stm/stm32mp257f-ev1-revB_s.dts"	CACHE STRING	  "set board devicetree file")
set(DTS_BOARD_NS	"arm/stm/stm32mp257f-ev1-revB_ns.dts"	CACHE STRING	  "set board devicetree file")

set(STM32_IPC				ON         CACHE BOOL     "Use IPC (rpmsg) to communicate with main processor" FORCE)
set(BL2                                 OFF        CACHE BOOL     "Whether to build BL2" FORCE)
set(TFM_DUMMY_PROVISIONING              ON         CACHE BOOL     "Provision with dummy values. NOT to be used in production" FORCE)
set(STM32_DDR_CACHED			OFF        CACHE BOOL     "Enable cache for ddr" FORCE)
set(STM32_PROV_FAKE			ON         CACHE BOOL     "Provisioning with dummy values. NOT to be used in production" FORCE)
