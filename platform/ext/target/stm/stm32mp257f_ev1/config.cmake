#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# set board specific config
########################## STM32 #######################################
#Before Soc config
set(STM32_STM32MP257f_EV1_REV   "revA"				    CACHE STRING	"Select ev1 board revision: revA" FORCE)

SET(DTS_BOARD_BASE "arm/stm/stm32mp257f-ev1")

if (NOT STM32_STM32MP257f_EV1_REV STREQUAL "revA")
	message(FATAL_ERROR "Board revision not defined by st")
	string(APPEND DTS_BOARD_BASE "-${STM32_STM32MP257f_EV1_REV}")
endif()

# set common soc config
if (EXISTS ${STM_SOC_DIR}/config.cmake)
	include(${STM_SOC_DIR}/config.cmake)
endif()

#After soc config
set(STM32_BOARD_MODEL           "stm32mp257f eval1"		CACHE STRING	"Define board model name" FORCE)
