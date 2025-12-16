#-------------------------------------------------------------------------------
# Copyright (c) 2023-2024, Arm Limited. All rights reserved.
# Copyright (c) 2024 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# This file is exported to NS side during CMake installation phase and renamed
# to CMakeLists.txt. It instructs how to build a platform on non-secture side.
# The structure and sources list are fully platform specific.

add_library(platform_ns)

include(cmsis_core_config)
add_subdirectory(${CMSIS_CORE_PATH} cmsis-core)

include(cmsis_device_mp2_config)
add_subdirectory(${CMSIS_DEVICE_MP2_PATH} cmsis-device-mp2)

include(stm32mp2xx_hal_driver_config)
add_subdirectory(${STM32MP2XX_HAL_DRIVER_PATH} stm32mp2xx-hal-driver)

#========================= Device tree ========================================#
include(${STM_DEVICETREE_DIR}/gen_dt.cmake)

add_devicetree_target(TARGET platform_ns
    DTS_BOARD ${DTS_BOARD_NS}
    DTS_DIR ${DTS_EXT_DIR} ${DT_DTS_DIR}/${STM32_DTS_DIR}
    DTC_FLAGS "-Wno-simple_bus_reg"
)

add_dependencies(tfm_ns dt_platform_ns_gen_h)

target_link_libraries(tfm_ns
    PUBLIC
        dt_platform_ns_defs
)

#======================= Platform Non-Secure ==================================#
# Include region_defs.h and flash_layout.h
target_include_directories(platform_region_defs
    INTERFACE
        partition
        ${PLATFORM_NS_API_DIR}/include
        ${STM_FAMILY_DIR}/native_driver/include
        ${STM_FAMILY_DIR}/cmsis_driver/include
)

target_include_directories(tfm_config
    INTERFACE
        $<BUILD_INTERFACE:${STM_FAMILY_DIR}>
        $<BUILD_INTERFACE:${STM_SOC_DIR}>
)

target_link_libraries(tfm_config
    INTERFACE
        $<BUILD_INTERFACE:cmsis_core_header>
        $<BUILD_INTERFACE:cmsis_device_mp2_header>
        $<BUILD_INTERFACE:stm32mp2xx_hal_driver_header>
)

target_link_libraries(platform_region_defs
    INTERFACE
        dt_platform_ns_defs
)

add_subdirectory(${STM_FAMILY_DIR}/lib/qsort qsort)

target_include_directories(platform_ns
    PUBLIC
        ${CMAKE_BINARY_DIR}/generated
        ${STM_FAMILY_DIR}/native_driver/include
        ${STM_FAMILY_DIR}/cmsis_driver/include
        ${STM_FAMILY_DIR}/secure/services/include
        ${PLATFORM_NS_API_DIR}/include
        ${PLATFORM_NS_API_DIR}/ext/cmsis/Include
        ${PLATFORM_NS_API_DIR}/ext/cmsis/Include/m-profile
)

target_sources(platform_ns
    PRIVATE
        ${STM_SOC_DIR}/src/system_stm32mp2xx_m33.c
        ${DEVICETREE_DIR}/initlevel.c
        ${STM_FAMILY_DIR}/tfm_ns_plat_init.c
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
        $<$<BOOL:${TFM_PLATFORM_WDT_API}>:${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_wdt_api.c>
        $<$<BOOL:${TFM_PLATFORM_CPU_API}>:${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_cpu_api.c>
        $<$<BOOL:${TFM_PARTITION_PM}>:${STM_FAMILY_DIR}/secure/services/src/tfm_pm_api.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/secure/services/src/tfm_scmi_api.c>
        $<$<BOOL:${TFM_PARTITION_STL}>:${STM_FAMILY_DIR}/secure/services/src/tfm_stl_api.c>
)

target_compile_definitions(platform_ns
    PUBLIC
        STM32_NSEC=1
)

target_link_libraries(platform_ns
    PUBLIC
        lib_qsort
        tfm_config
        tfm_ns_log
)

# GNU ld has some strange behaviour to do with weak functions, and does not deal
# well with strong functions in static libraries overriding weak functions in
# object files. For this reason, the file tfm_ns_plat_init is on tfm_ns target
target_sources(tfm_ns
    PRIVATE
        ${STM_SOC_DIR}/src/startup_stm32mp2_ns.c
        ${DEVICETREE_DIR}/initlevel.c
        ${STM_FAMILY_DIR}/tfm_ns_plat_init.c
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
        $<$<BOOL:${TFM_PLATFORM_CPU_API}>:${STM_FAMILY_DIR}/secure/services/src/tfm_ioctl_cpu_api.c>
        $<$<BOOL:${STM32_M33TDCID}>:${STM_FAMILY_DIR}/secure/services/src/tfm_scmi_api.c>
        $<$<BOOL:${TFM_PARTITION_STL}>:${STM_FAMILY_DIR}/secure/services/src/tfm_stl_api.c>
)
