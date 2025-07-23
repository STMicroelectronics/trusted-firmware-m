#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

#def for stm32mp2
include(${STM_FAMILY_DIR}/ns/CMakeLists.txt)

target_sources(platform_ns
    PRIVATE
        ${STM_FAMILY_DIR}/native_driver/src/clk/clk-stm32mp21.c
)

target_sources(tfm_ns
    PRIVATE
        ${STM_FAMILY_DIR}/native_driver/src/clk/clk-stm32mp21.c
)
