# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (WIN32)
    cmake_minimum_required(VERSION 3.21)
else()
    cmake_minimum_required(VERSION 3.16)
endif()
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}..3.27)

set(languages CXX)
if (APPLE)
    include(cmake/MacOSInitialSetup.cmake)
    list(APPEND languages OBJCXX)
endif ()

project(tremotesf VERSION 2.5.0 LANGUAGES ${languages})

option(TREMOTESF_QT6 "Build with Qt 6" OFF)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CommonOptions.cmake)

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Core)

set(QRC_FILES "")
if (APPLE)
    set(TREMOTESF_MACOS_BUNDLE_NAME "Tremotesf")
    set(TREMOTESF_EXTERNAL_RESOURCES_PATH "${TREMOTESF_MACOS_BUNDLE_NAME}.app/Contents/Resources")
elseif (WIN32)
    set(TREMOTESF_EXTERNAL_RESOURCES_PATH ".")
endif ()

add_subdirectory("data")
add_subdirectory("translations")
add_subdirectory("src")
add_subdirectory("packaging")
