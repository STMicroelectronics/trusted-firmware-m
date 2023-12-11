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
target_compile_definitions(platform_region_defs
	INTERFACE
		$<$<BOOL:${STM32_IPC}>:STM32_IPC>
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
	DTS_DIR ${DTS_EXT_DIR}
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
		${STM_SOC_DIR}/native_driver/src/pwr/stm32_pwr.c
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
		${STM_SOC_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
		${STM_SOC_DIR}/native_driver/src/mtd/nor/spi_nor.c
		${STM_SOC_DIR}/native_driver/src/rif/stm32_rif.c
		${STM_SOC_DIR}/native_driver/src/rif/stm32_rifsc.c
		${STM_SOC_DIR}/native_driver/src/rif/stm32_iac.c
		${STM_SOC_DIR}/native_driver/src/rif/stm32_serc.c
		${STM_SOC_DIR}/native_driver/src/core/stm32_icache.c
		${STM_SOC_DIR}/native_driver/src/core/stm32_dcache.c
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
		$<$<BOOL:${STM32_M33TDCID}>:STM32_M33TDCID>
		STM32_PROV_FAKE=${STM32_PROV_FAKE}
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

# GNU ld has some strange behaviour to do with weak functions, and does not deal
# well with strong functions in static libraries overriding weak functions in
# object files.
target_sources(tfm_s
	PRIVATE
		${STM_SOC_DIR}/device/source/startup_stm32mp2_s.c
		${STM_SOC_DIR}/native_driver/src/timer/systick.c
		${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
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
		DTS_DIR ${DTS_EXT_DIR}
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
			${STM_SOC_DIR}/native_driver/src/pwr/stm32_pwr.c
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
			$<$<BOOL:${STM32_M33TDCID}>:STM32_M33TDCID>
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
		DTS_DIR ${DTS_EXT_DIR}
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
			${MCUBOOT_PATH}/boot/bootutil/include
			$<TARGET_PROPERTY:bl2,BINARY_DIR>/ext/mcuboot
			${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/include
			${CMAKE_SOURCE_DIR}/interface/include
	)

	target_sources(platform_bl2
		PRIVATE
			${DEVICETREE_DIR}/initlevel.c
			${STM_SOC_DIR}/device/source/system_stm32mp2xx_m33.c
			${STM_SOC_DIR}/bl2/boot_hal.c
			${STM_SOC_DIR}/cmsis_driver/driver_spi_flash.c
			${STM_SOC_DIR}/cmsis_driver/driver_flash_mm.c
			${STM_SOC_DIR}/native_driver/src/core/mpu_armv8m_drv.c
			${STM_SOC_DIR}/native_driver/src/pwr/stm32_pwr.c
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
			${STM_SOC_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
			${STM_SOC_DIR}/native_driver/src/mtd/nor/spi_nor.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_rif.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_rifsc.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_iac.c
			${STM_SOC_DIR}/native_driver/src/rif/stm32_serc.c
			${STM_SOC_DIR}/native_driver/src/core/stm32_icache.c
			$<$<NOT:$<BOOL:${OTP_NV_COUNTERS_RAM_EMULATION}>>:${STM_SOC_DIR}/secure/nv_counters.c>
			${STM_COMMON_DIR}/hal/provision/otp_provision.c
	)

	target_compile_options(platform_bl2
		PUBLIC
			${COMPILER_CMSE_FLAG}
	)

	target_compile_definitions(platform_bl2
		PUBLIC
			STM32_BL2
			STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
			$<$<BOOL:${STM32_M33TDCID}>:STM32_M33TDCID>
			STM32_PROV_FAKE=${STM32_PROV_FAKE}
	)

	target_link_libraries(platform_bl2
		PUBLIC
			tfm_config
	)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files.
	target_sources(bl2
		PRIVATE
			${STM_SOC_DIR}/device/source/startup_stm32mp2_bl2.c
			${STM_SOC_DIR}/native_driver/src/timer/systick.c
			${STM_SOC_DIR}/native_driver/src/syscfg/stm32_syscfg.c
	)

	add_custom_target(bl2_stm32
		ALL
		SOURCES bl2.stm32
	)

	add_custom_command(OUTPUT bl2.stm32
		DEPENDS $<TARGET_FILE_DIR:bl2>/bl2.bin
		DEPENDS $<TARGET_FILE_DIR:bl2>/bl2.elf
		COMMENT "STM32 image tool for bl2"
		COMMAND ${PYTHON_EXECUTABLE} ${STM_SOC_DIR}/tools/imageheader/stm32_imgtool.py
		-e $<TARGET_FILE_DIR:bl2>/bl2.elf
		-b $<TARGET_FILE_DIR:bl2>/bl2.bin
		-o $<TARGET_FILE_DIR:bl2>/bl2.stm32
		-bt 0x30
	)

endif()
