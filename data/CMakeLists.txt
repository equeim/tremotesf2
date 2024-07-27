# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

list(
    APPEND QRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/icons/status/status.qrc"
)

if (TREMOTESF_UNIX_FREEDESKTOP)
    find_package(Gettext 0.21 REQUIRED)

    install(DIRECTORY "icons/hicolor" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons" PATTERN ".DS_Store" EXCLUDE)

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
elseif (WIN32 OR APPLE)
    message(STATUS "Building for Windows or macOS, deploying icon themes")

    set(TREMOTESF_BUNDLED_ICON_THEME "breeze")
    set(TREMOTESF_BUNDLED_ICON_THEME ${TREMOTESF_BUNDLED_ICON_THEME} PARENT_SCOPE)

    install(DIRECTORY "icons/hicolor" DESTINATION "${TREMOTESF_EXTERNAL_RESOURCES_PATH}/icons" PATTERN ".DS_Store" EXCLUDE)

    include(FetchContent)
    cmake_policy(SET CMP0135 NEW)
    set(FETCHCONTENT_QUIET OFF)

    FetchContent_Declare(
        hicolor_icon_theme
        URL "https://gitlab.freedesktop.org/xdg/default-icon-theme/-/archive/0.17/default-icon-theme-0.17.tar.gz"
        URL_HASH SHA256=3d3a762121c2e78e37ab98221ad48e4596a716abf3bf4475f13e372328f86cd1
    )
    FetchContent_Populate(hicolor_icon_theme)
    install(FILES "${hicolor_icon_theme_SOURCE_DIR}/index.theme" DESTINATION "${TREMOTESF_EXTERNAL_RESOURCES_PATH}/icons/hicolor")

    FetchContent_Declare(
        breeze_icons
        URL "https://download.kde.org/stable/frameworks/6.1/breeze-icons-6.1.0.tar.xz"
        URL_HASH SHA256=b1ba802d0d0dbd0ecd786cbdfaaf545f2dd3c217966b894b9321a2a18426a16a
    )
    FetchContent_Populate(breeze_icons)
    install(CODE "set(breeze_icons_SOURCE_DIR \"${breeze_icons_SOURCE_DIR}\")")
    install(CODE "set(TREMOTESF_EXTERNAL_RESOURCES_PATH \"${TREMOTESF_EXTERNAL_RESOURCES_PATH}\")")
    install(SCRIPT InstallBreezeIcons.cmake)
endif()

if (APPLE)
    install(FILES icons/macos/tremotesf.icns DESTINATION "${TREMOTESF_EXTERNAL_RESOURCES_PATH}")
endif ()

set(QRC_FILES ${QRC_FILES} PARENT_SCOPE)
