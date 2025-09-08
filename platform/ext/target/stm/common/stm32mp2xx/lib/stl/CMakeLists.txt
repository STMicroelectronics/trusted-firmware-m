#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT TFM_PARTITION_STL)
    return()
endif()

set(STM32CUBE_STL_PATH        "DOWNLOAD" CACHE PATH   "Path to STL (or DOWNLOAD to fetch automatically)")

fetch_remote_library(
    LIB_NAME                stm32mp2-stl-lib
    LIB_SOURCE_PATH_VAR     STM32CUBE_STL_PATH
    LIB_PATCH_DIR           ${CMAKE_CURRENT_LIST_DIR}
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ${STM32CUBE_STL_GIT_REMOTE}
        GIT_TAG             ${STM32CUBE_STL_VERSION}
        # ToDo: set GIT_SHALLOW to 'TRUE' when STM32CUBE_STL_VERSION will be set to a tag
        # (instead of a commit hash) with the new release.
        GIT_SHALLOW         FALSE
        GIT_PROGRESS        TRUE
)
