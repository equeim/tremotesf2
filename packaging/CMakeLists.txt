# SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (NOT WIN32)
    return()
endif()

configure_file("${CMAKE_SOURCE_DIR}/LICENSES/GPL-3.0-or-later.txt" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" COPYONLY)

set(CPACK_GENERATOR "ZIP" "WIX")
set(CPACK_PACKAGE_NAME "Tremotesf")
set(CPACK_PACKAGE_VENDOR "Alexey Rochev")
set(CPACK_PACKAGE_EXECUTABLES "tremotesf;Tremotesf")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Tremotesf")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
set(CPACK_THREADS 0)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPACK_SYSTEM_NAME "win64")
else()
    set(CPACK_SYSTEM_NAME "win32")
endif()

set(package_suffix "")
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT is_multi_config)
    string(LENGTH "${CMAKE_BUILD_TYPE}" build_type_length)
    if (build_type_length GREATER 0)
        set(package_suffix "-${CMAKE_BUILD_TYPE}")
    endif()
endif()

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}${package_suffix}")

set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

set(CPACK_WIX_UPGRADE_GUID "67e0511b-d9de-4b3c-a604-0dd47522d451")
set(CPACK_WIX_PRODUCT_ICON "${TREMOTESF_WINDOWS_ICON}")

include(CPack)
