# set familly platform config
if (EXISTS ${STM_FAMILLY_DIR}/check_config.cmake)
	include(${STM_FAMILLY_DIR}/check_config.cmake)
endif()

# specif STM32MP25 restriction
tfm_invalid_config(STM32_STM32MP25_SOC_REV STREQUAL "revA")
