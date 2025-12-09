#-------------------------------------------------------------------------------
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

########################## Dependencies ################################
set(MBEDCRYPTO_BUILD_TYPE               minsizerel		CACHE STRING    "Build type of Mbed Crypto library")

########################## STM32 #######################################
### stm32 flag
set(STM32_BOARD_MODEL           "stm32mp2"	            CACHE STRING	"Define board model name")
set(STM32_IPC                   OFF                     CACHE BOOL      "Use IPC (rpmsg) to communicate with main processor")
set(STM32_M33TDCID              OFF                     CACHE BOOL      "Define M33 like Trusted Domain Compartiment ID")
set(STM32_DTS_DIR               "arm/stm"               CACHE STRING	"Define relative path of stm32 dts")
set(STM32_CACHE_ENABLED         ON                      CACHE BOOL      "Enable cache")
set(STM32_HEADER_MAJOR_VER      2                       CACHE STRING    "Define stm32 header major version: 2")
set(STM32_HEADER_MINOR_VER      2                       CACHE STRING    "Define stm32 header minor version: 0,2")
set(DTS_EXT_DIR                 ""                      CACHE STRING	"Define external dts directory")

#debug
set(MCUBOOT_LOG_LEVEL                   INFO            CACHE STRING    "Set mcuboot log level (OFF ERROR WARNING INFO DEBUG)" FORCE)
set(LOG_LEVEL                           LOG_LEVEL_INFO  CACHE STRING    "Set tfm priv log level (see tfm_vprintf.h)" FORCE)
set(LOG_LEVEL_UNPRIV                    LOG_LEVEL_INFO  CACHE STRING    "Set tfm unpriv log level (see tfm_vprintf.h)" FORCE)
set(CONFIG_TFM_BACKTRACE_ON_CORE_PANIC  ON              CACHE BOOL      "On fatal errors in secure firmware, log backtrace and then halt" FORCE)
set(TFM_EXCEPTION_INFO_DUMP             ON              CACHE BOOL      "On fatal errors in the secure firmware, capture info about the exception. Print the info if the SPM log level is sufficient." FORCE)
set(TFM_EXCEPTION_DUMP_LVL              SPM_LOG_LEVEL_INFO CACHE STRING "Set default exception dump log level as SPM_LOG_LEVEL_DEBUG level." FORCE)
set(TFM_PARTITION_LOG_LEVEL             TFM_PARTITION_LOG_LEVEL_INFO CACHE STRING "Set debug SP log level as Debug level" FORCE)
set(TFM_SPM_LOG_LEVEL                   TFM_SPM_LOG_LEVEL_INFO       CACHE STRING "Set default SPM log level as INFO level" FORCE)

## platform
set(PLATFORM_DEFAULT_ATTEST_HAL         OFF         CACHE BOOL  "Use default attest hal implementation.")
set(PLATFORM_DEFAULT_UART_STDOUT        OFF         CACHE BOOL  "Use default uart stdout implementation.")
set(CONFIG_TFM_USE_TRUSTZONE            ON          CACHE BOOL  "Enable use of TrustZone to transition between NSPE and SPE")
set(PLATFORM_DEFAULT_NV_COUNTERS        OFF         CACHE BOOL  "Use default nv counter implementation.")
set(PLATFORM_DEFAULT_OTP_WRITEABLE      OFF         CACHE BOOL  "Use on chip flash with write support")
set(PLATFORM_DEFAULT_OTP                OFF         CACHE BOOL  "Use trusted on-chip flash to implement OTP memory")
set(PLATFORM_DEFAULT_PROVISIONING       OFF         CACHE BOOL  "Use default provisioning implementation")
set(TFM_DUMMY_PROVISIONING              ON          CACHE BOOL  "Provision with dummy values. NOT to be used in production")
set(STM32_OVERRIDE_OTP                  OFF         CACHE BOOL  "Override the OTP key with dummy values if TFM_DUMMY_PROVISIONING is ON")
set(TFM_PARTITION_PLATFORM              ON          CACHE BOOL  "Enable the TF-M Platform partition")
set(PLATFORM_DEFAULT_CRYPTO_KEYS        OFF         CACHE BOOL  "Use default crypto keys implementation.")
set(CRYPTO_HW_ACCELERATOR               ON          CACHE BOOL  "Whether to enable the crypto hardware accelerator on supported platforms")
set(STM32_PROFILE_DEFINITION		"STM32_PROFILE_MEDIUM"	CACHE STRING	"The default profile definition of platform")
set(PLATFORM_DEFAULT_SYSTEM_RESET_HALT  OFF         CACHE BOOL  "Use default system reset/halt implementation")

