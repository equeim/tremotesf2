# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include(CTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(TREMOTESF_COMMON_TARGET_PROPERTIES AUTOMOC ON AUTORCC ON)

find_package(Threads REQUIRED)
find_package(fmt 8.0 REQUIRED)
find_package(cxxopts REQUIRED)
find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Core Concurrent Network Widgets)

if (TREMOTESF_QT6)
    set(minimum_kf_version 5.245)
else()
    set(minimum_kf_version 5.92)
endif()
find_package(KF${TREMOTESF_QT_VERSION_MAJOR}WidgetsAddons ${minimum_kf_version} REQUIRED)

find_package(PkgConfig REQUIRED)

if (WIN32 OR APPLE)
    set(registrable_domain_qt ON)
else ()
    set(registrable_domain_qt OFF)
    pkg_check_modules(libpsl REQUIRED IMPORTED_TARGET libpsl)
endif ()

list(APPEND QRC_FILES resources.qrc)

add_library(
    tremotesf_objects
    OBJECT

    bencodeparser.cpp
    bencodeparser.h
    desktoputils.cpp
    desktoputils.h
    filemanagerlauncher.cpp
    filemanagerlauncher.h
    fileutils.cpp
    fileutils.h
    formatutils.cpp
    formatutils.h
    itemlistupdater.h
    literals.h
    settings.cpp
    settings.h
    stdutils.h
    target_os.h

    unixhelpers.h
    windowshelpers.h
    macoshelpers.h

    coroutines/coroutines.cpp
    coroutines/coroutines.h
    coroutines/scope.h
    coroutines/scope.cpp
    coroutines/waitall.h
    coroutines/waitall.cpp

    ipc/ipcclient.h
    ipc/ipcserver.h

    log/log.cpp
    log/log.h
    log/formatters.cpp
    log/formatters.h
    log/demangle.cpp
    log/demangle.h

    rpc/addressutils.cpp
    rpc/addressutils.h
    rpc/mounteddirectoriesutils.cpp
    rpc/mounteddirectoriesutils.h
    rpc/servers.cpp
    rpc/servers.h
    rpc/torrent.cpp
    rpc/torrent.h
    rpc/torrentfile.cpp
    rpc/torrentfile.h
    rpc/tracker.cpp
    rpc/jsonutils.h
    rpc/tracker.h
    rpc/pathutils.cpp
    rpc/pathutils.h
    rpc/peer.cpp
    rpc/peer.h
    rpc/rpc.cpp
    rpc/rpc.h
    rpc/requestrouter.cpp
    rpc/requestrouter.h
    rpc/serversettings.cpp
    rpc/serversettings.h
    rpc/serverstats.cpp
    rpc/serverstats.h

    startup/commandlineparser.cpp
    startup/commandlineparser.h
    startup/signalhandler.h

    ui/notificationscontroller.cpp
    ui/notificationscontroller.h
    ui/savewindowstatedispatcher.cpp
    ui/savewindowstatedispatcher.h
    ui/stylehelpers.cpp
    ui/stylehelpers.h
    ui/systemcolorsprovider.h
    ui/itemmodels/baseproxymodel.cpp
    ui/itemmodels/baseproxymodel.h
    ui/itemmodels/basetorrentfilesmodel.cpp
    ui/itemmodels/basetorrentfilesmodel.h
    ui/itemmodels/modelutils.h
    ui/itemmodels/stringlistmodel.cpp
    ui/itemmodels/stringlistmodel.h
    ui/itemmodels/torrentfilesmodelentry.cpp
    ui/itemmodels/torrentfilesmodelentry.h
    ui/itemmodels/torrentfilesproxymodel.cpp
    ui/itemmodels/torrentfilesproxymodel.h
    ui/screens/aboutdialog.cpp
    ui/screens/aboutdialog.h
    ui/screens/serverstatsdialog.cpp
    ui/screens/serverstatsdialog.h
    ui/screens/settingsdialog.cpp
    ui/screens/settingsdialog.h
    ui/screens/addtorrent/addtorrentdialog.cpp
    ui/screens/addtorrent/addtorrentdialog.h
    ui/screens/addtorrent/addtorrenthelpers.cpp
    ui/screens/addtorrent/addtorrenthelpers.h
    ui/screens/addtorrent/droppedtorrents.cpp
    ui/screens/addtorrent/droppedtorrents.h
    ui/screens/addtorrent/localtorrentfilesmodel.cpp
    ui/screens/addtorrent/localtorrentfilesmodel.h
    ui/screens/connectionsettings/connectionsettingsdialog.cpp
    ui/screens/connectionsettings/connectionsettingsdialog.h
    ui/screens/connectionsettings/servereditdialog.cpp
    ui/screens/connectionsettings/servereditdialog.h
    ui/screens/connectionsettings/serversmodel.cpp
    ui/screens/connectionsettings/serversmodel.h
    ui/screens/mainwindow/alltrackersmodel.cpp
    ui/screens/mainwindow/alltrackersmodel.h
    ui/screens/mainwindow/basetorrentsfilterssettingsmodel.cpp
    ui/screens/mainwindow/basetorrentsfilterssettingsmodel.h
    ui/screens/mainwindow/downloaddirectoriesmodel.cpp
    ui/screens/mainwindow/downloaddirectoriesmodel.h
    ui/screens/mainwindow/mainwindow.cpp
    ui/screens/mainwindow/mainwindow.h
    ui/screens/mainwindow/mainwindowsidebar.cpp
    ui/screens/mainwindow/mainwindowsidebar.h
    ui/screens/mainwindow/mainwindowstatusbar.cpp
    ui/screens/mainwindow/mainwindowstatusbar.h
    ui/screens/mainwindow/mainwindowviewmodel.cpp
    ui/screens/mainwindow/mainwindowviewmodel.h
    ui/screens/mainwindow/statusfiltersmodel.cpp
    ui/screens/mainwindow/statusfiltersmodel.h
    ui/screens/mainwindow/torrentsmodel.cpp
    ui/screens/mainwindow/torrentsmodel.h
    ui/screens/mainwindow/torrentsproxymodel.cpp
    ui/screens/mainwindow/torrentsproxymodel.h
    ui/screens/mainwindow/torrentsview.cpp
    ui/screens/mainwindow/torrentsview.h
    ui/screens/serversettings/serversettingsdialog.cpp
    ui/screens/serversettings/serversettingsdialog.h
    ui/screens/torrentproperties/peersmodel.cpp
    ui/screens/torrentproperties/peersmodel.h
    ui/screens/torrentproperties/torrentfilesmodel.cpp
    ui/screens/torrentproperties/torrentfilesmodel.h
    ui/screens/torrentproperties/torrentpropertiesdialog.cpp
    ui/screens/torrentproperties/torrentpropertiesdialog.h
    ui/screens/torrentproperties/trackersmodel.cpp
    ui/screens/torrentproperties/trackersmodel.h
    ui/screens/torrentproperties/trackersviewwidget.cpp
    ui/screens/torrentproperties/trackersviewwidget.h
    ui/widgets/basetreeview.cpp
    ui/widgets/basetreeview.h
    ui/widgets/commondelegate.cpp
    ui/widgets/commondelegate.h
    ui/widgets/remotedirectoryselectionwidget.cpp
    ui/widgets/remotedirectoryselectionwidget.h
    ui/widgets/textinputdialog.cpp
    ui/widgets/textinputdialog.h
    ui/widgets/torrentfilesview.cpp
    ui/widgets/torrentfilesview.h
    ui/widgets/torrentremotedirectoryselectionwidget.cpp
    ui/widgets/torrentremotedirectoryselectionwidget.h
    ${QRC_FILES}
)

