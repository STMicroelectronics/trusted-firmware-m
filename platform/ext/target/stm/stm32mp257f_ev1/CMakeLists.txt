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

# Specify the location of platform specific build dependencies.
target_sources(tfm_s
	PRIVATE
		${STM_BOARD_DIR}/plat_device.c
)

if(BL2)
	target_sources(bl2
		PRIVATE
			${STM_BOARD_DIR}/plat_device.c
	)
endif()

#========================= Files for building NS side platform ================#
install(FILES
    ${TARGET_PLATFORM_PATH}/config.cmake
    ${TARGET_PLATFORM_PATH}/cpuarch.cmake
    ${TARGET_PLATFORM_PATH}/plat_device.c
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

install(DIRECTORY
    ${TARGET_PLATFORM_PATH}/include
    ${TARGET_PLATFORM_PATH}/ns/
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)