if (STM32_M33TDCID)
	set(BL2                               ON                   CACHE BOOL   "Whether to build BL2" FORCE)
	set(MCUBOOT_UPGRADE_STRATEGY          "RAM_LOAD"           CACHE STRING "Upgrade strategy when multiple boot images are loaded [OVERWRITE_ONLY, SWAP, DIRECT_XIP, RAM_LOAD]" FORCE)
	if (EXISTS ${STM32_CA35_FW})
		set(MCUBOOT_IMAGE_NUMBER              3                    CACHE STRING "Number of images to be handled by MCUBoot" FORCE)
	else()
		set(MCUBOOT_IMAGE_NUMBER              2                    CACHE STRING "Number of images to be handled by MCUBoot" FORCE)
	endif()
	set(TFM_NS_INDEPENDENT_SIG            OFF                  CACHE BOOL   "Indicate if S and NS must be signed independently or not" FORCE)
	set(STM32_BOOT_DEV                    "ospi"               CACHE STRING "Set boot device [ddr, ospi, sdmmc1, sdmmc2]")
	set(BL2_HEADER_SIZE                   0x800                CACHE STRING "Header size to aligned vector table")
	set(STM32_DDR_PHY_FILE                "ddr4_pmu_train.bin" CACHE STRING "Set ddr phy binary name need for your board")
	set(STM32_DDR_TYPE                    STM32MP_DDR4_TYPE    CACHE STRING "Set ddr type flag")
	set(STM32_DDR_SIZE                    0x100000000          CACHE STRING "Set ddr size in Bytes")
	set(STM32_DDR_FREQ                    1200000              CACHE STRING "Set ddr frequency in KHz")
	set(TFM_MULTI_CORE_TOPOLOGY           ON                   CACHE BOOL   "Whether to build for a dual-cpu architecture")
	set(TFM_PARTITION_SCP                 ON                   CACHE BOOL   "Use System Control Processor library in partition")
	set(TFM_PARTITION_IPCC                ON                   CACHE BOOL   "Use IPCC partition")
	set(TFM_PARTITION_NS_AGENT_MAILBOX    ON                   CACHE BOOL   "Enable Non-Secure Mailbox Agent partition")
	set(TFM_PARTITION_PM                  ON                   CACHE BOOL   "Enable Power Management partition")
	set(TFM_PARTITION_STL                 OFF                  CACHE BOOL   "Enable STL Partition for Safety NS service")
	set(TFM_PLAT_SPECIFIC_MULTI_CORE_COMM ON                   CACHE BOOL   "Whether to use a platform specific inter-core communication instead of mailbox in dual-cpu topology")
	set(CONFIG_TFM_ARM_RSE_COMMS_PLAT_HAL ON                   CACHE BOOL   "Whether to use a platform specific hal for platform/ext/arm/rse/common/rse_comms")
	set(DDR_IMAGE_VERSION                 "0.1.0"              CACHE STRING "The version of DDR Firmware to avoid rollback")
	set(DDRFW_SECURITY_COUNTER_S          "1"                  CACHE STRING "Security counter for ddr-fw.")
	set(MUCBOOT_KEY_DDRFW		      ""                   CACHE FILEPATH "Path to key with which to sign ddr firwmare. if not set MCUBOOT_KEY_NS is used.")
	set(STM32_CA35_FW                     ""                   CACHE STRING	"Path to CA35 binary file")
	set(MUCBOOT_KEY_A35FW		      ""                   CACHE FILEPATH "Path to key with which to sign cortex A35 firwmare. if not set MCUBOOT_KEY_S is used.")
	set(CA35FW_SECURITY_COUNTER_S         "1"                  CACHE STRING "Security counter for CA35 firmware.")
	set(CA35FW_IMAGE_VERSION              "1.0"                CACHE STRING "The version of CA35 firmware to avoid rollback")
	set(DEFAULT_MCUBOOT_FLASH_MAP         OFF                  CACHE BOOL   "Whether to use the default flash map defined by TF-M project" FORCE)
	set(MCUBOOT_SIGNATURE_TYPE            "EC-P256"            CACHE STRING "Algorithm to use for signature validation [RSA-2048, RSA-3072, EC-P256, EC-P384]" FORCE)
	set(MCUBOOT_USE_PSA_CRYPTO            ON                   CACHE BOOL   "Enable the cryptographic abstraction layer to use PSA Crypto APIs")
	set(PLATFORM_HAS_NS_NOTIF             ON                   CACHE BOOL   "Plaform implements hal for enabling the ns notifcation from the secure")
	set(PLATFORM_HAS_BOOTDATA             ON                   CACHE BOOL   "Use a platform bootdata section in MCUBOOT_DATA_SHARING")
	set(IPCC_LEGACY                       OFF                  CACHE BOOL   "Use IPCC implementation without NS Notification")
	set(TFM_PLATFORM_CPU_API              ON                   CACHE BOOL   "Enable platform cpu support")
	set(STM32_HW_KEYSHARE                 ON                   CACHE BOOL   "Enable SAES and CRYP key sharing")
