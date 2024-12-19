#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2021 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# cpuarch.cmake is used to set things that related to the cpu architecture that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practise this is normally compiler definitions and
# variables related to hardware.

# set platform directory
set(STM_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR})
set(STM_DIR ${STM_BOARD_DIR}/..)

include(${STM_DIR}/common/stm32mp2xx/stm32mp25/cpuarch.cmake)

add_compile_definitions(
	STM32MP257Cxx
)
