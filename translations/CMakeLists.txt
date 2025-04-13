# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS LinguistTools)

qt_add_lupdate(
    TS_FILES source.ts
    SOURCE_TARGETS tremotesf_objects tremotesf
)

qt_add_lrelease(
    TS_FILES
        de.ts
        en.ts
        es.ts
        es_ES.ts
        fr.ts
        it_IT.ts
        nl_BE.ts
        nl.ts
        pl.ts
        ru.ts
        tr.ts
        zh_CN.ts
    QM_FILES_OUTPUT_VARIABLE qm_files
)

qt_add_resources(
    tremotesf_objects "translations"
    PREFIX "/translations"
    FILES ${qm_files}
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
)

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
