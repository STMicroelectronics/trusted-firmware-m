#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

# Generate board model
configure_file(${STM_SOC_DIR}/boards/board_model.h.in
               ${CMAKE_BINARY_DIR}/generated/board_model.h)
#========================= Device tree ========================================#
include(${STM_DEVICETREE_DIR}/gen_dt.cmake)

#========================= Platform region defs ===============================#
# these flags are includes in bl2, tfm_s and tfm_ns
target_compile_definitions(platform_region_defs
	INTERFACE
		$<$<BOOL:${STM32_IPC}>:STM32_IPC>
		$<$<STREQUAL:${STM32_STM32MP2_SOC_REV},revA>:CONFIG_STM32MP25X_REVA=1>
		$<$<STREQUAL:${STM32_STM32MP2_SOC_REV},revB>:CONFIG_STM32MP25X_REVB=1>
		$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:STM32_BOOT_DEV_OSPI>
		$<$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>:STM32_BOOT_DEV_SDMMC1>
		$<$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>:STM32_BOOT_DEV_SDMMC2>
		$<$<STREQUAL:${STM32_BOOT_DEV},ddr>:STM32_BOOT_DEV_DDR>
		$<$<BOOL:${STM32_M33TDCID}>:STM32_M33TDCID>
		# this flag is not defined in tfm_s and needed by stm32_bsec3
		$<$<BOOL:${TFM_DUMMY_PROVISIONING}>:TFM_DUMMY_PROVISIONING>
		$<$<BOOL:${TFM_PARTITION_SCP}>:S_HEAP_SIZE=0x00020000>
)

target_include_directories(platform_region_defs
	INTERFACE
		include
		${STM_SOC_DIR}/boards
		${STM_SOC_DIR}/native_driver/include
)

#========================= Platform psa interface =============================#
target_include_directories(psa_interface
	INTERFACE
		${STM_SOC_DIR}/secure/services/include
)

#========================= Platform Secure ====================================#
target_add_scatter_file(tfm_s
	$<$<C_COMPILER_ID:GNU>:${STM_SOC_DIR}/device/source/gcc/stm32mp2_s.ld>
)

add_devicetree_target(TARGET platform_s
	DTS_BOARD ${DTS_BOARD_S}
	DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
)
add_dependencies(tfm_s_scatter dt_platform_s_gen_h)
target_link_libraries(tfm_s_scatter
	dt_platform_s_defs
)

target_include_directories(platform_s
	PUBLIC
		${PLATFORM_DIR}/include
	        ${CMAKE_BINARY_DIR}/generated
		${STM_SOC_DIR}/native_driver
		${STM_SOC_DIR}/native_driver/include
		${STM_SOC_DIR}/device/include
		${STM_SOC_DIR}/secure
		${STM_SOC_DIR}/secure/services/include
)

