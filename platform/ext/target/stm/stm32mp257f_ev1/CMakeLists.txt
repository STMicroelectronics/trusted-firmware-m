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
		$<$<BOOL:${STM32_DDR_CACHED}>:STM32_DDR_CACHED>
)

# Specify the location of platform specific build dependencies.
target_sources(tfm_s
	PRIVATE
		${STM_BOARD_DIR}/plat_device.c
)

if(NS)
	target_sources(tfm_ns
		PRIVATE
			${STM_BOARD_DIR}/plat_device.c
	)

	target_compile_definitions(tfm_ns
		PUBLIC
			__DOMAIN_NS=1
			DOMAIN_NS=__DOMAIN_NS
	)
endif()

install(FILES ${STM_BOARD_DIR}/include/flash_layout.h
	${STM_BOARD_DIR}/include/region_defs.h
	DESTINATION ${CMAKE_BINARY_DIR}
)
