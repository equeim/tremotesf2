# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

# Using macros instead of functions since find_package can set variable that we need to propagate to parent scope

macro(find_system_httplib)
    # httplib breaks backwards compatibility on each minor version (i.e. second component)
    # Because of that we can't pass minimum version to find_package
    message(STATUS "Trying cpp-httplib as a CMake package")
    find_package(httplib)
    if (httplib_FOUND)
        message(STATUS "Found cpp-httplib ${httplib_VERSION} as a CMake package")
    else ()
        message(STATUS "Did not find cpp-httplib as a CMake package")
        message(STATUS "Trying cpp-httplib using pkg-config")
        pkg_check_modules(httplib IMPORTED_TARGET "cpp-httplib >= 0.11")
        unset(module)
        if (httplib_FOUND)
            message(STATUS "Found cpp-httplib ${httplib_VERSION} using pkg-config")
        else ()
            message(STATUS "Did not find cpp-httplib using pkg-config")
        endif ()
    endif ()
endmacro()

macro(include_bundled_httplib)
    if (NOT WIN32)
        set(HTTPLIB_REQUIRE_OPENSSL ON)
    endif()
    add_subdirectory(3rdparty/cpp-httplib EXCLUDE_FROM_ALL)
endmacro()

macro(find_cpp_httplib)
    if (TREMOTESF_WITH_HTTPLIB STREQUAL "auto")
        find_system_httplib()
        if (NOT httplib_FOUND)
            message(WARNING "Using bundled cpp-httplib")
            set(TREMOTESF_WITH_HTTPLIB "bundled" CACHE STRING "" FORCE)
            include_bundled_httplib()
        endif()
    elseif (TREMOTESF_WITH_HTTPLIB STREQUAL "system")
        find_system_httplib()
    elseif (TREMOTESF_WITH_HTTPLIB STREQUAL "bundled")
        message(STATUS "Using bundled cpp-httplib")
        include_bundled_httplib()
    else()
        # We shouldn't actually get here
        message(FATAL_ERROR "Invalid TREMOTESF_WITH_HTTPLIB value ${TREMOTESF_WITH_HTTPLIB}")
    endif()
endmacro()
