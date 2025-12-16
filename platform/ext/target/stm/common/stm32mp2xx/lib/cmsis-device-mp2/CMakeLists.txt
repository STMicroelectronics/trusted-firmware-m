#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
project(cmsis_device_mp2 VERSION 1.0 LANGUAGES CXX)

if (TARGET cmsis_device_mp2_header)
    return()
endif()

set(CMSIS_DEVICE_MP2_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(cmsis_device_mp2_header INTERFACE)

target_include_directories(cmsis_device_mp2_header
    INTERFACE
	$<BUILD_INTERFACE:${CMSIS_DEVICE_MP2_PATH}/Include>
)

add_library(cmsis_device_mp2::header ALIAS cmsis_device_mp2_header)
