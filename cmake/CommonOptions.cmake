# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

set(TREMOTESF_MINIMUM_QT_VERSION 6.8.0)

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

macro(prepend_clang_prefix_for_clang_cl_options compile_options_var)
    list(TRANSFORM "${compile_options_var}" PREPEND "/clang:")
endmacro()

macro(apply_warning_options common_compile_options_var)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(
            APPEND "${common_compile_options_var}"
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
    else()
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
            if (MSVC)
                prepend_clang_prefix_for_clang_cl_options(gcc_style_warnings)
            endif()
        else()
            list(APPEND gcc_style_warnings -Wlogical-op)
        endif()
        list(APPEND "${common_compile_options_var}" ${gcc_style_warnings})
    endif()
endmacro()

macro(check_if_stdlib_is_glibcxx)
    include(CheckCXXSymbolExists)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    check_cxx_symbol_exists("__GLIBCXX__" "version" TREMOTESF_STDLIB_IS_GLIBCXX)
endmacro()

macro(check_if_stdlib_is_libcpp)
    include(CheckCXXSymbolExists)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    check_cxx_symbol_exists("_LIBCPP_VERSION" "version" TREMOTESF_STDLIB_IS_LIBCPP)
endmacro()

macro(check_if_libcpp_18_or_newer)
    include(CheckCXXSourceCompiles)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    # Can't use CMAKE_CXX_COMPILER_VERSION here because it may not correspond to the actual version of libc++ (e.g. with Apple Clang)
    check_cxx_source_compiles([=[
        #include <version>
        static_assert(_LIBCPP_VERSION >= 180000);
    ]=] TREMOTESF_LIBCPP_IS_18_OR_NEWER)
endmacro()

macro(apply_hardening_options common_compile_options_var common_compile_definitions_var)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") # Not Microsofts's cl.exe, but allowing LLVM's clang-cl.exe on Windows
        include(CheckCXXCompilerFlag)
        set(hardened_flag_to_check "-fhardened")
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND MSVC)
            prepend_clang_prefix_for_clang_cl_options(hardened_flag_to_check)
        endif()
        check_cxx_compiler_flag("${hardened_flag_to_check}" TREMOTESF_COMPILER_SUPPORTS_FHARDENED)
        unset(gcc_style_hardened_options)
        if (TREMOTESF_COMPILER_SUPPORTS_FHARDENED)
            # -fhardened sets -ftrivial-auto-var-init=zero, override it with pattern
            list(APPEND gcc_style_hardened_options -fhardened -Wno-hardened -ftrivial-auto-var-init=pattern)
        else()
            list(APPEND gcc_style_hardened_options -ftrivial-auto-var-init=pattern -fstack-protector-strong)
            if (CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
                list(APPEND gcc_style_hardened_options -fstack-clash-protection)
            endif()
            if ((VCPKG_TARGET_TRIPLET MATCHES "^x64.*") OR (NOT DEFINED VCPKG_TARGET_TRIPLET AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64"))
                list(APPEND gcc_style_hardened_options -fcf-protection=full)
            endif()
        endif()
        # -mbranch-protection is not enabled through -fhardened
        # It also causes crashes on macOS
        if (NOT APPLE AND ((VCPKG_TARGET_TRIPLET MATCHES "^arm64.*") OR (NOT DEFINED VCPKG_TARGET_TRIPLET AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|ARM64|aarch64")))
            list(APPEND gcc_style_hardened_options -mbranch-protection=standard)
        endif()
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND MSVC)
            prepend_clang_prefix_for_clang_cl_options(gcc_style_hardened_options)
        endif()
        list(APPEND "${common_compile_options_var}" ${gcc_style_hardened_options})
    endif()
    if (NOT WIN32) # Not Microsoft's libc so exclude Windows completely
        # Undefine _FORTIFY_SOURCE unconditionally to make sure it's enabled only when ASAN is disabled and build type is not Debug
        # Enable it through compile_options instead of compile_definitions so that the order of flags is correct
        # ASAN can be enabled via CXXFLAGS circumventing TREMOTESF_ASAN option, so check for that too
        list(APPEND "${common_compile_options_var}" -U_FORTIFY_SOURCE)
        if (NOT (TREMOTESF_ASAN OR CMAKE_CXX_FLAGS MATCHES "-fsanitize=address"))
            list(APPEND "${common_compile_options_var}" $<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=3>)
        endif()
    endif()
    if (NOT MSVC) # Not Microsoft's STL, but allowing libstdc++ and libc++ on Windows
        # Macros for libstdc++/libc++ hardening
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            check_if_stdlib_is_glibcxx()
            if (TREMOTESF_STDLIB_IS_GLIBCXX)
                list(APPEND "${common_compile_definitions_var}" _GLIBCXX_ASSERTIONS)
            else()
                check_if_stdlib_is_libcpp()
                if (TREMOTESF_STDLIB_IS_LIBCPP)
                    check_if_libcpp_18_or_newer()
                    if (TREMOTESF_LIBCPP_IS_18_OR_NEWER)
                        list(APPEND "${common_compile_definitions_var}" _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
                    endif()
                else()
                    message(WARNING "Unknown C++ standard library implementation")
                endif()
            endif()
        else()
            # Assuming libstdc++
            list(APPEND "${common_compile_definitions_var}" _GLIBCXX_ASSERTIONS)
        endif()
    endif()
endmacro()

macro(apply_asan_options common_compile_options_var common_compile_definitions_var common_link_options_var)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if (VCPKG_TARGET_TRIPLET MATCHES "^arm64.*")
            message(WARNING "Ignoring TREMOTESF_ASAN=ON for ARM64, it is not supported")
        else()
            list(APPEND "${common_compile_options_var}" /fsanitize=address)
            list(APPEND "${common_compile_definitions_var}" _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
        endif()
    else()
        if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            message(WARNING "Ignoring TREMOTESF_ASAN=ON with clang-cl, it doesn't work out of the box")
        else()
            list(APPEND "${common_compile_options_var}" -fsanitize=address)
            list(APPEND "${common_link_options_var}" -fsanitize=address)
        endif()
    endif()
endmacro()

# Needed for std::views::join
macro(apply_old_libcpp_workaround common_compile_options_var)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
        check_if_stdlib_is_libcpp()
        if (TREMOTESF_STDLIB_IS_LIBCPP)
            check_if_libcpp_18_or_newer()
            if (NOT TREMOTESF_LIBCPP_IS_18_OR_NEWER)
                list(APPEND "${common_compile_options_var}" -fexperimental-library)
            endif()
        endif()
    endif()
endmacro()

function(append_qt_disable_deprecated_macro common_compile_definitions_var)
    string(REPLACE "." ";" min_qt_version_components "${TREMOTESF_MINIMUM_QT_VERSION}")
    list(GET min_qt_version_components 0 major)
    list(GET min_qt_version_components 1 minor)
    list(GET min_qt_version_components 2 patch)
    math(EXPR macro_value "(${major}<<16)|(${minor}<<8)|(${patch})" OUTPUT_FORMAT HEXADECIMAL)
    list(APPEND "${common_compile_definitions_var}" "QT_DISABLE_DEPRECATED_UP_TO=${macro_value}")
    return(PROPAGATE "${common_compile_definitions_var}")
endfunction()

function(set_common_options_on_targets)
    unset(common_compile_options)
    unset(common_compile_definitions)
    unset(common_link_options)
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
            )
        endif()
    endif()

    apply_warning_options(common_compile_options)
    apply_hardening_options(common_compile_options common_compile_definitions)
    if (TREMOTESF_ASAN)
        apply_asan_options(common_compile_options common_compile_definitions common_link_options)
    endif()
    apply_old_libcpp_workaround(common_compile_options)

    list(
        APPEND common_compile_definitions
        QT_MESSAGELOGCONTEXT
        # Needed to silence a warning when using QList/QVector with ranges
        QT_STRICT_QLIST_ITERATORS
    )

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

    set(common_public_compile_features cxx_std_20)
    set(common_target_properties CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)

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
