# SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include(CTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Threads REQUIRED)
find_package(fmt 10.1.0 REQUIRED)
find_package(cxxopts 3.2.1 REQUIRED)
find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Core Network Widgets)

set(minimum_kf_version 6.11.0)
find_package(KF6WidgetsAddons ${minimum_kf_version} REQUIRED)

find_package(PkgConfig REQUIRED)

if (WIN32 OR APPLE)
    set(registrable_domain_qt ON)
else ()
    set(registrable_domain_qt OFF)
    pkg_check_modules(libpsl REQUIRED IMPORTED_TARGET "libpsl >= 0.21.2")
endif ()

add_library(tremotesf_objects OBJECT)

target_sources(tremotesf_objects
    PRIVATE
    bencodeparser.cpp
    desktoputils.cpp
    filemanagerlauncher.cpp
    fileutils.cpp
    formatutils.cpp
    magnetlinkparser.cpp

    settings.cpp
    torrentfileparser.cpp

    coroutines/coroutines.cpp
    coroutines/scope.cpp
    coroutines/waitall.cpp

    log/demangle.cpp
    log/formatters.cpp
    log/log.cpp
    rpc/addressutils.cpp
    rpc/mounteddirectoriesutils.cpp
    rpc/pathutils.cpp
    rpc/peer.cpp
    rpc/qsslcertificateformatter.cpp
    rpc/requestrouter.cpp
    rpc/rpc.cpp
    rpc/servers.cpp
    rpc/serversettings.cpp
    rpc/serverstats.cpp
    rpc/torrent.cpp
    rpc/torrentfile.cpp
    rpc/tracker.cpp

    startup/commandlineparser.cpp

    ui/itemmodels/baseproxymodel.cpp
    ui/itemmodels/basetorrentfilesmodel.cpp
    ui/itemmodels/stringlistmodel.cpp
    ui/itemmodels/torrentfilesmodelentry.cpp
    ui/itemmodels/torrentfilesproxymodel.cpp
    ui/notificationscontroller.cpp
    ui/savewindowstatedispatcher.cpp
    ui/screens/aboutdialog.cpp
    ui/screens/addtorrent/addtorrentdialog.cpp
    ui/screens/addtorrent/addtorrenthelpers.cpp
    ui/screens/addtorrent/droppedtorrents.cpp
    ui/screens/addtorrent/localtorrentfilesmodel.cpp
    ui/screens/connectionsettings/connectionsettingsdialog.cpp
    ui/screens/connectionsettings/servereditdialog.cpp
    ui/screens/connectionsettings/serversmodel.cpp
    ui/screens/mainwindow/alltrackersmodel.cpp
    ui/screens/mainwindow/basetorrentsfilterssettingsmodel.cpp
    ui/screens/mainwindow/downloaddirectoriesmodel.cpp
    ui/screens/mainwindow/downloaddirectorydelegate.cpp
    ui/screens/mainwindow/editlabelsdialog.cpp
    ui/screens/mainwindow/labelsmodel.cpp
    ui/screens/mainwindow/mainwindow.cpp
    ui/screens/mainwindow/mainwindowsidebar.cpp
    ui/screens/mainwindow/mainwindowstatusbar.cpp
    ui/screens/mainwindow/mainwindowviewmodel.cpp
    ui/screens/mainwindow/statusfiltersmodel.cpp
    ui/screens/mainwindow/torrentsmodel.cpp
    ui/screens/mainwindow/torrentsproxymodel.cpp
    ui/screens/mainwindow/torrentsview.cpp
    ui/screens/serversettings/serversettingsdialog.cpp
    ui/screens/serverstatsdialog.cpp
    ui/screens/settingsdialog.cpp
    ui/screens/torrentproperties/peersmodel.cpp
    ui/screens/torrentproperties/torrentfilesmodel.cpp
    ui/screens/torrentproperties/torrentpropertiesdialog.cpp
    ui/screens/torrentproperties/torrentpropertieswidget.cpp
    ui/screens/torrentproperties/trackersmodel.cpp
    ui/screens/torrentproperties/trackersviewwidget.cpp
    ui/stylehelpers.cpp
    ui/widgets/basetreeview.cpp

    ui/widgets/editlabelswidget.cpp
    ui/widgets/listplaceholder.cpp
    ui/widgets/progressbardelegate.cpp
    ui/widgets/remotedirectoryselectionwidget.cpp
    ui/widgets/textinputdialog.cpp
    ui/widgets/tooltipwhenelideddelegate.cpp
    ui/widgets/torrentfilesview.cpp
    ui/widgets/torrentremotedirectoryselectionwidget.cpp

    resources.qrc

    PRIVATE FILE_SET HEADERS
    FILES

    bencodeparser.h
    desktoputils.h
    filemanagerlauncher.h
    fileutils.h
    formatutils.h
    itemlistupdater.h
    magnetlinkparser.h
    pragmamacros.h
    settings.h
    stdutils.h
    target_os.h

    macoshelpers.h
    torrentfileparser.h
    unixhelpers.h
    windowshelpers.h

    coroutines/coroutinefwd.h
    coroutines/coroutines.h
    coroutines/hostinfo.h
    coroutines/network.h
    coroutines/qobjectsignal.h
    coroutines/scope.h
    coroutines/threadpool.h
    coroutines/timer.h
    coroutines/waitall.h

    ipc/ipcclient.h
    ipc/ipcserver.h

    log/demangle.h
    log/formatters.h
    log/log.h

    rpc/addressutils.h
    rpc/jsonutils.h
    rpc/mounteddirectoriesutils.h
    rpc/pathutils.h
    rpc/peer.h
    rpc/qsslcertificateformatter.h
    rpc/requestrouter.h
    rpc/rpc.h
    rpc/servers.h
    rpc/serversettings.h
    rpc/serverstats.h
    rpc/torrent.h
    rpc/torrentfile.h
    rpc/tracker.h

    startup/commandlineparser.h
    startup/signalhandler.h

    ui/iconthemesetup.h
    ui/itemmodels/baseproxymodel.h
    ui/itemmodels/basetorrentfilesmodel.h
    ui/itemmodels/modelutils.h
    ui/itemmodels/stringlistmodel.h
    ui/itemmodels/torrentfilesmodelentry.h
    ui/itemmodels/torrentfilesproxymodel.h
    ui/notificationscontroller.h
    ui/savewindowstatedispatcher.h
    ui/screens/aboutdialog.h
    ui/screens/addtorrent/addtorrentdialog.h
    ui/screens/addtorrent/addtorrenthelpers.h
    ui/screens/addtorrent/droppedtorrents.h
    ui/screens/addtorrent/localtorrentfilesmodel.h
    ui/screens/connectionsettings/connectionsettingsdialog.h
    ui/screens/connectionsettings/servereditdialog.h
    ui/screens/connectionsettings/serversmodel.h
    ui/screens/mainwindow/alltrackersmodel.h
    ui/screens/mainwindow/basetorrentsfilterssettingsmodel.h
    ui/screens/mainwindow/downloaddirectoriesmodel.h
    ui/screens/mainwindow/downloaddirectorydelegate.h
    ui/screens/mainwindow/editlabelsdialog.h
    ui/screens/mainwindow/labelsmodel.h
    ui/screens/mainwindow/mainwindow.h
    ui/screens/mainwindow/mainwindowsidebar.h
    ui/screens/mainwindow/mainwindowstatusbar.h
    ui/screens/mainwindow/mainwindowviewmodel.h
    ui/screens/mainwindow/statusfiltersmodel.h
    ui/screens/mainwindow/torrentsmodel.h
    ui/screens/mainwindow/torrentsproxymodel.h
    ui/screens/mainwindow/torrentsview.h
    ui/screens/serversettings/serversettingsdialog.h
    ui/screens/serverstatsdialog.h
    ui/screens/settingsdialog.h
    ui/screens/torrentproperties/peersmodel.h
    ui/screens/torrentproperties/torrentfilesmodel.h
    ui/screens/torrentproperties/torrentpropertiesdialog.h
    ui/screens/torrentproperties/torrentpropertieswidget.h
    ui/screens/torrentproperties/trackersmodel.h
    ui/screens/torrentproperties/trackersviewwidget.h
    ui/stylehelpers.h
    ui/systemcolorsprovider.h
    ui/widgets/basetreeview.h

    ui/widgets/editlabelswidget.h
    ui/widgets/listplaceholder.h
    ui/widgets/progressbardelegate.h
    ui/widgets/remotedirectoryselectionwidget.h
    ui/widgets/textinputdialog.h
    ui/widgets/tooltipwhenelideddelegate.h
    ui/widgets/torrentfilesview.h
    ui/widgets/torrentremotedirectoryselectionwidget.h
)