else()
	set(BL2                              OFF                   CACHE BOOL   "Whether to build BL2" FORCE)
	set(TFM_MULTI_CORE_TOPOLOGY          OFF                   CACHE BOOL   "Whether to build for a dual-cpu architecture")
	set(TFM_PARTITION_SCP                OFF                   CACHE BOOL   "Use System Control Processor library in partition")
	set(TFM_PARTITION_IPCC               OFF                   CACHE BOOL   "Use IPCC partition")
endif()


if (TFM_PARTITION_SCP)
	list(APPEND MANIFEST_LISTS ${CMAKE_CURRENT_LIST_DIR}/manifest/tfm_manifest_list.yaml)
endif()

if (TFM_PARTITION_PM)
	set(CONFIG_PM_DEVICE                 ON                    CACHE BOOL   "Enable power device support")
endif()

if (TFM_PARTITION_PLATFORM)
	set(TFM_PLATFORM_WDT_API             ON                    CACHE BOOL   "Enable platform watchdog support")
endif()

set(CFG_SCPFW_MOD_POWER_DOMAIN          ON                      CACHE BOOL      "Set ON to enable power domain modules")
set(CFG_SCPFW_MOD_SCMI_SYSTEM_POWER     ON                      CACHE BOOL      "Set ON to enable scmi system power domain modules")
set(PLATFORM_SLIH_IRQ_TEST_SUPPORT      OFF                     CACHE BOOL      "Platform supports SLIH IRQ tests")
set(PLATFORM_FLIH_IRQ_TEST_SUPPORT      OFF                     CACHE BOOL      "Platform supports FLIH IRQ tests")

## DT
if (STM32_M33TDCID)
	string(APPEND DTS_BOARD_BASE "-cm33tdcid")
endif()

set(DTS_BOARD_BL2	"${DTS_BOARD_BASE}-bl2.dts"     CACHE STRING "set bl2 board devicetree file")
set(DTS_BOARD_S		"${DTS_BOARD_BASE}-s.dts"       CACHE STRING "set s board devicetree file")
set(DTS_BOARD_NS	"${DTS_BOARD_BASE}-ns.dts"      CACHE STRING "set ns board devicetree file")

