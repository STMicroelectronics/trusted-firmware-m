#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# preload.cmake is used to set things that related to the platform that are both
# immutable and global, which is to say they should apply to any kind of project
# that uses this platform. In practise this is normally compiler definitions and
# variables related to hardware.

# Set architecture and CPU
set(TFM_SYSTEM_PROCESSOR cortex-m33)
set(TFM_SYSTEM_ARCHITECTURE armv8-m.main)


add_compile_definitions(
	STM32MP257Cxx
	CORE_CM33
	TFM_ENV
)

# set platform directory
set(STM_BOARD_DIR ${CMAKE_CURRENT_LIST_DIR})
set(STM_DIR ${STM_BOARD_DIR}/..)
set(STM_COMMON_DIR ${STM_DIR}/common)
set(STM_SOC_DIR ${STM_COMMON_DIR}/stm32mp2)
set(STM_DEVICETREE_DIR ${STM_COMMON_DIR}/devicetree)
