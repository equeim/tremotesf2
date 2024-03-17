# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS LinguistTools)

add_custom_target(
    update_source_ts
    COMMAND $<TARGET_FILE:Qt::lupdate> "${CMAKE_SOURCE_DIR}/src" -no-obsolete -ts source.ts
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(
    ts_files
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

qt_add_translation(qm_files ${ts_files})
add_custom_target(translations ALL DEPENDS ${qm_files})

set(translations_qrc_content "<RCC>\n<qresource prefix=\"/translations\">")
foreach(qm_file IN LISTS qm_files)
    string(FIND "${qm_file}" "/" last_separator_index REVERSE)
    if (last_separator_index EQUAL -1)
        message(FATAL_ERROR "Did not find last separator in path ${qm_file}")
    endif()
    string(SUBSTRING "${qm_file}" "${last_separator_index}" -1 qm_file)
    string(SUBSTRING "${qm_file}" 1 -1 qm_file)
    string(APPEND translations_qrc_content "\n<file><![CDATA[${qm_file}]]></file>")
endforeach()
string(APPEND translations_qrc_content "\n</qresource>\n</RCC>")

set(translations_qrc "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
file(WRITE "${translations_qrc}" "${translations_qrc_content}")

list(APPEND QRC_FILES "${translations_qrc}")
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