target_link_libraries(tremotesf_objects PUBLIC cxxopts::cxxopts Threads::Threads fmt::fmt Qt::Core Qt::Network Qt::Widgets KF${TREMOTESF_QT_VERSION_MAJOR}::WidgetsAddons)
target_include_directories(tremotesf_objects PRIVATE ${Qt${TREMOTESF_QT_VERSION_MAJOR}Concurrent_INCLUDE_DIRS})

# We need these as CMake variables for configure_file() call below
set(TREMOTESF_APP_NAME "Tremotesf")
set(TREMOTESF_EXECUTABLE_NAME "tremotesf")

target_compile_definitions(
    tremotesf_objects
    PUBLIC
    TREMOTESF_EXECUTABLE_NAME="${TREMOTESF_EXECUTABLE_NAME}"
    TREMOTESF_APP_ID="org.equeim.Tremotesf"
    TREMOTESF_APP_NAME="${TREMOTESF_APP_NAME}"
    TREMOTESF_VERSION="${PROJECT_VERSION}"
)
if (registrable_domain_qt)
    target_compile_definitions(tremotesf_objects PUBLIC TREMOTESF_REGISTRABLE_DOMAIN_QT)
else ()
    target_link_libraries(tremotesf_objects PUBLIC PkgConfig::libpsl)
