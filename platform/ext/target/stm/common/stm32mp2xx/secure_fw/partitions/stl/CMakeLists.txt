#-------------------------------------------------------------------------------
# Copyright (C) 2024, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if (NOT TFM_PARTITION_STL)
    return()
endif()

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)
add_library(stllib_headers INTERFACE)
find_library(STLIB NAMES stl_lib PATHS ${STM32CUBE_STL_PATH})

add_library(tfm_psa_rot_partition_stl STATIC)

target_sources(tfm_psa_rot_partition_stl
    PRIVATE
        stl_init.c
    PUBLIC
        stl_util.c
        stl_user_param_template.c
)

# The generated sources
target_sources(tfm_psa_rot_partition_stl
    PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/stl/auto_generated/intermedia_tfm_stl.c
)

target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/stl/auto_generated/load_info_tfm_stl.c
)

target_include_directories(tfm_psa_rot_partition_stl
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/stl
    PUBLIC
        ${STM32CUBE_STL_PATH}/include
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/stl
)

target_link_libraries(tfm_psa_rot_partition_stl
    PRIVATE
        platform_s
        tfm_sprt
         "-Wl,--whole-archive"
         "-Wl,--no-whole-archive"
         tfm_config
    PUBLIC
        ${STLIB}
)

target_compile_definitions(tfm_config
    INTERFACE
    TFM_PARTITION_STL
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_stl
)

############################### install ##############################
install(FILES
    ${STM32CUBE_STL_PATH}/include/stl_user_api.h
    DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/secure/services/include
)

###############################  #####################################

cmake_policy(SET CMP0079 NEW)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(ENABLE_TESTING OFF)
set(ENABLE_PROGRAMS OFF)
