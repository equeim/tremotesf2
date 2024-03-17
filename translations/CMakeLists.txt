# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS LinguistTools)

add_custom_target(
    update_source_ts
    COMMAND $<TARGET_FILE:Qt::lupdate> "${CMAKE_SOURCE_DIR}/src" -ts source.ts
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Make sure to also change translations.qrc
qt_add_translation(qm_files
    de.ts
    en.ts
    es.ts
    fr.ts
    it_IT.ts
    nl_BE.ts
    nl.ts
    pl.ts
    ru.ts
    tr.ts
    zh_CN.ts
)
add_custom_target(translations ALL DEPENDS ${qm_files})

configure_file(translations.qrc "${CMAKE_BINARY_DIR}" COPYONLY)
list(APPEND QRC_FILES "${CMAKE_BINARY_DIR}/translations.qrc")
set(QRC_FILES ${QRC_FILES} PARENT_SCOPE)

if (WIN32 OR APPLE)
    message(STATUS "Building for Windows or macOS, deploying Qt translations")

    if (DEFINED VCPKG_TARGET_TRIPLET)
        # vcpkg
        set(relative_qt_translations_dir "translations/Qt6")
    else ()
        # MSYS2
        set(relative_qt_translations_dir "share/qt6/translations")
    endif ()
    if (DEFINED QT_HOST_PATH)
        find_file(qt_translations_dir "${relative_qt_translations_dir}" PATHS "${QT_HOST_PATH}" REQUIRED)
    else ()
        find_file(qt_translations_dir "${relative_qt_translations_dir}" REQUIRED)
    endif ()
    message(STATUS "Deploying Qt translations from ${qt_translations_dir}")
    install(DIRECTORY "${qt_translations_dir}/" DESTINATION "${TREMOTESF_EXTERNAL_RESOURCES_PATH}/qt-translations"
        FILES_MATCHING PATTERN "qt_*.qm" PATTERN "qtbase_*.qm" PATTERN "qtmultimedia_*.qm"
        PATTERN "qt_help*" EXCLUDE)
endif ()
