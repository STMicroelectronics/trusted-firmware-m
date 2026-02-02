#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# set family platform config
if (EXISTS ${STM_FAMILY_DIR}/config.cmake)
    include(${STM_FAMILY_DIR}/config.cmake)
endif()

# set specific stm32mp21 config
########################## STM32 #######################################
set(STM32_BOARD_MODEL           "stm32mp21xxxx" CACHE STRING    "Define board model name" FORCE)
set(STM32_STM32MP21_SOC_REV     "revA"          CACHE STRING    "Set soc revision: revA, revZ")

set(STM32_IPC                   ON              CACHE BOOL      "Use IPC (rpmsg) to communicate with main processor" FORCE)
set(STM32_PROV_FAKE             ON              CACHE BOOL      "Provisioning with dummy values. NOT to be used in production" FORCE)
set(STM32_HEADER_MAJOR_VER      2               CACHE STRING    "Define stm32 header major version: 2" FORCE)
set(STM32_HEADER_MINOR_VER      3               CACHE STRING    "Define stm32 header minor version: 0,3" FORCE)

set(PLATFORM_PSA_ADAC_GIT_REMOTE "https://github.com/STMicroelectronics/psa-adac" CACHE STRING "The URL (or path) to retrieve psa-adac from.")
set(PLATFORM_PSA_ADAC_VERSION "v1.0-stm32mp-r1" CACHE STRING    "The version of psa-adac to use.")
if (STM32_M33TDCID)
    set(PLATFORM_PSA_ADAC_SECURE_DEBUG TRUE     CACHE BOOL      "Whether to use psa-adac secure debug.")
    set(TFM_PARTITION_ADAC      ON              CACHE BOOL      "Enable ADAC partition")
    set(TFM_PARTITION_DBGMCU    ON              CACHE BOOL      "Enable DBGMCU partition")
    set(PSA_ADAC_TFM_PLATFORM   stm/stm32mp215f_dk CACHE STRING "Platform folder used by PSA ADAC")
endif()
