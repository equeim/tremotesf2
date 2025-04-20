# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    return()
endif()

if (DEFINED VCPKG_HOST_TRIPLET)
    return()
endif()

if (NOT WIN32 AND NOT APPLE)
    message(FATAL_ERROR "Building with vcpkg is not supported on this platform")
endif()

message(STATUS "Automatically selecting host triplet")
cmake_host_system_information(RESULT platform QUERY OS_PLATFORM)
message(STATUS "Host platform is ${platform}")
if (platform MATCHES "arm64|ARM64")
    set(vcpkg_arch "arm64")
elseif (platform MATCHES "x86_64|AMD64")
    set(vcpkg_arch "x64")
else()
    message(FATAL_ERROR "Unsupported host platform '${platform}'")
endif()
if (WIN32)
    set(VCPKG_HOST_TRIPLET "${vcpkg_arch}-windows-static")
else()
    set(VCPKG_HOST_TRIPLET "${vcpkg_arch}-osx-release")
endif()
message(STATUS "Setting VCPKG_HOST_TRIPLET to ${VCPKG_HOST_TRIPLET}")
unset(vcpkg_arch)
