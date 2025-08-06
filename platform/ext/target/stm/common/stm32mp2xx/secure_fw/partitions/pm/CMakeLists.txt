#-------------------------------------------------------------------------------
# Copyright (C) 2025, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#-------------------------------------------------------------------------------
if (NOT TFM_PARTITION_PM)
    return()
endif()

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0079 NEW)

add_subdirectory(${STM_FAMILY_DIR}/lib/stm32mp2-low-power stm32mp2_low_power)

########################### LP Partition ###################################
add_library(tfm_psa_rot_partition_pm STATIC)

target_sources(tfm_psa_rot_partition_pm
  PRIVATE
        pm_init.c
	device.c
	pm.c
)

# The generated sources
target_sources(tfm_psa_rot_partition_pm
  PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/pm/auto_generated/intermedia_tfm_pm.c
)
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/pm/auto_generated/load_info_tfm_pm.c
)

target_include_directories(tfm_psa_rot_partition_pm
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/pm
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/pm
)

target_link_libraries(tfm_psa_rot_partition_pm
    PRIVATE
        platform_s
        tfm_config
	tfm_sprt
)

set(TFM_LP_FW "${STM32MP2_LP_OUT_DIR}/${STM32MP2_LP_OUT_BIN}.o")

add_custom_target(tfm_lp_fw ALL
    DEPENDS ${STM32MP2_LP_TARGET}
    DEPENDS ${TFM_LP_FW}
)

add_custom_command(
    OUTPUT ${TFM_LP_FW}
    DEPENDS $<TARGET_FILE_DIR:${STM32MP2_LP_TARGET}>/${STM32MP2_LP_OUT_BIN}
    COMMAND ${CMAKE_OBJCOPY} -I binary -O elf32-littlearm
        ${STM32MP2_LP_OUT_DIR}/${STM32MP2_LP_OUT_BIN}
        ${TFM_LP_FW}
    COMMENT "generate low power firmware object included in tfm (${STM32MP2_LP_OUT_BIN}.o)"
)

add_dependencies(tfm_s_scatter tfm_lp_fw)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_pm
)

target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_PM
        TFM_LP_FW=${TFM_LP_FW}
        $<$<BOOL:${CONFIG_PM_DEVICE}>:CONFIG_PM_DEVICE>
)
