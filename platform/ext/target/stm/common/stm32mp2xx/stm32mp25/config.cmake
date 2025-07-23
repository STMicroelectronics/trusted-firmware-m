#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2024 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# set family platform config
if (EXISTS ${STM_FAMILY_DIR}/config.cmake)
	include(${STM_FAMILY_DIR}/config.cmake)
endif()

# set specific stm32mp25 config
########################## STM32 #######################################
set(STM32_BOARD_MODEL           "stm32mp25xxxx" CACHE STRING   "Define board model name" FORCE)
set(STM32_STM32MP25_SOC_REV     "revY"          CACHE STRING   "Set soc revision: revY, revX")

# set specific board config
set(STM32_IPC                   ON              CACHE BOOL     "Use IPC (rpmsg) to communicate with main processor" FORCE)
set(STM32_PROV_FAKE             ON              CACHE BOOL     "Provisioning with dummy values. NOT to be used in production" FORCE)

set(STM32_HEADER_MAJOR_VER      2               CACHE STRING   "Define stm32 header major version: 2" FORCE)
set(STM32_HEADER_MINOR_VER      2               CACHE STRING   "Define stm32 header minor version: 0,2" FORCE)
