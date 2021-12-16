#-------------------------------------------------------------------------------
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(STM32_LOG_LEVEL		        STM32_LOG_LEVEL_INFO    CACHE STRING    "Set default stm32 log level as NOTICE, (see debug.h)")

set(PLATFORM_DEFAULT_NV_COUNTERS        OFF                     CACHE BOOL      "Use default nv counter implementation.")
set(PLATFORM_DEFAULT_OTP_WRITEABLE      OFF                     CACHE BOOL      "Use on chip flash with write support")
set(TFM_DUMMY_PROVISIONING              OFF                     CACHE BOOL      "Provision with dummy values. NOT to be used in production")
