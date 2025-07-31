#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(STM32CUBE_MP2_PATH                  "DOWNLOAD"              CACHE PATH      "Path to stm32cubemp2 package (or DOWNLOAD to fetch automatically)")
set(STM32CUBE_MP2_GIT_REMOTE "https://github.com/STMicroelectronics/STM32CubeMP2.git" CACHE STRING "URL (or path) to retrieve stm32cubemp2 package")
set(STM32CUBE_MP2_VERSION               "v1.2.0"                CACHE STRING    "version of stm32cubemp2 package")

if (NOT EXISTS ${STM32CUBE_MP2_PATH})

    fetch_remote_library(
        LIB_NAME                stm32cube
        LIB_SOURCE_PATH_VAR	STM32CUBE_MP2_PATH
        FETCH_CONTENT_ARGS
            GIT_REPOSITORY      ${STM32CUBE_MP2_GIT_REMOTE}
            GIT_TAG             ${STM32CUBE_MP2_VERSION}
            GIT_PROGRESS        TRUE
    )

    file(INSTALL
        ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt
        DESTINATION ${STM32CUBE_MP2_PATH}
    )

endif()

# to share the set of stm32cube options settled on secure side build
# but necessary for building the non-secure side too.
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/stm32cube_config.cmake.in
    ${INSTALL_CMAKE_DIR}/stm32cube_config.cmake
    @ONLY
)

add_subdirectory(${STM32CUBE_MP2_PATH} stm32cube)