target_sources(platform_s
	PRIVATE
		${DEVICETREE_DIR}/initlevel.c
		${STM_SOC_DIR}/device/source/system_stm32mp2xx_m33.c
		${STM_SOC_DIR}/secure/target_cfg.c
		${STM_SOC_DIR}/secure/tfm_hal_isolation.c
		${STM_SOC_DIR}/secure/provisioning.c
		${STM_SOC_DIR}/secure/tfm_hal_platform.c
		${STM_SOC_DIR}/cmsis_driver/driver_spi_flash.c
		${STM_SOC_DIR}/cmsis_driver/driver_flash_mm.c
		${STM_SOC_DIR}/native_driver/src/core/mpu_armv8m_drv.c
		${STM_SOC_DIR}/native_driver/src/core/sau_armv8m_drv.c
		${STM_SOC_DIR}/native_driver/src/power/stm32mp2_pwr.c
		${STM_SOC_DIR}/native_driver/src/bsec/stm32_bsec3.c
		${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
		${STM_SOC_DIR}/native_driver/src/clk/clk.c
		${STM_SOC_DIR}/native_driver/src/clk/clk-stm32-core.c
		${STM_SOC_DIR}/native_driver/src/clk/clk-stm32mp25.c
		${STM_SOC_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
		${STM_SOC_DIR}/native_driver/src/reset/core.c
		${STM_SOC_DIR}/native_driver/src/reset/stm32_reset.c
		${STM_SOC_DIR}/native_driver/src/timer/systick.c
		${STM_SOC_DIR}/native_driver/src/pinctrl/common.c
		${STM_SOC_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
		${STM_SOC_DIR}/native_driver/src/uart/console.c
		${STM_SOC_DIR}/native_driver/src/uart/stm32_uart.c
		${STM_SOC_DIR}/native_driver/src/spi/stm32_ospi.c
		${STM_SOC_DIR}/native_driver/src/spi/stm32_omm.c
		${STM_SOC_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
		${STM_SOC_DIR}/native_driver/src/mtd/nor/spi_nor.c
		${STM_SOC_DIR}/native_driver/src/rif/stm32_rif.c
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_rifsc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_risaf.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_risab.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_iac.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_serc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/regulator/regulator_common.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/regulator/stm32mp2_pmic.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rtc/stm32_rtc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/memc/stm32_fmc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/dma/stm32_hpdma.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/interrupt-controller/stm32_exti.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/tamper/stm32_tamp.c>
		$<$<BOOL:${STM32_DDR_CACHED}>:${STM_SOC_DIR}/native_driver/src/core/stm32_icache.c>
		$<$<BOOL:${STM32_DDR_CACHED}>:${STM_SOC_DIR}/native_driver/src/core/stm32_dcache.c>
		${STM_SOC_DIR}/native_driver/src/i2c/stm32_i2c.c
		$<$<BOOL:${TFM_PARTITION_PLATFORM}>:${STM_SOC_DIR}/secure/services/src/tfm_platform_system.c>
		$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_SOC_DIR}/secure/nv_counters.c>
		$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${STM_SOC_DIR}/secure/otp.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/secure/services/src/stm32_copro.c>
		${CMAKE_SOURCE_DIR}/platform/ext/common/tfm_hal_nvic.c
)

target_compile_options(platform_s
	PUBLIC
		${COMPILER_CMSE_FLAG}
)

target_compile_definitions(platform_s
	PUBLIC
		STM32_SEC
		STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
		STM32_PROV_FAKE=${STM32_PROV_FAKE}
		$<$<BOOL:${TFM_PARTITION_SCP}>:CLK_MINIMAL_SZ=0>
)

if(TFM_PARTITION_CRYPTO)
        target_include_directories(platform_crypto_keys
                PUBLIC
                        ${PLATFORM_DIR}/include
                        ${STM_SOC_DIR}/native_driver/include
                        ${STM_SOC_DIR}/device/include
                        ${STM_SOC_DIR}/secure
                        ${STM_SOC_DIR}/secure/services/include
        )

	target_link_libraries(platform_crypto_keys
		PUBLIC
			dt_platform_s_defs
	)
endif()

if (TFM_PARTITION_SCP)
	add_subdirectory(${STM_SOC_DIR}/lib/SCP-firmware SCP-firmware)
	add_subdirectory(${STM_SOC_DIR}/secure_fw/partitions/scp scp)
endif()

# GNU ld has some strange behaviour to do with weak functions, and does not deal
# well with strong functions in static libraries overriding weak functions in
# object files.
# Ditto with devicetree driver.
target_sources(tfm_s
	PRIVATE
		${STM_SOC_DIR}/device/source/startup_stm32mp2_s.c
		${STM_SOC_DIR}/native_driver/src/timer/systick.c
		${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
		${STM_SOC_DIR}/native_driver/src/i2c/stm32_i2c.c
		${STM_SOC_DIR}/native_driver/src/power/stm32mp2_pwr.c
		${STM_SOC_DIR}/native_driver/src/bsec/stm32_bsec3.c
		${STM_SOC_DIR}/native_driver/src/spi/stm32_omm.c
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_rifsc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_risaf.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rif/stm32_risab.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/regulator/stm32mp2_pmic.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/secure/services/src/stm32_copro.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/rtc/stm32_rtc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/memc/stm32_fmc.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/dma/stm32_hpdma.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/interrupt-controller/stm32_exti.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/tamper/stm32_tamp.c>
		$<$<BOOL:${STM32_M33TDCID}>:${STM_SOC_DIR}/native_driver/src/mbox/stm32_ipcc.c>
)

#========================= Platform Non-Secure ================================#
if(NS)
	target_add_scatter_file(tfm_ns
		$<$<C_COMPILER_ID:GNU>:${STM_SOC_DIR}/device/source/gcc/stm32mp2_ns.ld>
	)

	target_link_libraries(CMSIS_5_tfm_ns
		INTERFACE
			CMSIS_5_RTX_V8MMN
	)

	add_devicetree_target(TARGET platform_ns
		DTS_BOARD ${DTS_BOARD_NS}
		DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
		DTC_FLAGS "-Wno-simple_bus_reg"
	)
	add_dependencies(tfm_ns_scatter dt_platform_ns_gen_h)
	target_link_libraries(tfm_ns_scatter
		dt_platform_ns_defs
	)

	target_include_directories(platform_ns
		PUBLIC
		        ${CMAKE_BINARY_DIR}/generated
			${PLATFORM_DIR}/include
			${STM_SOC_DIR}/native_driver
			${STM_SOC_DIR}/native_driver/include
			${STM_SOC_DIR}/device/include
			${STM_SOC_DIR}/secure/services/include
	)

	target_sources(platform_ns
		PRIVATE
			${DEVICETREE_DIR}/initlevel.c
			${STM_SOC_DIR}/device/source/system_stm32mp2xx_m33.c
			${STM_SOC_DIR}/native_driver/src/clk/clk.c
			${STM_SOC_DIR}/native_driver/src/clk/clk-stm32-core.c
			${STM_SOC_DIR}/native_driver/src/clk/clk-stm32mp25.c
			${STM_SOC_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
			${STM_SOC_DIR}/native_driver/src/reset/core.c
			${STM_SOC_DIR}/native_driver/src/reset/stm32_reset.c
			${STM_SOC_DIR}/native_driver/src/timer/systick.c
			${STM_SOC_DIR}/native_driver/src/pinctrl/common.c
			${STM_SOC_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
			${STM_SOC_DIR}/native_driver/src/uart/console.c
			${STM_SOC_DIR}/native_driver/src/uart/stm32_uart.c
	)

	target_compile_definitions(platform_ns
		PUBLIC
			STM32_NSEC
			STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
	)

	target_link_libraries(platform_ns
		PUBLIC
			tfm_config
	)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files. For this reason, the file tfm_ns_plat_init is on tfm_ns target
	target_sources(tfm_ns
		PRIVATE
			${STM_SOC_DIR}/device/source/startup_stm32mp2_ns.c
			${STM_SOC_DIR}/tfm_ns_plat_init.c
			${STM_SOC_DIR}/native_driver/src/timer/systick.c
			${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
	)
endif()

#========================= Platform BL2 =======================================#
if(BL2)

	target_add_scatter_file(bl2
		$<$<C_COMPILER_ID:GNU>:${STM_SOC_DIR}/device/source/gcc/stm32mp2_bl2.ld>
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
			${STM_SOC_DIR}/native_driver
			${STM_SOC_DIR}/native_driver/include
			${STM_SOC_DIR}/device/include
			${STM_SOC_DIR}/secure
			${MCUBOOT_PATH}/boot/bootutil/include
			$<TARGET_PROPERTY:bl2,BINARY_DIR>/ext/mcuboot
			${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/include
			${CMAKE_SOURCE_DIR}/interface/include
	)

	target_sources(platform_bl2
		PRIVATE
			${DEVICETREE_DIR}/initlevel.c
			${STM_SOC_DIR}/device/source/system_stm32mp2xx_m33.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_SOC_DIR}/cmsis_driver/driver_spi_flash.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_SOC_DIR}/cmsis_driver/driver_mmc_flash.c>
			${STM_SOC_DIR}/bl2/boot_hal.c
			${STM_SOC_DIR}/cmsis_driver/driver_flash_mm.c
			${STM_SOC_DIR}/native_driver/src/core/mpu_armv8m_drv.c
			${STM_SOC_DIR}/native_driver/src/power/stm32mp2_pwr.c
			${STM_SOC_DIR}/native_driver/src/bsec/stm32_bsec3.c
			${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
			${STM_SOC_DIR}/native_driver/src/clk/clk.c
			${STM_SOC_DIR}/native_driver/src/clk/clk-stm32-core.c
			${STM_SOC_DIR}/native_driver/src/clk/clk-stm32mp25.c
			${STM_SOC_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
			${STM_SOC_DIR}/native_driver/src/reset/core.c
			${STM_SOC_DIR}/native_driver/src/reset/stm32_reset.c
			${STM_SOC_DIR}/native_driver/src/timer/systick.c
			${STM_SOC_DIR}/native_driver/src/pinctrl/common.c
			${STM_SOC_DIR}/native_driver/src/pinctrl/stm32_pinctrl.c
			${STM_SOC_DIR}/native_driver/src/uart/console.c
			${STM_SOC_DIR}/native_driver/src/uart/stm32_uart.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_SOC_DIR}/native_driver/src/spi/stm32_ospi.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_SOC_DIR}/native_driver/src/spi/stm32_omm.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_SOC_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c>
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_SOC_DIR}/native_driver/src/mtd/nor/spi_nor.c>
			${STM_SOC_DIR}/native_driver/src/rif/stm32_rif.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_risaf.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_risab.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_rifsc.c
			${STM_SOC_DIR}/native_driver/src/tamper/stm32_tamp.c
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_SOC_DIR}/native_driver/src/mmc/mmc.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_SOC_DIR}/native_driver/src/mmc/stm32_sdmmc2.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_SOC_DIR}/native_driver/src/mmc/soft_crc.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_SOC_DIR}/native_driver/src/mmc/partition.c>
			$<$<BOOL:${STM32_DDR_CACHED}>:${STM_SOC_DIR}/native_driver/src/core/stm32_icache.c>
			${STM_SOC_DIR}/native_driver/src/i2c/stm32_i2c.c
			${STM_SOC_DIR}/native_driver/src/regulator/regulator_common.c
			${STM_SOC_DIR}/native_driver/src/regulator/stm32mp2_pmic.c
			${STM_SOC_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c
			$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_SOC_DIR}/secure/nv_counters.c>
			$<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${STM_SOC_DIR}/secure/otp.c>
			${STM_SOC_DIR}/secure/provisioning.c
	)

	target_compile_options(platform_bl2
		PUBLIC
			${COMPILER_CMSE_FLAG}
	)

	target_compile_definitions(platform_bl2
		PUBLIC
			STM32_BL2
			STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
			STM32_PROV_FAKE=${STM32_PROV_FAKE}
	)

	target_link_libraries(platform_bl2
		PUBLIC
			tfm_config
	)

	add_subdirectory(${STM_SOC_DIR}/native_driver/src/ddr native_driver/src/ddr)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files.
	target_sources(bl2
		PRIVATE
			${STM_SOC_DIR}/device/source/startup_stm32mp2_bl2.c
			${STM_SOC_DIR}/native_driver/src/timer/systick.c
			${STM_SOC_DIR}/native_driver/src/uart/console.c
			${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
			${STM_SOC_DIR}/native_driver/src/bsec/stm32_bsec3.c
			${STM_SOC_DIR}/native_driver/src/i2c/stm32_i2c.c
			${STM_SOC_DIR}/native_driver/src/regulator/stm32mp2_pmic.c
			${STM_SOC_DIR}/native_driver/src/regulator/stm32mp25_pwr_regul.c
			${STM_SOC_DIR}/native_driver/src/ddr/stm32mp2_ram.c
			${STM_SOC_DIR}/native_driver/src/power/stm32mp2_pwr.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_rif.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_rifsc.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_risaf.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_risab.c
			${STM_SOC_DIR}/native_driver/src/tamper/stm32_tamp.c
			$<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_SOC_DIR}/native_driver/src/spi/stm32_omm.c>
			$<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_SOC_DIR}/native_driver/src/mmc/stm32_sdmmc2>
	)

	add_custom_target(bl2_stm32
		ALL
		DEPENDS bl2 bl2.stm32
	)

	add_custom_command(OUTPUT bl2.stm32
		COMMENT "STM32 image tool for bl2"
		COMMAND ${PYTHON_EXECUTABLE} ${STM_SOC_DIR}/tools/imageheader/stm32_imgtool.py
		-e $<TARGET_FILE_DIR:bl2>/bl2.elf
		-b $<TARGET_FILE_DIR:bl2>/bl2.bin
		-o $<TARGET_FILE_DIR:bl2>/bl2.stm32
		-bt 0x30
		-v_maj ${STM32_HEADER_MAJOR_VER}
		-v_min ${STM32_HEADER_MINOR_VER}
	)

endif()
