#-------------------------------------------------------------------------------
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
set(TFM_EXTRA_GENERATED_FILE_LIST_PATH  ${CMAKE_SOURCE_DIR}/platform/ext/target/stm/common/stm32mp2/generated_file_list.yaml  CACHE PATH "Path to extra generated file list. Appended to stardard TFM generated file list." FORCE)

set(STM32_BOARD_MODEL			"stm32mp257 common"	CACHE STRING	"Define board model name")
set(STM32_LOG_LEVEL		        STM32_LOG_LEVEL_INFO    CACHE STRING    "Set default stm32 log level as NOTICE, (see debug.h)")
set(STM32_IPC				OFF			CACHE BOOL      "Use IPC (rpmsg) to communicate with main processor")

set(CONFIG_TFM_USE_TRUSTZONE            ON			CACHE BOOL      "Enable use of TrustZone to transition between NSPE and SPE")
set(TFM_MULTI_CORE_TOPOLOGY             OFF			CACHE BOOL      "Whether to build for a dual-cpu architecture")
set(PLATFORM_DEFAULT_NV_COUNTERS        OFF                     CACHE BOOL      "Use default nv counter implementation.")
set(PLATFORM_DEFAULT_OTP_WRITEABLE      OFF                     CACHE BOOL      "Use on chip flash with write support")
set(TFM_DUMMY_PROVISIONING              OFF                     CACHE BOOL      "Provision with dummy values. NOT to be used in production")
