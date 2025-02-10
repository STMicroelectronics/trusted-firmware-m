#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# set familly platform config
if (EXISTS ${STM_FAMILLY_DIR}/config.cmake)
    include(${STM_FAMILLY_DIR}/config.cmake)
endif()

# set specific stm32mp21 config
########################## STM32 #######################################
set(STM32_BOARD_MODEL           "stm32mp21xxxx" CACHE STRING    "Define board model name" FORCE)

set(STM32_IPC                   ON              CACHE BOOL      "Use IPC (rpmsg) to communicate with main processor" FORCE)
set(TFM_DUMMY_PROVISIONING      ON              CACHE BOOL      "Provision with dummy values. NOT to be used in production" FORCE)
set(STM32_PROV_FAKE             ON              CACHE BOOL      "Provisioning with dummy values. NOT to be used in production" FORCE)
set(STM32_HEADER_MAJOR_VER      2               CACHE STRING    "Define stm32 header major version: 2" FORCE)
set(STM32_HEADER_MINOR_VER      3               CACHE STRING    "Define stm32 header minor version: 0,3" FORCE)
