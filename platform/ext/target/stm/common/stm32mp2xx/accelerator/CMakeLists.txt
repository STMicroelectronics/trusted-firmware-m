#-------------------------------------------------------------------------------
# Copyright (C) 2025, STMicroelectronics - All Rights Reserved
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

################################ BL2 ###########################################
if(BL2)

    target_sources(bl2_crypto_hw
        PUBLIC
            ${STM_FAMILY_DIR}/accelerator/stm32mp2.c
    )

    target_include_directories(bl2_crypto_hw
        PRIVATE
            ${STM_BOARD_DIR}/include
            ${STM_FAMILY_DIR}/accelerator
    )

    target_link_libraries(bl2_crypto_hw
        PRIVATE
            bl2_mbedcrypto
            platform_bl2
            cmsis
    )

    set(STM_LIB_TARGET bl2_crypto_hw)

    target_link_libraries(bl2_mbedcrypto
        PUBLIC
            bl2_crypto_hw
            platform_bl2
            cmsis
    )
    target_include_directories(bl2_mbedcrypto
        PUBLIC
            ${STM_BOARD_DIR}/include
            ${STM_FAMILY_DIR}/accelerator
            ${STM_FAMILY_DIR}/native_driver/include
    )

    target_link_libraries(bl2_mbedcrypto
        PUBLIC
            dt_platform_bl2_defs
    )
endif()
############################ Crypto Service ####################################
if(TFM_PARTITION_CRYPTO)
    target_sources(crypto_service_crypto_hw
        PRIVATE
            ${STM_FAMILY_DIR}/accelerator/stm32mp2.c
    )

    target_include_directories(crypto_service_crypto_hw
        PRIVATE
            ${PLATFORM_DIR}/include
            ${CMAKE_BINARY_DIR}/generated
            ${CMAKE_SOURCE_DIR}/interface/include
            ${STM_FAMILY_DIR}/accelerator
    )
    target_include_directories(crypto_service_mbedcrypto
        PUBLIC
            ${PLATFORM_DIR}/include
            ${STM_BOARD_DIR}/include
            ${STM_FAMILY_DIR}/accelerator
    )

    target_include_directories(psa_crypto_config
        INTERFACE
            $<BUILD_INTERFACE:${STM_FAMILY_DIR}/accelerator/>
    )

    target_link_libraries(crypto_service_crypto_hw
        PRIVATE
            crypto_service_mbedcrypto
            platform_s
            cmsis
    )

    target_link_libraries(crypto_service_mbedcrypto
        PUBLIC
            cmsis
    )

    target_link_libraries(platform_s
        PRIVATE
            crypto_service_crypto_hw
    )

    target_link_libraries(crypto_service_crypto_hw
         INTERFACE
            tfm_config
    )
endif()
