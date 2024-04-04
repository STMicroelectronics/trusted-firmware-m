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

fetch_remote_library(
    LIB_NAME                scp_fw
    LIB_SOURCE_PATH_VAR     SCP_FW_PATH
    LIB_PATCH_DIR           ${CMAKE_CURRENT_LIST_DIR}
    FETCH_CONTENT_ARGS
        GIT_REPOSITORY      ssh://gerrit.st.com:29418/mpu/oe/optee/SCP-firmware
        GIT_TAG             ${SCP_FW_VERSION}
        # ToDo: set GIT_SHALLOW to 'TRUE' when SCP_FW_VERSION will be set to a tag
        # (instead of a commit hash) with the new release.
        GIT_SHALLOW         FALSE
        GIT_PROGRESS        TRUE
        GIT_SUBMODULES      "${SCP_FW_SUBMODULES}"
)
