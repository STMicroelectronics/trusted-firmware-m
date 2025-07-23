#-------------------------------------------------------------------------------
# Copyright (C) 2025, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
# Allow linking to things 'upwards' in the directory tree (in this case bl2 / tfm_partition_crypto)
cmake_policy(SET CMP0079 NEW)
# Allow relative paths
cmake_policy(SET CMP0076 NEW)

if(NOT DEFINED PLATFORM_DIR)
    set(PLATFORM_DIR ../../.. CACHE PATH "Path to plaform dir")
endif()

# mbedtls_accelerator_config.h is included from the profile config files and must
# be visible in the platform_s always, due to the compilation of provisioning.c
target_include_directories(platform_s
    PUBLIC
        ${STM_FAMILY_DIR}/accelerator
)

################################ BL2 ###########################################
if(BL2)
    target_compile_definitions(platform_bl2
        PRIVATE
            CRYPTO_HW_ACCELERATOR
            LEGACY_DRIVER_API_ENABLED
    )

    target_include_directories(platform_bl2
        PUBLIC
            ${STM_FAMILY_DIR}/accelerator
    )

    set(STM_LIB_TARGET bl2_crypto_hw)

    target_compile_definitions(bl2_mbedcrypto
        PUBLIC
            CRYPTO_HW_ACCELERATOR
            LEGACY_DRIVER_API_ENABLED
    )

    target_compile_options(bl2_mbedcrypto
        PRIVATE
            $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
            $<$<C_COMPILER_ID:ARMClang>:-Wno-unused-parameter>
    )
endif()
include(${STM_FAMILY_DIR}/accelerator/CMakeLists.txt)

if (TFM_PARTITION_CRYPTO)
    set(STM_LIB_TARGET crypto_service_crypto_hw)

    target_link_libraries(crypto_service_mbedcrypto
        PUBLIC
            crypto_service_crypto_hw
    )

    target_compile_definitions(crypto_service_mbedcrypto
        PUBLIC
            CRYPTO_HW_ACCELERATOR
            LEGACY_DRIVER_API_ENABLED
    )

    target_compile_options(crypto_service_mbedcrypto
        PRIVATE
            $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
            $<$<C_COMPILER_ID:ARMClang>:-Wno-unused-parameter>
    )
endif()
