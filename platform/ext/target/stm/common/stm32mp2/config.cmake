#-------------------------------------------------------------------------------
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
set(TFM_EXTRA_GENERATED_FILE_LIST_PATH  ${STM_SOC_DIR}/generated_file_list.yaml  CACHE PATH "Path to extra generated file list. Appended to stardard TFM generated file list." FORCE)

### stm32 flag
set(STM32_BOARD_MODEL			"stm32mp257 common"	CACHE STRING	"Define board model name")
set(STM32_LOG_LEVEL		        STM32_LOG_LEVEL_INFO    CACHE STRING    "Set default stm32 log level as NOTICE, (see debug.h)")
set(STM32_IPC				OFF			CACHE BOOL      "Use IPC (rpmsg) to communicate with main processor")
set(STM32_PROV_FAKE			OFF                     CACHE BOOL      "Provisioning with dummy values. NOT to be used in production")

## platform
set(PLATFORM_DEFAULT_UART_STDOUT        OFF			CACHE BOOL      "Use default uart stdout implementation.")
set(CONFIG_TFM_USE_TRUSTZONE            ON			CACHE BOOL      "Enable use of TrustZone to transition between NSPE and SPE")
set(TFM_MULTI_CORE_TOPOLOGY             OFF			CACHE BOOL      "Whether to build for a dual-cpu architecture")
set(PLATFORM_DEFAULT_NV_COUNTERS        OFF                     CACHE BOOL      "Use default nv counter implementation.")
set(PLATFORM_DEFAULT_OTP_WRITEABLE      OFF                     CACHE BOOL      "Use on chip flash with write support")
set(PLATFORM_DEFAULT_OTP                OFF                     CACHE BOOL      "Use trusted on-chip flash to implement OTP memory")
set(PLATFORM_DEFAULT_PROVISIONING       OFF                     CACHE BOOL      "Use default provisioning implementation")
set(TFM_DUMMY_PROVISIONING              OFF                     CACHE BOOL      "Provision with dummy values. NOT to be used in production")