target_link_libraries(tremotesf_objects PUBLIC cxxopts::cxxopts Threads::Threads fmt::fmt Qt::Core Qt::Network Qt::Widgets KF6::WidgetsAddons)

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
    FMT_VERSION_MAJOR=${fmt_VERSION_MAJOR}
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
        find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS DBus)
        find_package(KF6WindowSystem ${minimum_kf_version} REQUIRED)
        target_link_libraries(tremotesf_objects PUBLIC Qt::DBus KF6::WindowSystem)

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
            org.freedesktop.portal.Notification.xml
            tremotesf_dbus_generated/org.freedesktop.portal.Notification
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
            ipc/ipcclient_dbus.cpp
            ipc/ipcserver_dbus.cpp
            ipc/ipcserver_dbus_service.cpp
            ui/iconthemesetup_freedesktop.cpp
            ui/notificationscontroller_freedesktop.cpp
            ${dbus_generated}

            PRIVATE FILE_SET HEADERS
            FILES
            coroutines/dbus.h
            ipc/ipcserver_dbus.h
            ipc/ipcserver_dbus_service.h
        )
    else ()
        target_sources(
            tremotesf_objects
            PRIVATE
            ipc/ipcclient_socket.cpp
            ipc/ipcserver_socket.cpp
            ui/notificationscontroller_fallback.cpp

            PRIVATE FILE_SET HEADERS
            FILES
            ipc/ipcserver_socket.h
        )
        if (APPLE)
            target_sources(
                tremotesf_objects
                PRIVATE
                macoshelpers.mm
                filemanagerlauncher_macos.mm
                ipc/fileopeneventhandler.cpp

                PRIVATE FILE_SET HEADERS
                FILES
                ipc/fileopeneventhandler.h
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
        ipc/ipcclient_socket.cpp
        ipc/ipcserver_socket.cpp
        startup/signalhandler_windows.cpp
        startup/windowsfatalerrorhandlers.cpp
        ui/darkthemeapplier_windows.cpp
        ui/notificationscontroller_fallback.cpp
        ui/systemcolorsprovider_windows.cpp
        windowshelpers.cpp

        PRIVATE FILE_SET HEADERS
        FILES
        ipc/ipcserver_socket.h
        startup/windowsfatalerrorhandlers.h
        ui/darkthemeapplier_windows.h
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
        startup/windowsmessagehandler.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/tremotesf.rc"

        PRIVATE FILE_SET HEADERS
        FILES
        startup/main_windows.h
        startup/windowsmessagehandler.h
    )
    if (MSVC)
        target_sources(tremotesf PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/tremotesf.manifest")
    else ()
        target_sources(tremotesf PRIVATE tremotesf.manifest.rc)
    endif ()
    if (MSVC)
        target_link_options(tremotesf PRIVATE "/PDBALTPATH:%_PDB%")
    endif()
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
        startup/recoloringsvgiconengineplugin.cpp
        ui/iconthemesetup_bundled.cpp
        ui/recoloringsvgiconengine.cpp

        PRIVATE FILE_SET HEADERS
        FILES
        startup/recoloringsvgiconengineplugin.h
        ui/recoloringsvgiconengine.h
    )
    target_compile_definitions(
        tremotesf
        PUBLIC
        TREMOTESF_USE_BUNDLED_QT_TRANSLATIONS
        QT_STATICPLUGIN
    )
    find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS GuiPrivate Svg)
    target_link_libraries(tremotesf PUBLIC Qt::GuiPrivate Qt::Svg)
    set(exclude_plugins bearer iconengines imageformats)
    if (WIN32)
        list(APPEND exclude_plugins styles)
    endif ()
    qt_import_plugins(tremotesf EXCLUDE_BY_TYPE ${exclude_plugins})
