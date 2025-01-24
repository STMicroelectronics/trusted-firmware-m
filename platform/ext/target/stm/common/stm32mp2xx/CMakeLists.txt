#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

# Generate board model
configure_file(${STM_FAMILLY_DIR}/stm_version.h.in
               ${CMAKE_BINARY_DIR}/generated/stm_version.h)
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
        # this flag is not defined in tfm_s and needed by stm32_bsec3
        $<$<BOOL:${TFM_DUMMY_PROVISIONING}>:TFM_DUMMY_PROVISIONING>
        $<$<BOOL:${TFM_PARTITION_SCP}>:S_HEAP_SIZE=0x00020000>
        $<$<BOOL:${STM32_DDR_CACHED}>:STM32_DDR_CACHED>
        STM32_PROV_FAKE=${STM32_PROV_FAKE}
)

target_include_directories(platform_region_defs
	INTERFACE
		include
		${STM_FAMILLY_DIR}
		${STM_FAMILLY_DIR}/native_driver/include
		${STM_FAMILLY_DIR}/cmsis_driver/include
		${STM_FAMILLY_DIR}/secure/services/include
)

#========================= Platform Secure ====================================#
target_add_scatter_file(tfm_s
	$<$<C_COMPILER_ID:GNU>:${STM_FAMILLY_DIR}/linker_scripts/gcc/stm32mp2_s.ld>
)

add_devicetree_target(TARGET platform_s
	DTS_BOARD ${DTS_BOARD_S}
	DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
)

add_dependencies(tfm_s_scatter dt_platform_s_gen_h)

target_link_libraries(tfm_s_scatter
	dt_platform_s_defs
)

add_subdirectory(${STM_FAMILLY_DIR}/lib/qsort qsort)

target_include_directories(platform_s
	PUBLIC
		${PLATFORM_DIR}/include
		${CMAKE_BINARY_DIR}/generated
		${STM_FAMILLY_DIR}/native_driver
		${STM_FAMILLY_DIR}/native_driver/include
		${STM_FAMILLY_DIR}/cmsis_driver/include
		${STM_FAMILLY_DIR}/secure
		${STM_FAMILLY_DIR}/secure/services/include
)

