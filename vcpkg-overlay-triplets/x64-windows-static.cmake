# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include("${VCPKG_ROOT_DIR}/triplets/x64-windows-static.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/WindowsMinimumVersion.cmake")
# /await:strict - use C++20 coroutines ABI when building C++17 dependencies
set(flags "/await:strict /DWINVER=${TREMOTESF_WINDOWS_WINVER_MACRO} /D_WIN32_WINNT=${TREMOTESF_WINDOWS_WINVER_MACRO}")
set(VCPKG_CXX_FLAGS "${flags}")
set(VCPKG_C_FLAGS "${flags}")
