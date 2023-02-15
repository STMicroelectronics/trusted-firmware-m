#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# set common platform config
if (EXISTS ${CMAKE_SOURCE_DIR}/platform/ext/target/stm/common/stm32mp2/config.cmake)
	include(${CMAKE_SOURCE_DIR}/platform/ext/target/stm/common/stm32mp2/config.cmake)
endif()

########################## Dependencies ################################
set(MBEDCRYPTO_BUILD_TYPE               minsizerel		CACHE STRING    "Build type of Mbed Crypto library")

# set board specific config
########################## STM32 #######################################
if (STM32_M33TDCID)
	message(FATAL_ERROR "INVALID CONFIG: STM32_M33TDCID NOT SUPPORTED ON ${TFM_PLATFORM}")
endif()

set(STM32_BOARD_MODEL			"stm32mp257f eval"	CACHE STRING	"Define board model name" FORCE)

set(STM32_IPC				ON         CACHE BOOL     "Use IPC (rpmsg) to communicate with main processor" FORCE)
set(BL2                                 OFF        CACHE BOOL     "Whether to build BL2" FORCE)
set(PLATFORM_DEFAULT_NV_COUNTERS        ON         CACHE BOOL     "Use default nv counter implementation." FORCE)
set(OTP_NV_COUNTERS_RAM_EMULATION       ON         CACHE BOOL     "Enable OTP/NV_COUNTERS emulation in RAM. Has no effect on non-default implementations of the OTP and NV_COUNTERS" FORCE)
set(TFM_DUMMY_PROVISIONING              ON         CACHE BOOL     "Provision with dummy values. NOT to be used in production" FORCE)
set(PLATFORM_DEFAULT_OTP_WRITEABLE      ON         CACHE BOOL     "Use on chip flash with write support" FORCE)
set(STM32_DDR_CACHED			OFF        CACHE BOOL     "Enable cache for ddr" FORCE)
