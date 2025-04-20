# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.25)
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}...3.31)

set(languages CXX)
if (APPLE)
    include(cmake/MacOSInitialSetup.cmake)
    list(APPEND languages OBJCXX)
endif ()

project(tremotesf VERSION 2.8.2 LANGUAGES ${languages})

option(TREMOTESF_ASAN "Build with AddressSanitizer" OFF)

set(TREMOTESF_WITH_HTTPLIB "auto" CACHE STRING "Where to find cpp-httplib dependency for unit tests. Possible values are: auto, system, bundled, none")
if (NOT TREMOTESF_WITH_HTTPLIB MATCHES "^(auto|system|bundled|none)$")
    message(FATAL_ERROR "Invalid TREMOTESF_WITH_HTTPLIB value ${TREMOTESF_WITH_HTTPLIB}. Possible values are: auto, system, bundled, none")
endif()

include(CTest)
include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CommonOptions.cmake)

find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Core)

if (APPLE)
    set(TREMOTESF_MACOS_BUNDLE_NAME "Tremotesf")
    set(TREMOTESF_EXTERNAL_RESOURCES_PATH "${TREMOTESF_MACOS_BUNDLE_NAME}.app/Contents/Resources")
elseif (WIN32)
    set(TREMOTESF_EXTERNAL_RESOURCES_PATH ".")
endif ()

add_subdirectory("src")
add_subdirectory("data")
add_subdirectory("translations")
add_subdirectory("packaging")
