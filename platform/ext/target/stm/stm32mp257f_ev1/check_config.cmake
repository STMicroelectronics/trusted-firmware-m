if (EXISTS ${STM_SOC_DIR}/check_config.cmake)
	include(${STM_SOC_DIR}/check_config.cmake)
endif()

#M33TDCID is not yet supported
tfm_invalid_config(STM32_M33TDCID)
