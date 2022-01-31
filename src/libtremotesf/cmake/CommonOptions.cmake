if (NOT DEFINED TREMOTESF_QT6)
    message(FATAL_ERROR "TREMOTESF_QT6 is not defined")
endif()

if (TREMOTESF_QT6)
    set(TREMOTESF_QT_VERSION_MAJOR 6)
else()
    set(TREMOTESF_QT_VERSION_MAJOR 5)
endif()

if (MSVC AND (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY))
    if (VCPKG_TARGET_TRIPLET MATCHES "^[a-zA-Z0-9]+-windows-static$")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

function(set_common_options_on_targets)
    if (MSVC)
        set(common_compile_options /W4)
    else()
        set(
            common_compile_options
            -Wall
            -Wextra
            -Wpedantic
            -Wnon-virtual-dtor
            -Wcast-align
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wdouble-promotion
            -Wformat=2
            -Werror=format
        )
        if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            list(APPEND common_compile_options -Wlogical-op)
        endif()
    endif()

    if (DEFINED TREMOTESF_COMMON_COMPILE_OPTIONS)
        list(APPEND common_compile_options ${TREMOTESF_COMMON_COMPILE_OPTIONS})
    endif()

    set(
        common_compile_definitions
        QT_DEPRECATED_WARNINGS
        QT_DISABLE_DEPRECATED_BEFORE=0x050e00
    )

    if (NOT DEFINED TREMOTESF_SAILFISHOS)
        message(FATAL_ERROR "TREMOTESF_SAILFISHOS is not defined")
    endif()

    if (NOT TREMOTESF_SAILFISHOS)
        list(APPEND common_compile_definitions QT_MESSAGELOGCONTEXT)
    endif()

    if (DEFINED TREMOTESF_COMMON_COMPILE_DEFINITIONS)
        list(APPEND common_compile_definitions ${TREMOTESF_COMMON_COMPILE_DEFINITIONS})
    endif()

    set(common_public_compile_features cxx_std_17)
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
