# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (NOT DEFINED TREMOTESF_QT6)
    message(FATAL_ERROR "TREMOTESF_QT6 is not defined")
endif()

if (TREMOTESF_QT6)
    set(TREMOTESF_QT_VERSION_MAJOR 6)
    set(TREMOTESF_MINIMUM_QT_VERSION 6.0)
else()
    set(TREMOTESF_QT_VERSION_MAJOR 5)
    set(TREMOTESF_MINIMUM_QT_VERSION 5.15)
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

function(set_common_options_on_targets)
    set(
        gcc_style_warnings
        -Wall
        -Wextra
        -Wpedantic
        -Werror=non-virtual-dtor
        -Wcast-align
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
        -Werror=format
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
                /w44623
                /we4774
                /we4777
                /w44800
                /w44826
                /we4905
                /we4906
                /w45204
            )
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            list(TRANSFORM gcc_style_warnings PREPEND "/clang:")
            list(APPEND common_compile_options ${gcc_style_warnings})
        endif()
    else()
        set(
            common_compile_options
            ${gcc_style_warnings}
        )
    endif()

    if (DEFINED TREMOTESF_COMMON_COMPILE_OPTIONS)
        list(APPEND common_compile_options ${TREMOTESF_COMMON_COMPILE_OPTIONS})
    endif()

    set(
        common_compile_definitions
        QT_DEPRECATED_WARNINGS
        QT_DISABLE_DEPRECATED_BEFORE=0x050e00
        QT_MESSAGELOGCONTEXT
    )

    if (WIN32)
        # Minimum supported version, 0x0A00 = Windows 10
        list(APPEND common_compile_definitions WINVER=0x0A00 _WIN32_WINNT=0x0A00)
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

    set(common_target_properties CXX_EXTENSIONS OFF)
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
            set_target_properties(${target} PROPERTIES ${common_target_properties})
        endif()
    endforeach()
endfunction()
