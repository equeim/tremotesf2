# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (NOT DEFINED TREMOTESF_QT6)
    message(FATAL_ERROR "TREMOTESF_QT6 is not defined")
endif()

if (TREMOTESF_QT6)
    set(TREMOTESF_QT_VERSION_MAJOR 6)
    set(TREMOTESF_MINIMUM_QT_VERSION 6.6.0)
else()
    set(TREMOTESF_QT_VERSION_MAJOR 5)
    set(TREMOTESF_MINIMUM_QT_VERSION 5.15.0)
endif()

if (UNIX AND NOT APPLE)
    set(TREMOTESF_UNIX_FREEDESKTOP ON)
else()
    set(TREMOTESF_UNIX_FREEDESKTOP OFF)
endif()

# FYI:
# if (MSVC)                                   -> MSVC or clang-cl
# if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")  -> MSVC only

if (MSVC AND (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY))
    if (VCPKG_TARGET_TRIPLET MATCHES "^[a-zA-Z0-9]+-windows-static$")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

function(append_qt_disable_deprecated_macro LIST_VAR)
    string(REPLACE "." ";" min_qt_version_components "${TREMOTESF_MINIMUM_QT_VERSION}")
    list(GET min_qt_version_components 0 major)
    list(GET min_qt_version_components 1 minor)
    list(GET min_qt_version_components 2 patch)
    math(EXPR macro_value "(${major}<<16)|(${minor}<<8)|(${patch})" OUTPUT_FORMAT HEXADECIMAL)
    if (TREMOTESF_QT6)
        list(APPEND "${LIST_VAR}" "QT_DISABLE_DEPRECATED_UP_TO=${macro_value}")
    else()
        list(APPEND "${LIST_VAR}" "QT_DISABLE_DEPRECATED_BEFORE=${macro_value}")
    endif()
    return(PROPAGATE "${LIST_VAR}")
endfunction()

function(set_common_options_on_targets)
    set(
        gcc_style_warnings
        -Wall
        -Wextra
        -Wpedantic
        -Wcast-align
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
        -Werror=format
        -Werror=non-virtual-dtor
        -Werror=return-type
    )
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND gcc_style_warnings -Wlogical-op-parentheses -Wno-gnu-zero-variadic-macro-arguments)
    else()
        list(APPEND gcc_style_warnings -Wlogical-op)
    endif()

    if (MSVC)
        set(
            common_compile_options
            /utf-8
            /permissive-
            /volatile:iso
            /Zc:__cplusplus
            /Zc:inline
        )
        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            list(
                APPEND
                common_compile_options
                /Zc:enumTypes
                /Zc:externConstexpr
                /Zc:lambda
                /Zc:preprocessor
                /Zc:throwingNew
                /diagnostics:caret
                /W4
                /w44062
                /w44165
                /w44242
                /w44254
                /w44263
                /w44264
                /w44265
                /w44287
                /w44296
                /w44355
                /w44365
                /w44388
                /w44577
                /we4774
                /we4777
                /w44800
                /w44826
                /we4905
                /we4906
                /w45204
            )
            if (TREMOTESF_ASAN)
                if (VCPKG_TARGET_TRIPLET MATCHES "^arm64.*")
                    message(WARNING "Ignoring TREMOTESF_ASAN=ON for ARM64, it is not supported")
                else()
                    list(APPEND common_compile_options /fsanitize=address /D_DISABLE_VECTOR_ANNOTATION /D_DISABLE_STRING_ANNOTATION)
                endif()
            endif()
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            list(TRANSFORM gcc_style_warnings PREPEND "/clang:")
            list(APPEND common_compile_options ${gcc_style_warnings})
            if (TREMOTESF_ASAN)
                message(WARNING "Ignoring TREMOTESF_ASAN=ON with clang-cl, it doesn't work out of the box")
            endif()
        endif()
    else()
        set(
            common_compile_options
            ${gcc_style_warnings}
        )
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18)
            list(APPEND common_compile_options -fexperimental-library)
        endif()
        if (TREMOTESF_ASAN)
            list(APPEND common_compile_options -fsanitize=address)
            list(APPEND common_link_options -fsanitize=address)
        endif()
    endif()

    if (DEFINED TREMOTESF_COMMON_COMPILE_OPTIONS)
        list(APPEND common_compile_options ${TREMOTESF_COMMON_COMPILE_OPTIONS})
    endif()

    set(common_compile_definitions QT_MESSAGELOGCONTEXT QT_STRICT_QLIST_ITERATORS)

    # QT_DISABLE_DEPRECATED_BEFORE can cause linker errors with static Qt
    get_target_property(qt_library_type Qt::Core TYPE)
    if (NOT (qt_library_type STREQUAL STATIC_LIBRARY))
        append_qt_disable_deprecated_macro(common_compile_definitions)
    endif()

    if (WIN32)
        include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/WindowsMinimumVersion.cmake")
        # Minimum supported version, 0x0A00 = Windows 10
        list(APPEND common_compile_definitions "WINVER=${TREMOTESF_WINDOWS_WINVER_MACRO}" "_WIN32_WINNT=${TREMOTESF_WINDOWS_WINVER_MACRO}")
        # Disable implicit ANSI codepage counterparts to Win32 functions dealing with strings
        list(APPEND common_compile_definitions UNICODE)
        # Slim down <windows.h>, can be undefined locally
        list(APPEND common_compile_definitions WIN32_LEAN_AND_MEAN)
        # C++/WinRT macros
        list(APPEND common_compile_definitions WINRT_LEAN_AND_MEAN WINRT_NO_MODULE_LOCK _SILENCE_CLANG_COROUTINE_MESSAGE)
    endif()

    if (TREMOTESF_UNIX_FREEDESKTOP)
        list(APPEND common_compile_definitions TREMOTESF_UNIX_FREEDESKTOP)
    endif()

    if (DEFINED TREMOTESF_COMMON_COMPILE_DEFINITIONS)
        list(APPEND common_compile_definitions ${TREMOTESF_COMMON_COMPILE_DEFINITIONS})
    endif()

    set(common_public_compile_features cxx_std_20)
    if (DEFINED TREMOTESF_COMMON_PUBLIC_COMPILE_FEATURES)
        list(APPEND common_public_compile_features ${TREMOTESF_COMMON_PUBLIC_COMPILE_FEATURES})
    endif()

    set(common_target_properties CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
    if (DEFINED TREMOTESF_COMMON_TARGET_PROPERTIES)
        list(APPEND common_target_properties ${TREMOTESF_COMMON_TARGET_PROPERTIES})
    endif()

    get_directory_property(targets BUILDSYSTEM_TARGETS)
    foreach (target ${targets})
        get_target_property(type ${target} TYPE)
        if ((type STREQUAL EXECUTABLE) OR (type STREQUAL SHARED_LIBRARY) OR (type STREQUAL STATIC_LIBRARY) OR (type STREQUAL OBJECT_LIBRARY))
            target_compile_options(${target} PRIVATE ${common_compile_options})
            target_compile_definitions(${target} PRIVATE ${common_compile_definitions})
            target_compile_features(${target} PUBLIC ${common_public_compile_features})
            target_link_options(${target} PRIVATE ${common_link_options})
            set_target_properties(${target} PROPERTIES ${common_target_properties})
        endif()
    endforeach()
endfunction()
