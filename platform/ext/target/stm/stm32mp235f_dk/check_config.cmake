if (EXISTS ${STM_SOC_DIR}/check_config.cmake)
    include(${STM_SOC_DIR}/check_config.cmake)
endif()

if (${STM32_M33TDCID})
    tfm_invalid_config(STM32_BOOT_DEV STREQUAL "ospi")
endif()
