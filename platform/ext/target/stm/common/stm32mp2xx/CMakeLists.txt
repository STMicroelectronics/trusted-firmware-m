#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

# fetch cmsis-core for soc header files
include(${STM_FAMILY_DIR}/lib/cmsis-core/cmsis_core.cmake)

# fetch cmsis-device-mp2 for STM32MP2 device header files
include(${STM_FAMILY_DIR}/lib/cmsis-device-mp2/cmsis_device_mp2.cmake)

# fetch stm32mp2xx-hal-driver for HAL drivers header files
include(${STM_FAMILY_DIR}/lib/stm32mp2xx-hal-driver/stm32mp2xx_hal_driver.cmake)

#========================= Device tree ========================================#
include(${STM_DEVICETREE_DIR}/gen_dt.cmake)

#========================= Platform region defs ===============================#
# these flags are includes in bl2, tfm_s and tfm_ns
target_compile_definitions(platform_region_defs
	INTERFACE
		$<$<BOOL:${STM32_IPC}>:STM32_IPC>
		$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:STM32_BOOT_DEV_OSPI>
		$<$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>:STM32_BOOT_DEV_SDMMC1>
		$<$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>:STM32_BOOT_DEV_SDMMC2>
		$<$<STREQUAL:${STM32_BOOT_DEV},ddr>:STM32_BOOT_DEV_DDR>
		$<$<BOOL:${STM32_M33TDCID}>:STM32_M33TDCID>
		# this flag is not defined in tfm_s and needed by stm32_bsec3
		$<$<BOOL:${TFM_DUMMY_PROVISIONING}>:TFM_DUMMY_PROVISIONING>
		$<$<BOOL:${TFM_PARTITION_SCP}>:S_HEAP_SIZE=0x00020000>
)

target_compile_definitions(tfm_config
	INTERFACE
        $<$<BOOL:${STM32_IPC}>:STM32_IPC>
        $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:STM32_BOOT_DEV_OSPI>
        $<$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>:STM32_BOOT_DEV_SDMMC1>
        $<$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>:STM32_BOOT_DEV_SDMMC2>
        $<$<STREQUAL:${STM32_BOOT_DEV},ddr>:STM32_BOOT_DEV_DDR>
        $<$<BOOL:${STM32_M33TDCID}>:STM32_M33TDCID>
        $<$<BOOL:${IPCC_LEGACY}>:IPCC_LEGACY>
        # this flag is not defined in tfm_s and needed by stm32_bsec3
        $<$<BOOL:${TFM_DUMMY_PROVISIONING}>:TFM_DUMMY_PROVISIONING>
	$<$<BOOL:${STM32_OVERRIDE_OTP}>:STM32_OVERRIDE_OTP>
        $<$<BOOL:${TFM_PARTITION_SCP}>:S_HEAP_SIZE=0x00020000>
        $<$<BOOL:${STM32_CACHE_ENABLED}>:STM32_CACHE_ENABLED>
        $<$<BOOL:${TFM_PLATFORM_WDT_API}>:TFM_PLATFORM_WDT_API>
        $<$<BOOL:${TFM_PLATFORM_CPU_API}>:TFM_PLATFORM_CPU_API>
        $<$<BOOL:${STM32_HW_KEYSHARE}>:STM32_HW_KEYSHARE>
)

target_include_directories(tfm_config
    INTERFACE
        $<BUILD_INTERFACE:${STM_FAMILY_DIR}>
        $<BUILD_INTERFACE:${STM_SOC_DIR}>
)

target_link_libraries(tfm_config
    INTERFACE
	$<BUILD_INTERFACE:cmsis_core::header>
	$<BUILD_INTERFACE:cmsis_device_mp2::header>
	$<BUILD_INTERFACE:stm32mp2xx_hal_driver::header>
)

target_include_directories(platform_region_defs
	INTERFACE
		include
		${STM_FAMILY_DIR}
		${STM_FAMILY_DIR}/native_driver/include
		${STM_FAMILY_DIR}/cmsis_driver/include
		${STM_FAMILY_DIR}/secure/services/include
)

#========================= Platform Secure ====================================#
target_add_scatter_file(tfm_s
	$<$<C_COMPILER_ID:GNU>:${STM_FAMILY_DIR}/linker_scripts/gcc/stm32mp2_s.ld>
)

add_devicetree_target(TARGET platform_s
	DTS_BOARD ${DTS_BOARD_S}
	DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
)

