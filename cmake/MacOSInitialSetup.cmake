# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include("${CMAKE_CURRENT_LIST_DIR}/MacOSDeploymentTarget.cmake")
message(STATUS "Setting CMAKE_OSX_DEPLOYMENT_TARGET to ${TREMOTESF_MACOS_DEPLOYMENT_TARGET}")
set(CMAKE_OSX_DEPLOYMENT_TARGET "${TREMOTESF_MACOS_DEPLOYMENT_TARGET}")

if ((NOT DEFINED VCPKG_HOST_TRIPLET) AND (VCPKG_TARGET_TRIPLET MATCHES "^[a-zA-Z0-9]+-osx.*$"))
    message(STATUS "Automatically selecting host triplet on macOS")
    cmake_host_system_information(RESULT platform QUERY OS_PLATFORM)
    message(STATUS "Host platform is ${platform}")
    if (platform STREQUAL "arm64")
        set(VCPKG_HOST_TRIPLET "arm64-osx-release")
    elseif (platform STREQUAL "x86_64")
        set(VCPKG_HOST_TRIPLET "x64-osx-release")
    else ()
        message(FATAL_ERROR "Unsupported host platform '${platform}'")
    endif ()
    message(STATUS "Setting VCPKG_HOST_TRIPLET to ${VCPKG_HOST_TRIPLET}")
endif()