endif ()

if (UNIX)
    target_sources(
        tremotesf_objects
        PRIVATE
        unixhelpers.cpp
        startup/signalhandler_unix.cpp
    )
    if (TREMOTESF_UNIX_FREEDESKTOP)
        find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS DBus)
        find_package(KF${TREMOTESF_QT_VERSION_MAJOR}WindowSystem ${minimum_kf_version} REQUIRED)
        target_link_libraries(tremotesf_objects PUBLIC Qt::DBus KF${TREMOTESF_QT_VERSION_MAJOR}::WindowSystem)

        qt_add_dbus_adaptor(
            dbus_generated
            ipc/org.freedesktop.Application.xml
            ipc/ipcserver_dbus_service.h
            tremotesf::IpcDbusService
            tremotesf_dbus_generated/ipc/org.freedesktop.Application.adaptor
            OrgFreedesktopApplicationAdaptor
        )
        qt_add_dbus_interface(
            dbus_generated
            ipc/org.freedesktop.Application.xml
            tremotesf_dbus_generated/ipc/org.freedesktop.Application
        )
        qt_add_dbus_interface(
            dbus_generated
            org.freedesktop.Notifications.xml
            tremotesf_dbus_generated/org.freedesktop.Notifications
        )
        qt_add_dbus_interface(
            dbus_generated
            org.freedesktop.FileManager1.xml
            tremotesf_dbus_generated/org.freedesktop.FileManager1
        )

        target_sources(
            tremotesf_objects
            PRIVATE
            filemanagerlauncher_freedesktop.cpp
            coroutines/dbus.h
            ipc/ipcclient_dbus.cpp
            ipc/ipcserver_dbus_service.cpp
            ipc/ipcserver_dbus_service.h
            ipc/ipcserver_dbus.cpp
            ipc/ipcserver_dbus.h
            ui/notificationscontroller_freedesktop.cpp
            ${dbus_generated}
        )
    else ()
        target_sources(
            tremotesf_objects
            PRIVATE
            ipc/ipcclient_socket.cpp
            ipc/ipcserver_socket.cpp
            ipc/ipcserver_socket.h
            ui/notificationscontroller_fallback.cpp
        )
        if (APPLE)
            target_sources(
                tremotesf_objects
                PRIVATE
                macoshelpers.mm
                filemanagerlauncher_macos.mm
                ipc/fileopeneventhandler.h
                ipc/fileopeneventhandler.cpp
            )
        else ()
            target_sources(
                tremotesf_objects
                PRIVATE
                filemanagerlauncher_fallback.cpp
            )
        endif ()
    endif ()