endif ()

if (BUILD_TESTING)
    find_package(Qt6 ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Test)

    add_executable(itemlistupdater_test itemlistupdater_test.cpp)
    add_test(NAME itemlistupdater_test COMMAND itemlistupdater_test)
    target_link_libraries(itemlistupdater_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(log_test log/log_test.cpp)
    add_test(NAME log_test COMMAND log_test)
    target_link_libraries(log_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(demangle_test log/demangle_test.cpp)
    add_test(NAME demangle_test COMMAND demangle_test)
    target_link_libraries(demangle_test PRIVATE tremotesf_objects Qt::Test)

    if (NOT TREMOTESF_WITH_HTTPLIB STREQUAL "none")
        include("${CMAKE_SOURCE_DIR}/cmake/FindCppHttplib.cmake")
        find_cpp_httplib()
        add_executable(requestrouter_test rpc/requestrouter_test.cpp)
        target_compile_definitions(requestrouter_test PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/rpc/test-data")
        add_test(NAME requestrouter_test COMMAND requestrouter_test)
        target_link_libraries(requestrouter_test PRIVATE tremotesf_objects Qt::Test)
        if (TARGET PkgConfig::httplib)
            target_link_libraries(requestrouter_test PRIVATE PkgConfig::httplib)
        else ()
            target_link_libraries(requestrouter_test PRIVATE httplib::httplib)
        endif ()
    endif()

    add_executable(pathutils_test rpc/pathutils_test.cpp)
    add_test(NAME pathutils_test COMMAND pathutils_test)
    target_link_libraries(pathutils_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(tracker_test rpc/tracker_test.cpp)
    add_test(NAME tracker_test COMMAND tracker_test)
    target_link_libraries(tracker_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(servers_test rpc/servers_test.cpp)
    add_test(NAME servers_test COMMAND servers_test)
    target_link_libraries(servers_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(servers_migration_test rpc/servers_migration_test.cpp)
    target_compile_definitions(servers_migration_test PRIVATE TEST_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/rpc/test-data")
    add_test(NAME servers_migration_test COMMAND servers_migration_test)
    target_link_libraries(servers_migration_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(coroutines_test coroutines/coroutines_test.cpp)
    add_test(NAME coroutines_test COMMAND coroutines_test)
    target_link_libraries(coroutines_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(magnetlinkparser_test magnetlinkparser_test.cpp)
    add_test(NAME magnetlinkparser_test COMMAND magnetlinkparser_test)
    target_link_libraries(magnetlinkparser_test PRIVATE tremotesf_objects Qt::Test)

    add_executable(torrentfileparser_test torrentfileparser_test.cpp)
    add_test(NAME torrentfileparser_test COMMAND torrentfileparser_test WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test-torrents")
    target_link_libraries(torrentfileparser_test PRIVATE tremotesf_objects Qt::Test)
endif ()

set_common_options_on_targets()
