# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

# Using macros instead of functions since find_package can set variable that we need to propagate to parent scope

macro(find_system_httplib)
    # httplib breaks backwards compatibility on each minor version (i.e. seconds component).
    # With CMake we need to make separate find_package() calls for each version,
    # with pkg-config we can specify range
    set(httplib_supported_versions 0.17 0.16 0.15 0.14 0.13 0.12 0.11)
    set(httplib_next_unsupported_version 0.18)
    foreach (version ${httplib_supported_versions})
        message(STATUS "Trying cpp-httplib ${version} as a CMake package")
        find_package(httplib "${version}" QUIET)
        if (httplib_FOUND)
            break()
        endif ()
    endforeach ()
    if (httplib_FOUND)
        message(STATUS "Found cpp-httplib ${httplib_VERSION} as a CMake package")
    else ()
        message(STATUS "Did not find cpp-httplib as a CMake package")
        list(GET httplib_supported_versions -1 oldest_supported_version)
        set(module "cpp-httplib >= ${oldest_supported_version}")
        message(STATUS "Trying ${module} using pkg-config")
        pkg_check_modules(httplib IMPORTED_TARGET "${module}" QUIET)
        unset(module)
        if (httplib_FOUND)
            message(STATUS "Found cpp-httplib ${httplib_VERSION} using pkg-config")
            if (httplib_VERSION VERSION_GREATER_EQUAL httplib_next_unsupported_version)
                message(WARNING "cpp-httplib version ${httplib_VERSION} is not supported. Compilation or tests may fail")
            endif ()
        else ()
            message(STATUS "Did not find cpp-httplib using pkg-config")
        endif ()
    endif ()
    unset(httplib_supported_versions)
    unset(httplib_next_unsupported_version)
endmacro()

macro(include_bundled_httplib)
    set(HTTPLIB_REQUIRE_OPENSSL ON)
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
