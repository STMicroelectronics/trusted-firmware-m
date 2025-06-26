#-------------------------------------------------------------------------------
# Copyright (c) 2022-2024, Arm Limited. All rights reserved.
# Copyright (C) 2025, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if (NOT TFM_PARTITION_ADAC OR NOT TFM_PARTITION_DBGMCU)
    return()
endif()

if (NOT STM32_M33TDCID)
    return()
endif()

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)

add_library(tfm_psa_rot_partition_adac STATIC)

target_sources(tfm_psa_rot_partition_adac
    PRIVATE
        adac.c
        adac_req_mngr.c
)

# Add the generated source files
target_sources(tfm_psa_rot_partition_adac
    # The intermedia file defines the partition stack.
    PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/adac/auto_generated/intermedia_tfm_adac.c
)
target_sources(tfm_partitions
    # The load info file includes the static data of the partition.
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/adac/auto_generated/load_info_tfm_adac.c
)

# Set include directories
target_include_directories(tfm_psa_rot_partition_adac
    PUBLIC
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/adac
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add dependency to the manifest_tool
add_dependencies(tfm_psa_rot_partition_adac manifest_tool)

target_link_libraries(tfm_psa_rot_partition_adac
    PRIVATE
        platform_s
        tfm_sprt
        trusted-firmware-m-psa-adac
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_adac
)

target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_ADAC
)
