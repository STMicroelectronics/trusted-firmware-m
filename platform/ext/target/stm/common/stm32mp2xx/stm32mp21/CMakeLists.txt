#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2024 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

#def for stm32mp2
include(${STM_FAMILLY_DIR}/CMakeLists.txt)

# specific for stm32mp21

#========================= Platform region defs ===============================#
# these flags are includes in bl2, tfm_s and tfm_ns
# target_compile_definitions(platform_region_defs
#     INTERFACE
# )

#========================= Platform Secure ====================================#
#target_sources(platform_s
#	PRIVATE
#)

#target_compile_definitions(platform_s
#	PUBLIC
#		STM32_SEC
#		STM32_LOG_LEVEL=${STM32_LOG_LEVEL}
#		STM32_PROV_FAKE=${STM32_PROV_FAKE}
#		$<$<BOOL:${TFM_PARTITION_SCP}>:CLK_MINIMAL_SZ=0>
#)

#========================= Platform BL2 =======================================#
if(BL2)

    #	target_include_directories(platform_bl2
    #		PUBLIC
    #	)

    #	target_sources(platform_bl2
    #		PRIVATE
    #	)

    #target_compile_options(platform_bl2
    #	PUBLIC
    #			${COMPILER_CMSE_FLAG}
    #	)

    #	target_compile_definitions(platform_bl2
    #		PUBLIC
    #	)

endif()
#========================= Files for building NS side platform ================#
install(FILES
    ${STM_SOC_DIR}/config.cmake
    ${STM_SOC_DIR}/cpuarch.cmake
    ${STM_SOC_DIR}/CMakeLists.txt
    ${STM_SOC_DIR}/config_tfm_stm32mp21.h
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/stm32mp21
)
