#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(CMSIS_CORE_PATH       "DOWNLOAD"                                             CACHE PATH   "Path to cmsis-core package (or DOWNLOAD to fetch automatically)")
set(CMSIS_CORE_GIT_REMOTE "https://github.com/STMicroelectronics/cmsis-core.git" CACHE STRING "URL (or path) to retrieve cmsis-core package")
set(CMSIS_CORE_VERSION    "v5.9.0"                                               CACHE STRING "version of cmsis-core package")

if (NOT EXISTS ${CMSIS_CORE_PATH})

    fetch_remote_library(
        LIB_NAME                cmsis-core
        LIB_SOURCE_PATH_VAR     CMSIS_CORE_PATH
        FETCH_CONTENT_ARGS
            GIT_REPOSITORY      ${CMSIS_CORE_GIT_REMOTE}
            GIT_TAG             ${CMSIS_CORE_VERSION}
            GIT_PROGRESS        TRUE
    )

    file(INSTALL
        ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt
        DESTINATION ${CMSIS_CORE_PATH}
    )

endif()

# to share the set of cmsis-core options settled on secure side build
# but necessary for building the non-secure side too.
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmsis_core_config.cmake.in
    ${INSTALL_CMAKE_DIR}/cmsis_core_config.cmake
    @ONLY
)

add_subdirectory(${CMSIS_CORE_PATH} cmsis-core)

