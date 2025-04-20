# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (DEFINED CPACK_BUILD_CONFIG)
    string(TOLOWER "${CPACK_BUILD_CONFIG}" config)
elseif (DEFINED CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" config)
else ()
    message(FATAL_ERROR "Unknown build type")
endif ()
if (config STREQUAL "release")
    message(STATUS "Stripping executable when packaging")
    set(CPACK_STRIP_FILES ON)
else()
    message(STATUS "Not stripping executable when packaging")
    set(CPACK_STRIP_FILES OFF)
endif()