elseif (WIN32)
    find_library(Dwmapi Dwmapi REQUIRED)
    find_library(RuntimeObject RuntimeObject REQUIRED)
    target_link_libraries(tremotesf_objects PUBLIC Qt::Svg "${Dwmapi}" "${RuntimeObject}")
    target_sources(
        tremotesf_objects
        PRIVATE
        filemanagerlauncher_windows.cpp
        windowshelpers.cpp
        ipc/ipcclient_socket.cpp
        ipc/ipcserver_socket.cpp
        ipc/ipcserver_socket.h
        startup/signalhandler_windows.cpp
        startup/windowsmessagehandler.cpp
        startup/windowsmessagehandler.h
        ui/darkthemeapplier_windows.h
        ui/darkthemeapplier_windows.cpp
        ui/notificationscontroller_fallback.cpp
        ui/systemcolorsprovider_windows.cpp
    )
    set(TREMOTESF_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/tremotesf.ico" PARENT_SCOPE)
else ()
    message(FATAL_ERROR "Unsupported target platform ${CMAKE_SYSTEM_NAME}")
endif ()

add_executable(tremotesf startup/main.cpp)
target_link_libraries(tremotesf PRIVATE tremotesf_objects)

# Executable configuration on Windows
if (WIN32)
    include("${CMAKE_SOURCE_DIR}/cmake/WindowsMinimumVersion.cmake")
    configure_file(tremotesf.rc.in tremotesf.rc @ONLY)
    configure_file(tremotesf.manifest.in tremotesf.manifest @ONLY)
    target_sources(
        tremotesf
        PRIVATE
        startup/main_windows.cpp
        startup/main_windows.h
        "${CMAKE_CURRENT_BINARY_DIR}/tremotesf.rc"
    )
    if (MSVC)
        target_sources(tremotesf PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/tremotesf.manifest")
    else ()
        target_sources(tremotesf PRIVATE tremotesf.manifest.rc)
    endif ()
    set_target_properties(tremotesf PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
endif ()

# Executable installation
if (WIN32)
    set(bindir ".")
elseif (APPLE)
    set(bindir "${TREMOTESF_MACOS_BUNDLE_NAME}.app/Contents/MacOS")
else ()
    set(bindir "${CMAKE_INSTALL_BINDIR}")
endif ()
install(TARGETS tremotesf DESTINATION "${bindir}")
if (WIN32 AND MSVC)
    install(FILES $<TARGET_PDB_FILE:tremotesf> DESTINATION "${bindir}" OPTIONAL)
endif ()

# Icons and Qt translations bundling
if (WIN32 OR APPLE)
    target_sources(
        tremotesf
        PRIVATE
        ui/recoloringsvgiconengine.cpp
        ui/recoloringsvgiconengine.h
        startup/recoloringsvgiconengineplugin.cpp
        startup/recoloringsvgiconengineplugin.h
    )
    target_compile_definitions(
        tremotesf
        PUBLIC
        TREMOTESF_BUNDLED_ICON_THEME="${TREMOTESF_BUNDLED_ICON_THEME}"
        TREMOTESF_USE_BUNDLED_QT_TRANSLATIONS
        QT_STATICPLUGIN
    )
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Svg)
    target_link_libraries(tremotesf PUBLIC Qt::Svg)
    set(exclude_plugins bearer iconengines imageformats)
    if (WIN32)
        list(APPEND exclude_plugins styles)
    endif ()
    qt_import_plugins(tremotesf EXCLUDE_BY_TYPE ${exclude_plugins})
endif ()

if (BUILD_TESTING)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Test)

    add_executable(itemlistupdater_test itemlistupdater_test.cpp)
    add_test(NAME itemlistupdater_test COMMAND itemlistupdater_test)
    target_link_libraries(itemlistupdater_test tremotesf_objects Qt::Test)

    add_executable(log_test log/log_test.cpp)
    add_test(NAME log_test COMMAND log_test)
    target_link_libraries(log_test tremotesf_objects Qt::Test)

    add_executable(demangle_test log/demangle_test.cpp)
    add_test(NAME demangle_test COMMAND demangle_test)
    target_link_libraries(demangle_test tremotesf_objects Qt::Test)

    if (NOT TREMOTESF_WITH_HTTPLIB STREQUAL "none")
        include("${CMAKE_SOURCE_DIR}/cmake/FindCppHttplib.cmake")
        find_cpp_httplib()
        add_executable(requestrouter_test rpc/requestrouter_test.cpp)
        target_compile_definitions(requestrouter_test PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/rpc/test-data" CPPHTTPLIB_OPENSSL_SUPPORT)
        add_test(NAME requestrouter_test COMMAND requestrouter_test)
        target_link_libraries(requestrouter_test tremotesf_objects Qt::Test)
        if (TARGET PkgConfig::httplib)
            target_link_libraries(requestrouter_test PkgConfig::httplib)
        else ()
            target_link_libraries(requestrouter_test httplib::httplib)
        endif ()
    endif()

    add_executable(pathutils_test rpc/pathutils_test.cpp)
    add_test(NAME pathutils_test COMMAND pathutils_test)
    target_link_libraries(pathutils_test tremotesf_objects Qt::Test)

    add_executable(tracker_test rpc/tracker_test.cpp)
    add_test(NAME tracker_test COMMAND tracker_test)
    target_link_libraries(tracker_test tremotesf_objects Qt::Test)
endif ()

set_common_options_on_targets()
