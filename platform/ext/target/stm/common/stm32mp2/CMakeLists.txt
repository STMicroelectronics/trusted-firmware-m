#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

#========================= Platform region defs ===============================#

target_include_directories(platform_region_defs
	INTERFACE
		include
		${CMAKE_CURRENT_SOURCE_DIR}/boards
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/include
)

#========================= Platform common defs ===============================#
target_add_scatter_file(tfm_s
	$<$<C_COMPILER_ID:GNU>:${CMAKE_SOURCE_DIR}/platform/ext/common/gcc/tfm_common_s.ld>
)

if(NS)
	target_add_scatter_file(tfm_ns
		$<$<C_COMPILER_ID:GNU>:${STM_COMMON_DIR}/stm32mp2/device/source/gcc/stm32mp2_ns.ld>
	)

	target_link_libraries(CMSIS_5_tfm_ns
		INTERFACE
			CMSIS_5_RTX_V8MMN
	)
endif()

if(BL2)
	target_add_scatter_file(bl2
		$<$<C_COMPILER_ID:GNU>:${STM_COMMON_DIR}/stm32mp2/device/source/gcc/stm32mp2_bl2.ld>
	)
endif()

#========================= Platform Secure ====================================#

target_include_directories(platform_s
	PUBLIC
		${PLATFORM_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/include
		${CMAKE_CURRENT_SOURCE_DIR}/device/include
		${CMAKE_CURRENT_SOURCE_DIR}/secure/services/include
)

target_sources(platform_s
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/device/source/system_stm32mp2xx_m33.c
		${CMAKE_CURRENT_SOURCE_DIR}/secure/spm_hal.c
		${CMAKE_CURRENT_SOURCE_DIR}/secure/target_cfg.c
		${CMAKE_CURRENT_SOURCE_DIR}/secure/tfm_hal_isolation.c
		${CMAKE_CURRENT_SOURCE_DIR}/secure/tfm_hal_platform.c
		${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_spi_flash.c
		${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_flash_mm.c
		${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_usart.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/core/mpu_armv8m_drv.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/pwr/stm32_pwr.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/clk.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/clk-stm32-core.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/stm32mp2_clk.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/reset/stm32_reset.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/systick/stm32_systick.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/gpio/stm32_gpio.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/uart/stm32_uart.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/spi/stm32_ospi.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/mtd/nor/spi_nor.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_rif.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_rifsc.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_iac.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_serc.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/core/stm32_icache.c
		${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/core/stm32_dcache.c
		${CMAKE_SOURCE_DIR}/platform/ext/common/tfm_hal_isolation_mpu_v8m.c
		$<$<BOOL:${TFM_PARTITION_PLATFORM}>:${CMAKE_CURRENT_SOURCE_DIR}/secure/services/src/tfm_platform_system.c>
		${CMAKE_SOURCE_DIR}/platform/ext/common/tfm_hal_nvic.c
		${CMAKE_CURRENT_SOURCE_DIR}/secure/nv_counters.c
)

target_compile_options(platform_s
	PUBLIC
		${COMPILER_CMSE_FLAG}
)

target_compile_definitions(platform_s
	PUBLIC
		STM32_SEC
		STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
)

# GNU ld has some strange behaviour to do with weak functions, and does not deal
# well with strong functions in static libraries overriding weak functions in
# object files.
target_sources(tfm_s
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/device/source/startup_stm32mp2_s.c
)

#========================= Platform Non-Secure ================================#
if(NS)
	target_include_directories(platform_ns
		PUBLIC
			${PLATFORM_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/include
			${CMAKE_CURRENT_SOURCE_DIR}/device/include
	)

	target_sources(platform_ns
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/device/source/system_stm32mp2xx_m33.c
			${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_usart.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/pwr/stm32_pwr.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/clk.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/clk-stm32-core.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/stm32mp2_clk.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/reset/stm32_reset.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/systick/stm32_systick.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/gpio/stm32_gpio.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/uart/stm32_uart.c
	)

	target_compile_definitions(platform_ns
		PUBLIC
			STM32_NSEC
			STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
	)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files. For this reason, the file tfm_ns_plat_init is on tfm_ns target
	target_sources(tfm_ns
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/device/source/startup_stm32mp2_ns.c
			${CMAKE_CURRENT_SOURCE_DIR}/tfm_ns_plat_init.c
	)
endif()

#========================= Platform BL2 =======================================#

if(BL2)
	target_include_directories(platform_bl2
		PUBLIC
			${PLATFORM_DIR}/include
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/include
			${CMAKE_CURRENT_SOURCE_DIR}/device/include
			${MCUBOOT_PATH}/boot/bootutil/include
			$<TARGET_PROPERTY:bl2,BINARY_DIR>/ext/mcuboot
			${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/include
	)

	target_sources(platform_bl2
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/device/source/system_stm32mp2xx_m33.c
			${CMAKE_CURRENT_SOURCE_DIR}/bl2/boot_hal.c
			${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_spi_flash.c
			${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_flash_mm.c
			${CMAKE_CURRENT_SOURCE_DIR}/cmsis_driver/driver_usart.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/core/mpu_armv8m_drv.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/pwr/stm32_pwr.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/clk.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/clk-stm32-core.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/stm32mp_clkfunc.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/clk/stm32mp2_clk.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/reset/stm32_reset.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/systick/stm32_systick.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/gpio/stm32_gpio.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/uart/stm32_uart.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/spi/stm32_ospi.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/mtd/nor/spi_nor.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_rif.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_rifsc.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_iac.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/rif/stm32_serc.c
			${CMAKE_CURRENT_SOURCE_DIR}/native_driver/src/core/stm32_icache.c
			${CMAKE_CURRENT_SOURCE_DIR}/secure/nv_counters.c
			${CMAKE_CURRENT_SOURCE_DIR}/secure/otp_provision.c
	)

	target_compile_options(platform_bl2
		PUBLIC
			${COMPILER_CMSE_FLAG}
	)

	target_compile_definitions(platform_bl2
		PUBLIC
			STM32_BL2
			STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
	)

	# GNU ld has some strange behaviour to do with weak functions, and does not deal
	# well with strong functions in static libraries overriding weak functions in
	# object files.
	target_sources(bl2
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/device/source/startup_stm32mp2_bl2.c
	)
endif()
