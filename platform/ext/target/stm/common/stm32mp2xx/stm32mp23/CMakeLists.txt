#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
# Copyright (c) 2024 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_policy(SET CMP0076 NEW)

#def for stm32mp2
include(${STM_FAMILY_DIR}/CMakeLists.txt)

# specific for stm32mp23
if (NOT ${STM32_STM32MP23_SOC_REV} STREQUAL "revY" AND
    NOT ${STM32_STM32MP23_SOC_REV} STREQUAL "revX")
    Message(FATAL_ERROR "STM32_STM32MP23_SOC_REV=\"${STM32_STM32MP23_SOC_REV}\" not supported.")
endif()

#========================= tfm config ===============================#
target_compile_definitions(tfm_config
    INTERFACE
        $<$<STREQUAL:${STM32_STM32MP23_SOC_REV},revY>:CONFIG_STM32MP25X_REVY=1>
        $<$<STREQUAL:${STM32_STM32MP23_SOC_REV},revX>:CONFIG_STM32MP25X_REVX=1>
)

#========================= Platform region defs ===============================#
# these flags are includes in bl2, tfm_s and tfm_ns
target_compile_definitions(platform_region_defs
	INTERFACE
		RELOCATE_DATA
)

#========================= Platform Secure ====================================#
target_sources(platform_s
	PRIVATE
		${STM_SOC_DIR}/src/system_stm32mp2xx_m33.c
		${STM_FAMILY_DIR}/native_driver/src/clk/clk-stm32mp25.c
)

target_sources(tfm_s
	PRIVATE
		${STM_SOC_DIR}/src/startup_stm32mp2_s.c
)

#========================= Platform BL2 =======================================#
if(BL2)

	target_sources(platform_bl2
		PRIVATE
			${STM_SOC_DIR}/src/system_stm32mp2xx_m33.c
			${STM_FAMILY_DIR}/native_driver/src/clk/clk-stm32mp25.c
	)

	target_sources(bl2
		PRIVATE
			${STM_SOC_DIR}/src/startup_stm32mp2_bl2.c
	)

endif()

#========================= Files for building NS side platform ================#
install(FILES
	${STM_SOC_DIR}/config.cmake
	${STM_SOC_DIR}/cpuarch.cmake
	${STM_SOC_DIR}/CMakeLists.txt
	${STM_SOC_DIR}/config_tfm_soc.h
	${STM_SOC_DIR}/soc_config.h
	DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/stm32mp23
)

install(DIRECTORY
	${STM_SOC_DIR}/ns
	DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/stm32mp23
)

install(FILES
	${STM_SOC_DIR}/src/startup_stm32mp2_ns.c
	${STM_SOC_DIR}/src/system_stm32mp2xx_m33.c
	DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/stm32mp2xx/stm32mp23/src
)
