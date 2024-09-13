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

add_library(tfm_psa_rot_partition_scp STATIC)

target_sources(tfm_psa_rot_partition_scp
  PRIVATE
        scp_init.c
        scmi_server.c
        scmi_server_scpfw.c
        scp_mbox.c
  )

# The generated sources
target_sources(tfm_psa_rot_partition_scp
  PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/scp/auto_generated/intermedia_tfm_scp.c
)
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/scp/auto_generated/load_info_tfm_scp.c
)

set(SCP_LOG_LEVEL DISABLED)
set(SCP_ARCHITECTURE tfm)
set(SCP_ENABLE_OVERRIDE_FIRMWARE_NAME OFF)
set(SCP_FIRMWARE scmi-fw)
set(SCP_FIRMWARE_SOURCE_DIR ${SCP_FW_PATH}/product/tfm-stm32mp2/fw)
set(SCP_FIRMWARE_BINARY_DIR ${CMAKE_BINARY_DIR}/scp)
set(SCP_FIRMWARE_TARGET scmi-fw)
set(SCP_FIRMWARE_SOURCE_ scmi-fw)
set(SCP_TFM_DIR ${TFM_OS_PATH})
set(CFG_CROSS_COMPILE arm-none-eabi)
set(DISABLE_CPPCHECK ON)
set(TARGET_FILE_DIR ${SCP_FW_PATH})
set(CMAKE_SOURCE_DIR ${SCP_SOURCE_DIR})
set(SCP_TFM_DIR ${STM_DIR}/../../../../)
set(SCP_TFM_PLAT_DIR ${STM_SOC_DIR})
add_subdirectory(${SCP_FW_PATH} scmi-fw)
add_dependencies(scmi-fw dt_platform_s_gen_h)

target_compile_definitions(scmi-fw
   INTERFACE
   CFG_SCPFW_MOD_TFM_CLOCK
   CFG_SCPFW_MOD_TFM_SMT
   CFG_SCPFW_MOD_TFM_RESET
   CFG_SCPFW_MOD_TFM_PSU_REGULATOR
   CFG_SCPFW_MOD_RESET_DOMAIN
   CFG_SCPFW_MOD_SCMI
   CFG_SCPFW_MOD_SCMI_CLOCK
   CFG_SCPFW_MOD_SCMI_RESET_DOMAIN
   CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN
   CFG_SCPFW_MOD_STM32_REGU_CONSUMER
   CFG_SCPFW_MOD_VOLTAGE_DOMAIN
   CFG_SCPFW_MOD_CLOCK
   CFG_SCPFW_MOD_TFM_CONSOLE
   CFG_SCPFW_MOD_MSG_SMT
)
add_dependencies(framework dt_platform_s_gen_h)

target_compile_definitions(framework
   INTERFACE
   CFG_SCPFW_MOD_TFM_CLOCK
   CFG_SCPFW_MOD_TFM_MBX
   CFG_SCPFW_MOD_TFM_SMT
   CFG_SCPFW_MOD_TFM_PSU_REGULATOR
   CFG_SCPFW_MOD_RESET_DOMAIN
   CFG_SCPFW_MOD_SCMI
   CFG_SCPFW_MOD_SCMI_CLOCK
   CFG_SCPFW_MOD_SCMI_RESET_DOMAIN
   CFG_SCPFW_MOD_SCMI_VOLTAGE_DOMAIN
   CFG_SCPFW_MOD_STM32_REGU_CONSUMER
   CFG_SCPFW_MOD_VOLTAGE_DOMAIN
   CFG_SCPFW_MOD_CLOCK
   CFG_SCPFW_MOD_TFM_CONSOLE
   CFG_SCPFW_MOD_MSG_SMT
)

target_include_directories(tfm_psa_rot_partition_scp
  PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/scp
        ${SCP_FW_PATH}/arch/none/tfm/include
        ${STM_SOC_DIR}/native_driver/src/clk

)
target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/partitions/scp
        ${STM_SOC_DIR}/native_driver/include

)

# Linking to external interfaces
target_link_libraries(tfm_psa_rot_partition_scp
    PRIVATE
        platform_s
        tfm_config
        tfm_sprt
        scmi-fw
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_scp
)

target_compile_definitions(tfm_partition_defs
    INTERFACE
    TFM_PARTITION_SCP
)

###############################  #####################################

add_library(scp_service_scpfirmware_config INTERFACE)

target_compile_definitions(scp_service_scpfirmware_config
    INTERFACE
        SCP_CONFIG_FILE="${TFM_SCP_CONFIG_PATH}"
)

target_link_libraries(scp_service_scpfirmware_config
    INTERFACE
        tfm_config
)

cmake_policy(SET CMP0079 NEW)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(ENABLE_TESTING OFF)
set(ENABLE_PROGRAMS OFF)
#set(MBEDTLS_FATAL_WARNINGS OFF)
set(ENABLE_DOCS OFF)
