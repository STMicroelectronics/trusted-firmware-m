# set family platform config
if (EXISTS ${STM_FAMILY_DIR}/check_config.cmake)
    include(${STM_FAMILY_DIR}/check_config.cmake)
endif()

# specific STM32MP21 restriction
