if (SAILFISHOS)
    set(app_icons "sailfishos/hicolor")
    set(desktop_file_path "sailfishos/harbour-tremotesf.desktop")
    install(FILES "sailfishos/cover.svg" DESTINATION "${DATA_PATH}")
else()
    set(app_icons "desktop/hicolor")

    if (TEST_BUILD)
        set(icons_path "${CMAKE_CURRENT_SOURCE_DIR}/desktop/status")
    else()
        set(icons_path "${DATA_PATH}/icons")
        if (NOT WIN32)
            install(DIRECTORY "desktop/status/" DESTINATION "${icons_path}")

            find_package(Gettext 0.19.7 REQUIRED)

            set(po_dir "${CMAKE_CURRENT_SOURCE_DIR}/desktop/po")

            set(desktop_file_template "${CMAKE_CURRENT_SOURCE_DIR}/desktop/org.equeim.Tremotesf.desktop.in")
            set(desktop_file_path "${CMAKE_CURRENT_BINARY_DIR}/org.equeim.Tremotesf.desktop")
            add_custom_command(
                OUTPUT "${desktop_file_path}"
                COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
                ARGS --desktop -d "${po_dir}" --template "${desktop_file_template}" -o "${desktop_file_path}"
                DEPENDS "${desktop_file_template}"
                VERBATIM
            )
            add_custom_target(desktop_file ALL DEPENDS "${desktop_file_path}")

            set(appdata_template "${CMAKE_CURRENT_SOURCE_DIR}/desktop/org.equeim.Tremotesf.appdata.xml.in")
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
        endif()
    endif()

    set(ICONS_PATH "${icons_path}" PARENT_SCOPE)
endif()

if (NOT TEST_BUILD AND NOT WIN32)
    install(DIRECTORY "${app_icons}" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons")
    install(FILES "${desktop_file_path}" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()
