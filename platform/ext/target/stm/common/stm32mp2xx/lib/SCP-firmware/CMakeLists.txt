#-------------------------------------------------------------------------------
# Copyright (c) 2024 STMicroelectronics. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Because of https://gitlab.kitware.com/cmake/cmake/-/issues/20579 CMP0097 is
# non-functional until cmake 3.18.0.
if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
    # Workaround for not having CMP0097. Use existing directory.
    set(SCP_FW_SUBMODULES "docs")
endif()

set(SCP_FW_GIT_REMOTE "https://github.com/STMicroelectronics/SCP-firmware" CACHE STRING "Git remote to fetch the SCP firmware from")
set(SCP_FW_VERSION    "v2.13.0-stm32mp-r3"                                 CACHE STRING "The version of SCP-firmware to use")
set(SCP_FW_PATH       "DOWNLOAD"                                           CACHE PATH   "Path to SCP-firmware or DOWNLOAD to fetch automatically")

fetch_remote_library(
    LIB_NAME                scp_fw
    LIB_SOURCE_PATH_VAR     SCP_FW_PATH
    LIB_PATCH_DIR           ${CMAKE_CURRENT_LIST_DIR}
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ${SCP_FW_GIT_REMOTE}
        GIT_TAG             ${SCP_FW_VERSION}
        # ToDo: set GIT_SHALLOW to 'TRUE' when SCP_FW_VERSION will be set to a tag
        # (instead of a commit hash) with the new release.
        GIT_SHALLOW         FALSE
        GIT_PROGRESS        TRUE
        GIT_SUBMODULES      "${SCP_FW_SUBMODULES}"
)
