# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

list(
    APPEND QRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/icons/status/status.qrc"
)

if (TREMOTESF_UNIX_FREEDESKTOP)
    find_package(Gettext 0.19.7 REQUIRED)

    install(DIRECTORY "icons/hicolor" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons")

    set(po_dir "${CMAKE_CURRENT_SOURCE_DIR}/po")

    set(desktop_file_template "${CMAKE_CURRENT_SOURCE_DIR}/org.equeim.Tremotesf.desktop.in")
    set(desktop_file_path "${CMAKE_CURRENT_BINARY_DIR}/org.equeim.Tremotesf.desktop")
    add_custom_command(
        OUTPUT "${desktop_file_path}"
        COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
        ARGS --desktop -d "${po_dir}" --template "${desktop_file_template}" -o "${desktop_file_path}"
        DEPENDS "${desktop_file_template}"
        VERBATIM
    )
    add_custom_target(desktop_file ALL DEPENDS "${desktop_file_path}")
    install(FILES "${desktop_file_path}" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

    set(appdata_template "${CMAKE_CURRENT_SOURCE_DIR}/org.equeim.Tremotesf.appdata.xml.in")
    set(appdata_path "${CMAKE_CURRENT_BINARY_DIR}/org.equeim.Tremotesf.appdata.xml")
    add_custom_command(
        OUTPUT "${appdata_path}"
        COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
        ARGS --xml -d "${po_dir}" --template "${appdata_template}" -o "${appdata_path}"
        DEPENDS "${appdata_template}"
        VERBATIM
    )
    add_custom_target(appdata ALL DEPENDS "${appdata_path}")
    install(FILES "${appdata_path}" DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
elseif (WIN32)
    set(TREMOTESF_BUNDLED_ICONS_DIR "icons")
    set(TREMOTESF_BUNDLED_ICONS_DIR ${TREMOTESF_BUNDLED_ICONS_DIR} PARENT_SCOPE)

    set(TREMOTESF_BUNDLED_ICON_THEME "breeze")
    set(TREMOTESF_BUNDLED_ICON_THEME ${TREMOTESF_BUNDLED_ICON_THEME} PARENT_SCOPE)

    install(DIRECTORY "icons/hicolor" DESTINATION "${TREMOTESF_BUNDLED_ICONS_DIR}")

    if (WIN32)
        message(STATUS "Building for Windows, deploying icon themes")

        include(FetchContent)

        cmake_policy(SET CMP0135 NEW)
        set(FETCHCONTENT_QUIET OFF)

        FetchContent_Declare(
            hicolor_icon_theme
            URL "https://icon-theme.freedesktop.org/releases/hicolor-icon-theme-0.17.tar.xz"
            URL_HASH SHA256=317484352271d18cbbcfac3868eab798d67fff1b8402e740baa6ff41d588a9d8
        )
        FetchContent_Populate(hicolor_icon_theme)
        install(FILES "${hicolor_icon_theme_SOURCE_DIR}/index.theme" DESTINATION "${TREMOTESF_BUNDLED_ICONS_DIR}/hicolor")

        FetchContent_Declare(
            breeze_icons
            URL "https://download.kde.org/stable/frameworks/5.104/breeze-icons-5.104.0.zip"
            URL_HASH SHA256=b4ea76a5737ca48abd61e0cee2c54cceb8bf3d3bd6f5d15f1f740c5c7432e60c
        )
        FetchContent_Populate(breeze_icons)

        # Keep in sync with QIcon::fromTheme() calls in source code
        set(bundled_icon_theme_files
            index.theme
            application-exit.svg
            applications-utilities.svg
            configure.svg
            dialog-cancel.svg
            dialog-error.svg
            document-open.svg
            document-preview.svg
            document-properties.svg
            download.svg
            edit-copy.svg
            edit-delete.svg
            edit-delete.svg
            edit-rename.svg
            edit-select-all.svg
            edit-select-invert.svg
            folder-download.svg
            go-bottom.svg
            go-down.svg
            go-jump.svg
            go-top.svg
            go-up.svg
            help-about.svg
            insert-link.svg
            list-add.svg
            list-remove.svg
            mark-location.svg
            media-playback-pause.svg
            media-playback-start.svg
            network-connect.svg
            network-disconnect.svg
            network-server.svg
            network-server.svg
            preferences-system.svg
            preferences-system-network.svg
            preferences-system-time.svg
            system-shutdown.svg
            view-refresh.svg
            view-refresh.svg
            view-statistics.svg
            window-close.svg
        )

        list(JOIN bundled_icon_theme_files "|" bundled_icon_theme_files_regex)
        string(REPLACE "." "\\." bundled_icon_theme_files_regex "${bundled_icon_theme_files_regex}")
        string(PREPEND bundled_icon_theme_files_regex "^.*\\/(")
        string(APPEND bundled_icon_theme_files_regex ")$")

        install(DIRECTORY "${breeze_icons_SOURCE_DIR}/icons/" DESTINATION "${TREMOTESF_BUNDLED_ICONS_DIR}/breeze" FILES_MATCHING REGEX "${bundled_icon_theme_files_regex}")
    endif()
endif()

set(QRC_FILES ${QRC_FILES} PARENT_SCOPE)
