list(
    APPEND QRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/authors/authors.qrc"
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

    if (DEFINED VCPKG_TARGET_TRIPLET)
        message(STATUS "Building with vcpkg, attempting to deploy icon themes")

        set(hicolor_theme_name "hicolor")

        set(hicolor_dir "")
        set(bundled_icon_theme_dir "")

        set(found_hicolor OFF)
        set(found_bundled OFF)
        foreach(prefix ${CMAKE_PREFIX_PATH})
            set(tmp_hicolor_dir "${prefix}/${CMAKE_INSTALL_DATADIR}/icons/${hicolor_theme_name}")
            if ((NOT found_hicolor) AND (EXISTS "${tmp_hicolor_dir}"))
                set(found_hicolor ON)
                set(hicolor_dir "${tmp_hicolor_dir}")
            endif()
            set(tmp_bundled_icon_theme_dir "${prefix}/${CMAKE_INSTALL_DATADIR}/icons/${TREMOTESF_BUNDLED_ICON_THEME}")
            if ((NOT found_bundled) AND (EXISTS "${tmp_bundled_icon_theme_dir}"))
                set(found_bundled ON)
                set(bundled_icon_theme_dir "${tmp_bundled_icon_theme_dir}")
            endif()
            if (found_hicolor AND found_bundled)
                break()
            endif()
        endforeach()

        if (NOT found_hicolor)
            message(FATAL_ERROR "hicolor icon theme not found")
        endif()
        message(STATUS "Deploying hicolor icon theme from ${hicolor_dir}")

        if (NOT found_bundled)
            message(FATAL_ERROR "${TREMOTESF_BUNDLED_ICON_THEME} icon theme not found")
        endif()
        message(STATUS "Deploying ${TREMOTESF_BUNDLED_ICON_THEME} icon theme from ${bundled_icon_theme_dir}")

        install(FILES "${hicolor_dir}/index.theme" DESTINATION "${TREMOTESF_BUNDLED_ICONS_DIR}/${hicolor_theme_name}")

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
        )

        list(JOIN bundled_icon_theme_files "|" bundled_icon_theme_files_regex)
        string(REPLACE "." "\\." bundled_icon_theme_files_regex "${bundled_icon_theme_files_regex}")
        string(PREPEND bundled_icon_theme_files_regex "^.*\\/(")
        string(APPEND bundled_icon_theme_files_regex ")$")

        install(DIRECTORY "${bundled_icon_theme_dir}" DESTINATION "${TREMOTESF_BUNDLED_ICONS_DIR}" FILES_MATCHING REGEX "${bundled_icon_theme_files_regex}")
    endif()
endif()

set(QRC_FILES ${QRC_FILES} PARENT_SCOPE)
