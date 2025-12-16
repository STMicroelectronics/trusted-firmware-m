#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(STM32MP2_LP_GIT_REMOTE  "https://github.com/STMicroelectronics/stm32mp2-low-power" CACHE STRING "Git remote to fetch the low power firmware from")
set(STM32MP2_LP_VERSION     "v1.0.0"                  CACHE STRING "The version of stm32mp2 low power firmware to use")
set(STM32MP2_LP_PATH        "DOWNLOAD"                CACHE PATH   "Path to stm32mp2 low power firmware (or DOWNLOAD to fetch automatically)")

#set stm32mp2 low power config
set(STM32MP2_LP_TARGET      stm32mp2_lp_fw            CACHE STRING "target name of stm32mp2 low power" FORCE)
set(STM32MP2_LP_OUT_DIR     ${CMAKE_BINARY_DIR}/bin   CACHE PATH   "stm32mp2 low power output dir" FORCE)
set(STM32MP2_LP_OUT_BIN     ${STM32MP2_LP_TARGET}.bin CACHE PATH   "stm32mp2 low power output binary" FORCE)

fetch_remote_library(
    LIB_NAME                stm32mp2_lp_fw
    LIB_SOURCE_PATH_VAR     STM32MP2_LP_PATH
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ${STM32MP2_LP_GIT_REMOTE}
        GIT_TAG             ${STM32MP2_LP_VERSION}
        GIT_PROGRESS        TRUE
)

add_subdirectory(${STM32MP2_LP_PATH} stm32mp2_lp_fw)