target_sources(platform_s
	PRIVATE
		${DEVICETREE_DIR}/initlevel.c
		${STM_FAMILLY_DIR}/secure/target_cfg.c
		${STM_FAMILLY_DIR}/secure/tfm_hal_isolation.c
		${STM_FAMILLY_DIR}/secure/provisioning.c
		${STM_FAMILLY_DIR}/secure/tfm_hal_platform.c
		${STM_FAMILLY_DIR}/cmsis_driver/src/driver_spi_flash.c
		${STM_FAMILLY_DIR}/cmsis_driver/src/driver_flash_mm.c
		${STM_FAMILLY_DIR}/native_driver/src/core/mpu_armv8m_drv.c
		${STM_FAMILLY_DIR}/native_driver/src/core/sau_armv8m_drv.c
		${STM_FAMILLY_DIR}/native_driver/src/power/stm32mp2_pwr.c
		${STM_FAMILLY_DIR}/native_driver/src/bsec/stm32_bsec3.c
		${STM_FAMILLY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
		${STM_FAMILLY_DIR}/native_driver/src/clk/clk.c
		${STM_FAMILLY_DIR}/native_driver/src/clk/clk-stm32-core.c
		${STM_FAMILLY_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
		${STM_FAMILLY_DIR}/native_driver/src/reset/core.c
		${STM_FAMILLY_DIR}/native_driver/src/reset/stm32_reset.c
		${STM_FAMILLY_DIR}/native_driver/src/timer/systick.c
		${STM_FAMILLY_DIR}/native_driver/src/timer/plat_test.c
		${STM_FAMILLY_DIR}/native_driver/src/pinctrl/common.c
		${STM_FAMILLY_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
		${STM_FAMILLY_DIR}/native_driver/src/uart/console.c
		${STM_FAMILLY_DIR}/native_driver/src/uart/stm32_uart.c
		${STM_FAMILLY_DIR}/native_driver/src/spi/stm32_ospi.c
		${STM_FAMILLY_DIR}/native_driver/src/spi/stm32_omm.c
		${STM_FAMILLY_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
		${STM_FAMILLY_DIR}/native_driver/src/mtd/nor/spi_nor.c
		${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rif.c
		${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rifsc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risaf.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risab.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_iac.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_serc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/regulator/regulator_common.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rtc/stm32_rtc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/memc/stm32_fmc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/dma/stm32_hpdma.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/interrupt-controller/stm32_exti.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/tamper/stm32_tamp.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/remoteproc/rproc_core.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/remoteproc/stm32_rproc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/secure/services/src/tfm_ioctl_cpu_api.c>
		$<$<BOOL:${STM32_DDR_CACHED}>:${STM_FAMILLY_DIR}/native_driver/src/core/stm32_icache.c>
		$<$<BOOL:${STM32_DDR_CACHED}>:${STM_FAMILLY_DIR}/native_driver/src/core/stm32_dcache.c>
		${STM_FAMILLY_DIR}/native_driver/src/i2c/stm32_i2c.c
		$<$<BOOL:${TFM_PARTITION_PLATFORM}>:${STM_FAMILLY_DIR}/secure/services/src/tfm_platform_system.c>
		$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_FAMILLY_DIR}/secure/nv_counters.c>
		$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${STM_FAMILLY_DIR}/secure/otp.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/secure/services/src/cpus.c>
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
)

if(TFM_PARTITION_CRYPTO)
        target_include_directories(platform_crypto_keys
                PUBLIC
                        ${PLATFORM_DIR}/include
                        ${STM_FAMILLY_DIR}/native_driver/include
                        ${STM_FAMILLY_DIR}/cmsis_driver/include
                        ${STM_FAMILLY_DIR}/secure
                        ${STM_FAMILLY_DIR}/secure/services/include
        )

	target_link_libraries(platform_crypto_keys
		PUBLIC
			dt_platform_s_defs
	)
endif()

if (TFM_PARTITION_SCP)
	add_subdirectory(${STM_FAMILLY_DIR}/lib/SCP-firmware SCP-firmware)
	add_subdirectory(${STM_FAMILLY_DIR}/secure_fw/partitions/scp scp)
endif()
if (TFM_PARTITION_IPCC)
	add_subdirectory(${STM_FAMILLY_DIR}/secure_fw/partitions/ipcc ipcc)
endif()

# GNU ld has some strange behaviour to do with weak functions, and does not deal
# well with strong functions in static libraries overriding weak functions in
# object files.
# Ditto with devicetree driver.
target_sources(tfm_s
	PRIVATE
        ${STM_FAMILLY_DIR}/native_driver/src/timer/systick.c
        ${STM_FAMILLY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
        ${STM_FAMILLY_DIR}/native_driver/src/i2c/stm32_i2c.c
        ${STM_FAMILLY_DIR}/native_driver/src/power/stm32mp2_pwr.c
        ${STM_FAMILLY_DIR}/native_driver/src/bsec/stm32_bsec3.c
        ${STM_FAMILLY_DIR}/native_driver/src/spi/stm32_omm.c
        ${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rifsc.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risaf.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risab.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/regulator/stm32mp2_pmic.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/secure/services/src/cpus.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/rtc/stm32_rtc.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/memc/stm32_fmc.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/dma/stm32_hpdma.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/interrupt-controller/stm32_exti.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/tamper/stm32_tamp.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILLY_DIR}/native_driver/src/mbox/stm32_ipcc.c>
)

#========================= tfm_spm ============================================#
target_sources(tfm_spm
    PRIVATE
      $<$<OR:$<BOOL:${CONFIG_TFM_FLIH_API}>,$<BOOL:${CONFIG_TFM_SLIH_API}>>:${STM_FAMILLY_DIR}/secure/tfm_interrupts.c>
)
#========================= Platform BL2 =======================================#
if(BL2)

	target_add_scatter_file(bl2
		$<$<C_COMPILER_ID:GNU>:${STM_FAMILLY_DIR}/linker_scripts/gcc/stm32mp2_bl2.ld>
	)

	add_devicetree_target(TARGET platform_bl2
		DTS_BOARD ${DTS_BOARD_BL2}
		DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
	)

	add_dependencies(bl2_scatter dt_platform_bl2_gen_h)

	target_link_libraries(bl2_scatter
		dt_platform_bl2_defs
	)

	target_include_directories(platform_bl2
		PUBLIC
			${PLATFORM_DIR}/include
			${STM_FAMILLY_DIR}/native_driver
			${STM_FAMILLY_DIR}/native_driver/include
			${STM_FAMILLY_DIR}/cmsis_driver/include
			${STM_FAMILLY_DIR}/secure
			${MCUBOOT_PATH}/boot/bootutil/include
			$<TARGET_PROPERTY:bl2,BINARY_DIR>/ext/mcuboot
			${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/include
			${CMAKE_SOURCE_DIR}/interface/include
	)

	target_sources(platform_bl2
		PRIVATE
			${DEVICETREE_DIR}/initlevel.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILLY_DIR}/cmsis_driver/src/driver_spi_flash.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILLY_DIR}/cmsis_driver/src/driver_mmc_flash.c>
			${STM_FAMILLY_DIR}/bl2/boot_hal.c
			${STM_FAMILLY_DIR}/cmsis_driver/src/driver_flash_mm.c
			${STM_FAMILLY_DIR}/native_driver/src/core/mpu_armv8m_drv.c
			${STM_FAMILLY_DIR}/native_driver/src/power/stm32mp2_pwr.c
			${STM_FAMILLY_DIR}/native_driver/src/bsec/stm32_bsec3.c
			${STM_FAMILLY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
			${STM_FAMILLY_DIR}/native_driver/src/clk/clk.c
			${STM_FAMILLY_DIR}/native_driver/src/clk/clk-stm32-core.c
			${STM_FAMILLY_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
			${STM_FAMILLY_DIR}/native_driver/src/reset/core.c
			${STM_FAMILLY_DIR}/native_driver/src/reset/stm32_reset.c
			${STM_FAMILLY_DIR}/native_driver/src/timer/systick.c
			${STM_FAMILLY_DIR}/native_driver/src/pinctrl/common.c
			${STM_FAMILLY_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
			${STM_FAMILLY_DIR}/native_driver/src/uart/console.c
			${STM_FAMILLY_DIR}/native_driver/src/uart/stm32_uart.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILLY_DIR}/native_driver/src/spi/stm32_ospi.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILLY_DIR}/native_driver/src/spi/stm32_omm.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILLY_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILLY_DIR}/native_driver/src/mtd/nor/spi_nor.c>
			${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rif.c
			${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risaf.c
			${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risab.c
			${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rifsc.c
			${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
			${STM_FAMILLY_DIR}/native_driver/src/tamper/stm32_tamp.c
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILLY_DIR}/native_driver/src/mmc/mmc.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILLY_DIR}/native_driver/src/mmc/stm32_sdmmc2.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILLY_DIR}/native_driver/src/mmc/soft_crc.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILLY_DIR}/native_driver/src/mmc/partition.c>
			$<$<BOOL:${STM32_DDR_CACHED}>:${STM_FAMILLY_DIR}/native_driver/src/core/stm32_icache.c>
			${STM_FAMILLY_DIR}/native_driver/src/i2c/stm32_i2c.c
			${STM_FAMILLY_DIR}/native_driver/src/regulator/regulator_common.c
			${STM_FAMILLY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c
			$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_FAMILLY_DIR}/secure/nv_counters.c>
			$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${STM_FAMILLY_DIR}/secure/otp.c>
			${STM_FAMILLY_DIR}/secure/provisioning.c
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

	add_subdirectory(${STM_FAMILLY_DIR}/native_driver/src/ddr native_driver/src/ddr)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files.
	target_sources(bl2
		PRIVATE
            ${STM_FAMILLY_DIR}/native_driver/src/timer/systick.c
            ${STM_FAMILLY_DIR}/native_driver/src/uart/console.c
            ${STM_FAMILLY_DIR}/native_driver/src/syscfg/stm32_syscfg.c
            ${STM_FAMILLY_DIR}/native_driver/src/bsec/stm32_bsec3.c
            ${STM_FAMILLY_DIR}/native_driver/src/i2c/stm32_i2c.c
            ${STM_FAMILLY_DIR}/native_driver/src/regulator/stm32mp2_pmic.c
            ${STM_FAMILLY_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c
            ${STM_FAMILLY_DIR}/native_driver/src/ddr/stm32mp2_ram.c
            ${STM_FAMILLY_DIR}/native_driver/src/power/stm32mp2_pwr.c
            ${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rif.c
            ${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rifsc.c
            ${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risaf.c
            ${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_risab.c
            ${STM_FAMILLY_DIR}/native_driver/src/tamper/stm32_tamp.c
            ${STM_FAMILLY_DIR}/native_driver/src/rif/stm32_rcc_rif.c
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILLY_DIR}/native_driver/src/spi/stm32_omm.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILLY_DIR}/native_driver/src/mmc/stm32_sdmmc2.c>
	)

	add_custom_target(bl2_stm32
		ALL
		DEPENDS bl2 bl2.stm32
	)

	add_custom_command(OUTPUT bl2.stm32
		COMMENT "STM32 image tool for bl2"
		COMMAND ${PYTHON_EXECUTABLE} ${STM_FAMILLY_DIR}/tools/imageheader/stm32_imgtool.py
		-e $<TARGET_FILE_DIR:bl2>/bl2.elf
		-b $<TARGET_FILE_DIR:bl2>/bl2.bin
		-o $<TARGET_FILE_DIR:bl2>/bl2.stm32
		-bt 0x30
		-v_maj ${STM32_HEADER_MAJOR_VER}
		-v_min ${STM32_HEADER_MINOR_VER}
	)

endif()
#========================= Files for building NS side platform ================#
install(DIRECTORY
    ${STM_DEVICETREE_DIR}
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common
)

install(DIRECTORY
    ${STM_FAMILLY_DIR}/native_driver
    ${STM_FAMILLY_DIR}/cmsis_driver
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx
)

install(DIRECTORY
    ${STM_FAMILLY_DIR}/lib/qsort
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/lib
)

install(DIRECTORY
    ${STM_FAMILLY_DIR}/secure/services/include
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/secure/services
)

install(FILES
    ${STM_FAMILLY_DIR}/config.cmake
    ${STM_FAMILLY_DIR}/cpuarch.cmake
    ${STM_FAMILLY_DIR}/CMakeLists.txt
    ${STM_FAMILLY_DIR}/tfm_ns_plat_init.c
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx
)

install(FILES
    ${STM_FAMILLY_DIR}/secure/services/src/tfm_ioctl_cpu_api.c
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/secure/services/src
)

install(FILES
    ${PLATFORM_DIR}/ext/common/uart_stdout.h
    ${PLATFORM_DIR}/ext/common/tfm_s_linker_alignments.h
    ${PLATFORM_DIR}/ext/driver/Driver_Common.h
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/include
)

install(FILES
    ${STM_FAMILLY_DIR}/linker_scripts/gcc/stm32mp2_ns.ld
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/linker_scripts
)
