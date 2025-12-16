#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
project(stm32mp2xx_hal_driver VERSION 1.0 LANGUAGES CXX)

if (TARGET stm32mp2xx_hal_driver_header)
    return()
endif()

set(STM32MP2XX_HAL_DRIVER_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(stm32mp2xx_hal_driver_header INTERFACE)

target_include_directories(stm32mp2xx_hal_driver_header
    INTERFACE
	$<BUILD_INTERFACE:${STM32MP2XX_HAL_DRIVER_PATH}/Inc>
)

add_library(stm32mp2xx_hal_driver::header ALIAS stm32mp2xx_hal_driver_header)
