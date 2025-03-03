if (EXISTS ${STM_SOC_DIR}/check_config.cmake)
    include(${STM_SOC_DIR}/check_config.cmake)
endif()

tfm_invalid_config(NOT STM32_BOOT_DEV STREQUAL "sdmmc1")
tfm_invalid_config(TFM_PARTITION_PROTECTED_STORAGE)
