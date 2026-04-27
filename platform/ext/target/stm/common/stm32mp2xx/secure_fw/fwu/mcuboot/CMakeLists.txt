#-------------------------------------------------------------------------------
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.21)

add_library(tfm_fwu_mcuboot_util INTERFACE)

if(STM32_FWU_STORAGE_ACCESS)
    target_sources(tfm_fwu_mcuboot_util
        INTERFACE
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/cmsis_driver/src/driver_spi_flash.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/cmsis_driver/src/driver_mmc_flash.c>
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/spi/stm32_ospi.c>
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/spi/stm32_omm.c>
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/mtd/spi-mem/spi_mem.c>
            $<$<STREQUAL:${STM32_BOOT_DEV},ospi>:${STM_FAMILY_DIR}/native_driver/src/mtd/nor/spi_nor.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/mmc.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/stm32_sdmmc2.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/soft_crc.c>
            $<$<OR:$<STREQUAL:${STM32_BOOT_DEV},sdmmc1>,$<STREQUAL:${STM32_BOOT_DEV},sdmmc2>>:${STM_FAMILY_DIR}/native_driver/src/mmc/partition.c>
            ${STM_FAMILY_DIR}/bl2/flash_map_bl2.c
            ${CMAKE_SOURCE_DIR}/bl2/src/flash_map.c
            ${MCUBOOT_PATH}/boot/bootutil/src/bootutil_public.c
            ${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/flash_map_extended.c
    )

    set(TFM_CONFIG_FWU_STORAGE_ACCESS 1)
else()
    set(TFM_CONFIG_FWU_STORAGE_ACCESS 0)
endif()

target_sources(tfm_fwu_mcuboot_util
    INTERFACE
        ${MCUBOOT_PATH}/boot/bootutil/src/tlv.c
        ./tfm_mcuboot_fwu.c
)

target_include_directories(tfm_fwu_mcuboot_util
    INTERFACE
        ${CMAKE_BINARY_DIR}/bl2/ext/mcuboot
        ${CMAKE_SOURCE_DIR}/secure_fw/partitions/firmware_update/bootloader/
        ${CMAKE_SOURCE_DIR}/bl2/ext/mcuboot/include
        ${MCUBOOT_PATH}/boot/bootutil/include
        ${MCUBOOT_PATH}/boot/bootutil/src
        ${MBEDCRYPTO_PATH}/include
)

target_link_libraries(tfm_fwu_mcuboot_util
    INTERFACE
        platform_region_defs
)

target_link_libraries(tfm_psa_rot_partition_fwu
    PRIVATE
        tfm_fwu_mcuboot_util
)

configure_file(${CMAKE_SOURCE_DIR}/platform/ext/target/stm/common/stm32mp2xx/secure_fw/fwu/fwu_config.h.in
               ${CMAKE_BINARY_DIR}/generated/interface/include/psa/fwu_config.h
               @ONLY)
set(FWU_DEVICE_CONFIG_FILE "${CMAKE_BINARY_DIR}/generated/interface/include/psa/fwu_config.h"
                            CACHE STRING "FWU device config header" FORCE)

target_compile_definitions(tfm_psa_rot_partition_fwu
    PRIVATE
        MCUBOOT_${MCUBOOT_UPGRADE_STRATEGY}
        $<$<STREQUAL:${MCUBOOT_UPGRADE_STRATEGY},RAM_LOAD>:MCUBOOT_BOOTUTIL_LIB_FOR_DIRECT_XIP>
        $<$<BOOL:${MCUBOOT_RAM_LOAD_REVERT}>:MCUBOOT_RAM_LOAD_REVERT>
        $<$<BOOL:${STM32_FWU_STORAGE_ACCESS}>:STM32_FWU_STORAGE_ACCESS>
    FWU_DEVICE_CONFIG_FILE="${FWU_DEVICE_CONFIG_FILE}"
)
