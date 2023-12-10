# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include("${VCPKG_ROOT_DIR}/triplets/community/x64-osx-release.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/MacOSDeploymentTarget.cmake")
set(VCPKG_OSX_DEPLOYMENT_TARGET "${TREMOTESF_MACOS_DEPLOYMENT_TARGET}")
set(VCPKG_CXX_FLAGS -g)
set(VCPKG_C_FLAGS -g)
