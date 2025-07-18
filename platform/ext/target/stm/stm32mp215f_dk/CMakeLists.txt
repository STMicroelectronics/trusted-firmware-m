#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

#def for stm32mp2
include(${STM_SOC_DIR}/CMakeLists.txt)
#========================= Platform defs ===============================#
target_compile_definitions(platform_region_defs
    INTERFACE
        $<$<BOOL:${STM32_CACHE_ENABLED}>:STM32_CACHE_ENABLED>
        STM32MP_DDR_DUAL_AXI_PORT=1
)

#========================= Files for building NS side platform ================#
install(FILES
    ${TARGET_PLATFORM_PATH}/config.cmake
    ${TARGET_PLATFORM_PATH}/cpuarch.cmake
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

install(DIRECTORY
    ${TARGET_PLATFORM_PATH}/include
    ${TARGET_PLATFORM_PATH}/ns/
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)