add_dependencies(tfm_s_scatter dt_platform_s_gen_h)

target_link_libraries(tfm_s_scatter
	dt_platform_s_defs
)

add_subdirectory(${STM_FAMILY_DIR}/lib/qsort qsort)

target_include_directories(platform_s
	PUBLIC
		${CMAKE_BINARY_DIR}/generated
		${STM_FAMILY_DIR}/native_driver/include
		${STM_FAMILY_DIR}/cmsis_driver/include
		${STM_FAMILY_DIR}/secure
		${STM_FAMILY_DIR}/secure/services/include
)

target_sources(platform_s
	PRIVATE
		${DEVICETREE_DIR}/initlevel.c
		${STM_FAMILY_DIR}/secure/target_cfg.c
		${STM_FAMILY_DIR}/secure/tfm_hal_isolation.c
		${STM_FAMILY_DIR}/secure/provisioning.c
		${STM_FAMILY_DIR}/secure/tfm_hal_platform.c
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/secure/tfm_hal_multi_core.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/secure/rse_comms_hal.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/secure/rse_comms_permissions_hal.c>
		${STM_FAMILY_DIR}/cmsis_driver/src/driver_spi_flash.c
		${STM_FAMILY_DIR}/cmsis_driver/src/driver_flash_mm.c
		${STM_FAMILY_DIR}/native_driver/src/core/mpu_armv8m_drv.c
		${STM_FAMILY_DIR}/native_driver/src/core/sau_armv8m_drv.c
		${STM_FAMILY_DIR}/native_driver/src/entropy/entropy.c
		${STM_FAMILY_DIR}/native_driver/src/entropy/stm32_rng.c
		${STM_FAMILY_DIR}/native_driver/src/watchdog/watchdog.c
		${STM_FAMILY_DIR}/native_driver/src/watchdog/stm32_iwdg.c
		${STM_FAMILY_DIR}/native_driver/src/bsec/stm32_bsec3.c
		${STM_FAMILY_DIR}/native_driver/src/nvmem/nvmem.c
		${STM_FAMILY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
		${STM_FAMILY_DIR}/native_driver/src/clk/clk.c
		${STM_FAMILY_DIR}/native_driver/src/clk/clk-stm32-core.c
		${STM_FAMILY_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
		${STM_FAMILY_DIR}/native_driver/src/reset/core.c
		${STM_FAMILY_DIR}/native_driver/src/reset/stm32_reset.c
		${STM_FAMILY_DIR}/native_driver/src/timer/systick.c
		${STM_FAMILY_DIR}/native_driver/src/timer/plat_test.c
		${STM_FAMILY_DIR}/native_driver/src/pinctrl/common.c
		${STM_FAMILY_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
		${STM_FAMILY_DIR}/native_driver/src/uart/console.c
		${STM_FAMILY_DIR}/native_driver/src/uart/stm32_uart.c
		${STM_FAMILY_DIR}/native_driver/src/spi/stm32_ospi.c
		${STM_FAMILY_DIR}/native_driver/src/spi/stm32_omm.c
		${STM_FAMILY_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
		${STM_FAMILY_DIR}/native_driver/src/mtd/nor/spi_nor.c
		${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rif.c
		${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
		$<$<BOOL:${TFM_PLATFORM_WDT_API}>:${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_wdt_api.c>
		$<$<BOOL:${TFM_PLATFORM_WDT_API}>:${STM_FAMILY_DIR}/secure/services/src/wdt.c>
		$<$<BOOL:${TFM_PLATFORM_CPU_API}>:${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_cpu_api.c>
		$<$<BOOL:${TFM_PLATFORM_CPU_API}>:${STM_FAMILY_DIR}/secure/services/src/cpus.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/power/stm32mp2_pwr.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rifsc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risaf.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risab.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_iac.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_serc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/crypto/stm32_saes.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/regulator/regulator_common.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rtc/stm32_rtc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/memc/stm32_fmc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/memc/stm32mp2_ramcfg.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/dma/stm32_hpdma.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/interrupt-controller/stm32_exti.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp_nvram.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/remoteproc/rproc_core.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/remoteproc/stm32_rproc.c>
		$<$<BOOL:${STM32_CACHE_ENABLED}>:${STM_FAMILY_DIR}/native_driver/src/core/stm32_dcache.c>
		${STM_FAMILY_DIR}/native_driver/src/i2c/stm32_i2c.c
		$<$<BOOL:${TFM_PARTITION_PLATFORM}>:${STM_FAMILY_DIR}/secure/services/src/tfm_platform_system.c>
		$<$<BOOL:${STM32_HW_KEYSHARE}>:${STM_FAMILY_DIR}/secure/services/src/stm32_keyshare.c>
		$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_FAMILY_DIR}/secure/nv_counters.c>
		$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_ATTEST_HAL}>>:${STM_FAMILY_DIR}/secure/attest_hal.c>
		$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${STM_FAMILY_DIR}/secure/otp.c>
		${CMAKE_SOURCE_DIR}/platform/ext/common/tfm_hal_nvic.c
)

target_compile_options(platform_s
	PUBLIC
		${COMPILER_CMSE_FLAG}
)

target_compile_definitions(platform_s
	PUBLIC
		STM32_SEC=1
		$<$<BOOL:${TFM_PARTITION_SCP}>:CLK_MINIMAL_SZ=0>
		LOG_LEVEL=${LOG_LEVEL}
		LOG_LEVEL_UNPRIV=${LOG_LEVEL_UNPRIV}
)

target_link_libraries(platform_s
    PUBLIC
        lib_qsort
        tfm_log
        tfm_log_unpriv
        tfm_sp_log
)

if(TFM_PARTITION_CRYPTO)
	target_include_directories(platform_crypto_keys
		PUBLIC
			${STM_FAMILY_DIR}/accelerator
			${STM_FAMILY_DIR}/secure
	)

	target_sources(platform_crypto_keys
		PRIVATE
			$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_CRYPTO_KEYS}>>:${STM_FAMILY_DIR}/secure/crypto_keys.c>
	)

	target_link_libraries(platform_crypto_keys
		PUBLIC
			dt_platform_s_defs
			platform_region_defs
			platform_s
	)
endif()

if (TFM_PARTITION_SCP)
	add_subdirectory(${STM_FAMILY_DIR}/lib/SCP-firmware SCP-firmware)
	add_subdirectory(${STM_FAMILY_DIR}/secure_fw/partitions/scp scp)
endif()

if (TFM_PARTITION_STL)
	add_subdirectory(${STM_FAMILY_DIR}/lib/stl libstl)
	add_subdirectory(${STM_FAMILY_DIR}/secure_fw/partitions/stl stl)
endif()

if (TFM_PARTITION_IPCC)
	add_subdirectory(${STM_FAMILY_DIR}/secure_fw/partitions/ipcc ipcc)
endif()

if (TFM_PARTITION_NS_AGENT_MAILBOX)
	add_subdirectory(${PLATFORM_DIR}/ext/target/arm/rse/common/rse_comms rse)
endif()

if(TFM_PARTITION_PM)
	add_subdirectory(${STM_FAMILY_DIR}/secure_fw/partitions/pm pm)
endif()

# GNU ld has some strange behaviour to do with weak functions, and does not deal
# well with strong functions in static libraries overriding weak functions in
# object files.
# Ditto with devicetree driver.
target_sources(tfm_s
	PRIVATE
        ${STM_FAMILY_DIR}/native_driver/src/timer/systick.c
        ${STM_FAMILY_DIR}/native_driver/src/core/sau_armv8m_drv.c
        ${STM_FAMILY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
        ${STM_FAMILY_DIR}/native_driver/src/i2c/stm32_i2c.c
        ${STM_FAMILY_DIR}/native_driver/src/entropy/stm32_rng.c
	${STM_FAMILY_DIR}/native_driver/src/watchdog/stm32_iwdg.c
        ${STM_FAMILY_DIR}/native_driver/src/bsec/stm32_bsec3.c
        ${STM_FAMILY_DIR}/native_driver/src/nvmem/nvmem.c
        ${STM_FAMILY_DIR}/native_driver/src/spi/stm32_omm.c
        ${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
        $<$<BOOL:${TFM_PLATFORM_WDT_API}>:${STM_FAMILY_DIR}/secure/services/src/wdt.c>
        $<$<BOOL:${TFM_PLATFORM_CPU_API}>:${STM_FAMILY_DIR}/secure/services/src/cpus.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/power/stm32mp2_pwr.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rifsc.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risaf.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risab.c>
	$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/crypto/stm32_saes.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/regulator/stm32mp2_pmic.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rtc/stm32_rtc.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/memc/stm32_fmc.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/memc/stm32mp2_ramcfg.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/dma/stm32_hpdma.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/interrupt-controller/stm32_exti.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp_nvram.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/mbox/stm32_ipcc.c>
	$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_iac.c>
	$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/native_driver/src/rif/stm32_serc.c>
)

#========================= tfm_spm ============================================#
target_sources(tfm_spm
    PRIVATE
      $<$<OR:$<BOOL:${CONFIG_TFM_FLIH_API}>,$<BOOL:${CONFIG_TFM_SLIH_API}>>:${STM_FAMILY_DIR}/secure/tfm_interrupts.c>
)
#========================= Platform BL2 =======================================#
if(BL2)

	target_add_scatter_file(bl2
		$<$<C_COMPILER_ID:GNU>:${STM_FAMILY_DIR}/linker_scripts/gcc/stm32mp2_bl2.ld>
	)

	add_devicetree_target(TARGET platform_bl2
		DTS_BOARD ${DTS_BOARD_BL2}
		DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
	)

	add_dependencies(bl2_scatter dt_platform_bl2_gen_h)

	target_link_libraries(bl2_scatter
		dt_platform_bl2_defs
	)

	target_compile_definitions(bootutil
		PRIVATE
			MULTIPLE_EXECUTABLE_RAM_REGIONS
	)

	target_include_directories(platform_bl2
		PUBLIC
			${PLATFORM_DIR}/include
			${CMAKE_BINARY_DIR}/generated
			${STM_FAMILY_DIR}/accelerator
			${STM_FAMILY_DIR}/native_driver/include
			${STM_FAMILY_DIR}/cmsis_driver/include
			${STM_FAMILY_DIR}/secure
			${MCUBOOT_PATH}/boot/bootutil/include
			$<TARGET_PROPERTY:bl2,BINARY_DIR>/ext/mcuboot
			${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/include
			${CMAKE_SOURCE_DIR}/interface/include
	)

	target_sources(platform_bl2
		PRIVATE
			${DEVICETREE_DIR}/initlevel.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/cmsis_driver/src/driver_spi_flash.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/cmsis_driver/src/driver_mmc_flash.c>
			${STM_FAMILY_DIR}/bl2/boot_hal.c
			${STM_FAMILY_DIR}/bl2/banner.c
			${STM_FAMILY_DIR}/bl2/flash_map_bl2.c
			${STM_FAMILY_DIR}/cmsis_driver/src/driver_flash_mm.c
			${STM_FAMILY_DIR}/native_driver/src/core/mpu_armv8m_drv.c
			${STM_FAMILY_DIR}/native_driver/src/entropy/entropy.c
			${STM_FAMILY_DIR}/native_driver/src/entropy/stm32_rng.c
			${STM_FAMILY_DIR}/native_driver/src/watchdog/watchdog.c
			${STM_FAMILY_DIR}/native_driver/src/watchdog/stm32_iwdg.c
			${STM_FAMILY_DIR}/native_driver/src/power/stm32mp2_pwr.c
			${STM_FAMILY_DIR}/native_driver/src/bsec/stm32_bsec3.c
			${STM_FAMILY_DIR}/native_driver/src/nvmem/nvmem.c
			${STM_FAMILY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
			${STM_FAMILY_DIR}/native_driver/src/clk/clk.c
			${STM_FAMILY_DIR}/native_driver/src/clk/clk-stm32-core.c
			${STM_FAMILY_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
			${STM_FAMILY_DIR}/native_driver/src/reset/core.c
			${STM_FAMILY_DIR}/native_driver/src/reset/stm32_reset.c
			${STM_FAMILY_DIR}/native_driver/src/timer/systick.c
			${STM_FAMILY_DIR}/native_driver/src/pinctrl/common.c
			${STM_FAMILY_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
			${STM_FAMILY_DIR}/native_driver/src/uart/console.c
			${STM_FAMILY_DIR}/native_driver/src/uart/stm32_uart.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/spi/stm32_ospi.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/spi/stm32_omm.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/mtd/nor/spi_nor.c>
			${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rif.c
			${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risaf.c
			${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risab.c
			${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rifsc.c
			${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
			${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp.c
			${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp_nvram.c
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/mmc.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/stm32_sdmmc2.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/soft_crc.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/partition.c>
			${STM_FAMILY_DIR}/native_driver/src/i2c/stm32_i2c.c
			${STM_FAMILY_DIR}/native_driver/src/regulator/regulator_common.c
			${STM_FAMILY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c
			$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_FAMILY_DIR}/secure/nv_counters.c>
			$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${STM_FAMILY_DIR}/secure/otp.c>
			${STM_FAMILY_DIR}/secure/provisioning.c
			$<$<BOOL:${STM32_CACHE_ENABLED}>:${STM_FAMILY_DIR}/native_driver/src/core/stm32_dcache.c>
	)

	target_compile_options(platform_bl2
		PUBLIC
			${COMPILER_CMSE_FLAG}
	)

	target_compile_definitions(platform_bl2
		PUBLIC
			STM32_BL2=1
	)

	target_link_libraries(platform_bl2
        PRIVATE
            lib_qsort
            bl2_mbedcrypto_config
	)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files.
	target_sources(bl2
		PRIVATE
            ${STM_FAMILY_DIR}/bl2/banner.c
            ${STM_FAMILY_DIR}/native_driver/src/timer/systick.c
            ${STM_FAMILY_DIR}/native_driver/src/uart/console.c
            ${STM_FAMILY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
            ${STM_FAMILY_DIR}/native_driver/src/bsec/stm32_bsec3.c
            ${STM_FAMILY_DIR}/native_driver/src/nvmem/nvmem.c
            ${STM_FAMILY_DIR}/native_driver/src/entropy/stm32_rng.c
	    ${STM_FAMILY_DIR}/native_driver/src/watchdog/stm32_iwdg.c
            ${STM_FAMILY_DIR}/native_driver/src/i2c/stm32_i2c.c
            ${STM_FAMILY_DIR}/native_driver/src/regulator/stm32mp2_pmic.c
            ${STM_FAMILY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c
            ${STM_FAMILY_DIR}/native_driver/src/ddr/stm32mp2_ram.c
            ${STM_FAMILY_DIR}/native_driver/src/power/stm32mp2_pwr.c
            ${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rif.c
            ${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rifsc.c
            ${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risaf.c
            ${STM_FAMILY_DIR}/native_driver/src/rif/stm32_risab.c
            ${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp.c
            ${STM_FAMILY_DIR}/native_driver/src/tamper/stm32_tamp_nvram.c
            ${STM_FAMILY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/spi/stm32_omm.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/stm32_sdmmc2.c>
	)

	add_custom_target(bl2_stm32
		ALL
		DEPENDS bl2 bl2.stm32
	)

	add_custom_command(OUTPUT bl2.stm32
		COMMENT "STM32 image tool for bl2"
		COMMAND ${PYTHON_EXECUTABLE} ${STM_FAMILY_DIR}/tools/imageheader/stm32_imgtool.py
		-e $<TARGET_FILE_DIR:bl2>/bl2.elf
		-b $<TARGET_FILE_DIR:bl2>/bl2.bin
		-o $<TARGET_FILE_DIR:bl2>/bl2.stm32
		-bt 0x30
		-v_maj ${STM32_HEADER_MAJOR_VER}
		-v_min ${STM32_HEADER_MINOR_VER}
	)

	add_subdirectory(${STM_FAMILY_DIR}/native_driver/src/ddr native_driver/src/ddr)

endif()

#========================= Generate board model ===============================#
include(${STM_FAMILY_DIR}/stm_version.cmake)

#========================= Generate profile definition ========================#
configure_file(${STM_FAMILY_DIR}/stm_profile.h.in
               ${CMAKE_BINARY_DIR}/generated/stm_profile.h)

#========================= Files for building NS side platform ================#
install(DIRECTORY
    ${STM_FAMILY_DIR}/tests
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

install(DIRECTORY
    ${STM_DEVICETREE_DIR}
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common
)

install(DIRECTORY
    ${STM_FAMILY_DIR}/native_driver
    ${STM_FAMILY_DIR}/cmsis_driver
    ${STM_FAMILY_DIR}/ns
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx
)

install(DIRECTORY
    ${STM_FAMILY_DIR}/lib/qsort
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/lib
)

install(DIRECTORY
    ${STM_FAMILY_DIR}/secure/services/include
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/secure/services
)

install(FILES
    ${STM_FAMILY_DIR}/config.cmake
    ${STM_FAMILY_DIR}/cpuarch.cmake
    ${STM_FAMILY_DIR}/CMakeLists.txt
    ${STM_FAMILY_DIR}/config_tfm_stm32mp2.h
    ${STM_FAMILY_DIR}/tfm_ns_plat_init.c
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx
)

install(FILES
    ${STM_FAMILY_DIR}/secure/services/src/tfm_scmi_api.c
    ${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_cpu_api.c
    ${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_wdt_api.c
    ${STM_FAMILY_DIR}/secure/services/src/tfm_pm_api.c
    ${STM_FAMILY_DIR}/secure/services/src/tfm_stl_api.c
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/secure/services/src
)

install(FILES
    ${PLATFORM_DIR}/ext/common/uart_stdout.h
    ${PLATFORM_DIR}/ext/common/tfm_s_linker_alignments.h
    ${PLATFORM_DIR}/ext/driver/Driver_Common.h
    ${STM_FAMILY_DIR}/accelerator/crypto_accelerator_config.h
    ${STM_FAMILY_DIR}/accelerator/mbedtls_accelerator_config.h
    ${STM_FAMILY_DIR}/secure/tfm_builtin_key_ids.h
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/include
)

install(FILES
    ${STM_FAMILY_DIR}/linker_scripts/gcc/stm32mp2_ns.ld
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/linker_scripts
)

#========================= SIGNING CA35 FW ====================================#
if (EXISTS ${STM32_CA35_FW})
	set(BL2_SOURCE ${CMAKE_SOURCE_DIR}/bl2)
	set(API_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/api_ns)

	if (NOT EXISTS ${MCUBOOT_KEY_A35FW})
		set(MCUBOOT_KEY_A35FW ${MCUBOOT_KEY_S})
	endif()

	configure_file(${STM_FAMILY_DIR}/bl2/ca35fw_signing_layout.c.in ${STM_FAMILY_DIR}/bl2/ca35fw_signing_layout.c @ONLY)
	add_library(signing_layout_for_ca35_fw OBJECT ${STM_FAMILY_DIR}/bl2/ca35fw_signing_layout.c)

	target_link_libraries(signing_layout_for_ca35_fw
	PRIVATE
			platform_bl2
	)

	target_compile_options(signing_layout_for_ca35_fw
		PRIVATE
			$<$<C_COMPILER_ID:GNU>:-E\;-xc>
			$<$<C_COMPILER_ID:ARMClang>:-E\;-xc>
	)

	add_custom_target(ca35_fw_signed_bin
		ALL
		SOURCES ca35_fw_signed.bin
	)
	add_custom_command(OUTPUT ca35_fw_signed.bin
		DEPENDS ${STM32_CA35_FW}
		DEPENDS platform_bl2 signing_layout_for_ca35_fw
		DEPENDS $<IF:$<BOOL:${MCUBOOT_GENERATE_SIGNING_KEYPAIR}>,generated_private_key,>
		WORKING_DIRECTORY ${MCUBOOT_PATH}/scripts

		#Sign secure binary image with provided secret key
		COMMAND ${Python3_EXECUTABLE} ${BL2_SOURCE}/ext/mcuboot/scripts/wrapper/wrapper.py
			--layout $<TARGET_OBJECTS:signing_layout_for_ca35_fw>
			-k ${MCUBOOT_KEY_A35FW}
			--public-key-format $<IF:$<BOOL:${MCUBOOT_HW_KEY}>,full,hash>
			--align ${MCUBOOT_ALIGN_VAL}
			--pad
			--pad-header
			-H ${BL2_HEADER_SIZE}
			-s ${CA35FW_SECURITY_COUNTER_S}
			-v ${CA35FW_IMAGE_VERSION}
			-L ${MCUBOOT_ENC_KEY_LEN}
			$<$<STREQUAL:${MCUBOOT_UPGRADE_STRATEGY},OVERWRITE_ONLY>:--overwrite-only>
			$<$<BOOL:${MCUBOOT_CONFIRM_IMAGE}>:--confirm>
			$<$<BOOL:${MCUBOOT_ENC_IMAGES}>:-E${API_SOURCE}/image_signing/keys/image_enc_key.pem>
			$<$<BOOL:${MCUBOOT_MEASURED_BOOT}>:--measured-boot-record>
			${STM32_CA35_FW}
			${CMAKE_CURRENT_BINARY_DIR}/ca35_fw_signed.bin
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/ca35_fw_signed.bin $<TARGET_FILE_DIR:bl2>
	)
endif()
