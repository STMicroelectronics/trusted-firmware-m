#-------------------------------------------------------------------------------
# Copyright (C) 2024, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if (NOT TFM_PARTITION_SCP)
    return()
endif()

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)

add_library(tfm_psa_rot_partition_ipcc STATIC)

target_sources(tfm_psa_rot_partition_ipcc
  PRIVATE
        ipcc_mbox.c
)

# The generated sources
target_sources(tfm_psa_rot_partition_ipcc
  PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/ipcc/auto_generated/intermedia_tfm_ipcc.c
)
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/ipcc/auto_generated/load_info_tfm_ipcc.c
)

add_dependencies(framework dt_platform_s_gen_h)

target_include_directories(tfm_psa_rot_partition_ipcc
  PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/ipcc
        ${SCP_FW_PATH}/arch/none/tfm/include
        ${STM_SOC_DIR}/native_driver/src/clk

)
target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/ipcc
        ${STM_SOC_DIR}/native_driver/include
)

# Linking to external interfaces
target_link_libraries(tfm_psa_rot_partition_ipcc
    PRIVATE
        platform_s
        tfm_config
        tfm_sprt
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_ipcc
)

target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_IPCC
)

###############################  #####################################


cmake_policy(SET CMP0079 NEW)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(ENABLE_TESTING OFF)
set(ENABLE_PROGRAMS OFF)
#set(MBEDTLS_FATAL_WARNINGS OFF)
set(ENABLE_DOCS OFF)
