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
set(STM32_STM32MP2_SOC_REV		"revA"			CACHE STRING	"Set soc revision: revA")
set(STM32_M33TDCID			OFF			CACHE BOOL	"Define M33 like Trusted Domain Compartiment ID")

## platform
set(PLATFORM_DEFAULT_UART_STDOUT        OFF			CACHE BOOL      "Use default uart stdout implementation.")
set(CONFIG_TFM_USE_TRUSTZONE            ON			CACHE BOOL      "Enable use of TrustZone to transition between NSPE and SPE")
set(TFM_MULTI_CORE_TOPOLOGY             OFF			CACHE BOOL      "Whether to build for a dual-cpu architecture")
set(PLATFORM_DEFAULT_NV_COUNTERS        OFF                     CACHE BOOL      "Use default nv counter implementation.")
set(PLATFORM_DEFAULT_OTP_WRITEABLE      OFF                     CACHE BOOL      "Use on chip flash with write support")
set(PLATFORM_DEFAULT_OTP                OFF                     CACHE BOOL      "Use trusted on-chip flash to implement OTP memory")
set(PLATFORM_DEFAULT_PROVISIONING       OFF                     CACHE BOOL      "Use default provisioning implementation")
set(TFM_DUMMY_PROVISIONING              OFF                     CACHE BOOL      "Provision with dummy values. NOT to be used in production")

if (STM32_M33TDCID)
	set(BL2                         ON                      CACHE BOOL     "Whether to build BL2" FORCE)
        set(MCUBOOT_UPGRADE_STRATEGY    "RAM_LOAD"              CACHE STRING   "Upgrade strategy when multiple boot images are loaded [OVERWRITE_ONLY, SWAP, DIRECT_XIP, RAM_LOAD]" FORCE)
	set(MCUBOOT_IMAGE_NUMBER        1                       CACHE STRING   "Whether to combine S and NS into either 1 image, or sign each seperately" FORCE)
	set(STM32_BOOT_DEV		"ospi"	                CACHE STRING   "Set boot device: ddr or ospi")
	set(BL2_HEADER_SIZE		0x800			CACHE STRING   "Header size to aligned vector table")
	set(STM32_DDR_TYPE              "ddr4"                  CACHE STRING    "Set ddr type on your board: ddr3, ddr4, lpddr4")
else()
	set(BL2                         OFF                     CACHE BOOL     "Whether to build BL2" FORCE)
endif()

if (STM32_STM32MP2_SOC_REV STREQUAL "revA")
	set(STM32_HEADER_MAJOR_VER 2)
	set(STM32_HEADER_MINOR_VER 0)
else()
	message(FATAL_ERROR "SoC Revision not supported")
endif()
