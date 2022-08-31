add_subdirectory(libtremotesf)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(TREMOTESF_COMMON_TARGET_PROPERTIES AUTOMOC ON AUTORCC ON)

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Concurrent Network Widgets)
find_package(KF5WidgetsAddons REQUIRED)

list(APPEND QRC_FILES tremotesf/resources.qrc)

add_executable(
    tremotesf

    3rdparty/cxxopts.hpp
    tremotesf/bencodeparser.cpp
    tremotesf/bencodeparser.h
    tremotesf/desktoputils.cpp
    tremotesf/desktoputils.h
    tremotesf/settings.cpp
    tremotesf/settings.h
    tremotesf/utils.cpp
    tremotesf/utils.h
    tremotesf/ipc/ipcclient.h
    tremotesf/ipc/ipcserver.h
    tremotesf/rpc/servers.cpp
    tremotesf/rpc/servers.h
    tremotesf/rpc/trpc.cpp
    tremotesf/rpc/trpc.h
    tremotesf/startup/commandlineparser.cpp
    tremotesf/startup/commandlineparser.h
    tremotesf/startup/main.cpp
    tremotesf/startup/signalhandler.cpp
    tremotesf/startup/signalhandler.h
    tremotesf/ui/darkthemeapplier.h
    tremotesf/ui/systemcolorsprovider.h
    tremotesf/ui/itemmodels/baseproxymodel.cpp
    tremotesf/ui/itemmodels/baseproxymodel.h
    tremotesf/ui/itemmodels/basetorrentfilesmodel.cpp
    tremotesf/ui/itemmodels/basetorrentfilesmodel.h
    tremotesf/ui/itemmodels/modelutils.h
    tremotesf/ui/itemmodels/stringlistmodel.cpp
    tremotesf/ui/itemmodels/stringlistmodel.h
    tremotesf/ui/itemmodels/torrentfilesmodelentry.cpp
    tremotesf/ui/itemmodels/torrentfilesmodelentry.h
    tremotesf/ui/itemmodels/torrentfilesproxymodel.cpp
    tremotesf/ui/itemmodels/torrentfilesproxymodel.h
    tremotesf/ui/screens/aboutdialog.cpp
    tremotesf/ui/screens/aboutdialog.h
    tremotesf/ui/screens/serverstatsdialog.cpp
    tremotesf/ui/screens/serverstatsdialog.h
    tremotesf/ui/screens/settingsdialog.cpp
    tremotesf/ui/screens/settingsdialog.h
    tremotesf/ui/screens/addtorrent/addtorrentdialog.cpp
    tremotesf/ui/screens/addtorrent/addtorrentdialog.h
    tremotesf/ui/screens/addtorrent/localtorrentfilesmodel.cpp
    tremotesf/ui/screens/addtorrent/localtorrentfilesmodel.h
    tremotesf/ui/screens/connectionsettings/connectionsettingsdialog.cpp
    tremotesf/ui/screens/connectionsettings/connectionsettingsdialog.h
    tremotesf/ui/screens/connectionsettings/servereditdialog.cpp
    tremotesf/ui/screens/connectionsettings/servereditdialog.h
    tremotesf/ui/screens/connectionsettings/serversmodel.cpp
    tremotesf/ui/screens/connectionsettings/serversmodel.h
    tremotesf/ui/screens/mainwindow/alltrackersmodel.cpp
    tremotesf/ui/screens/mainwindow/alltrackersmodel.h
    tremotesf/ui/screens/mainwindow/basetorrentsfilterssettingsmodel.cpp
    tremotesf/ui/screens/mainwindow/basetorrentsfilterssettingsmodel.h
    tremotesf/ui/screens/mainwindow/downloaddirectoriesmodel.cpp
    tremotesf/ui/screens/mainwindow/downloaddirectoriesmodel.h
    tremotesf/ui/screens/mainwindow/mainwindow.cpp
    tremotesf/ui/screens/mainwindow/mainwindow.h
    tremotesf/ui/screens/mainwindow/mainwindowsidebar.cpp
    tremotesf/ui/screens/mainwindow/mainwindowsidebar.h
    tremotesf/ui/screens/mainwindow/mainwindowstatusbar.cpp
    tremotesf/ui/screens/mainwindow/mainwindowstatusbar.h
    tremotesf/ui/screens/mainwindow/statusfiltersmodel.cpp
    tremotesf/ui/screens/mainwindow/statusfiltersmodel.h
    tremotesf/ui/screens/mainwindow/torrentsmodel.cpp
    tremotesf/ui/screens/mainwindow/torrentsmodel.h
    tremotesf/ui/screens/mainwindow/torrentsproxymodel.cpp
    tremotesf/ui/screens/mainwindow/torrentsproxymodel.h
    tremotesf/ui/screens/mainwindow/torrentsview.cpp
    tremotesf/ui/screens/mainwindow/torrentsview.h
    tremotesf/ui/screens/serversettings/serversettingsdialog.cpp
    tremotesf/ui/screens/serversettings/serversettingsdialog.h
    tremotesf/ui/screens/torrentproperties/peersmodel.cpp
    tremotesf/ui/screens/torrentproperties/peersmodel.h
    tremotesf/ui/screens/torrentproperties/torrentfilesmodel.cpp
    tremotesf/ui/screens/torrentproperties/torrentfilesmodel.h
    tremotesf/ui/screens/torrentproperties/torrentpropertiesdialog.cpp
    tremotesf/ui/screens/torrentproperties/torrentpropertiesdialog.h
    tremotesf/ui/screens/torrentproperties/trackersmodel.cpp
    tremotesf/ui/screens/torrentproperties/trackersmodel.h
    tremotesf/ui/screens/torrentproperties/trackersviewwidget.cpp
    tremotesf/ui/screens/torrentproperties/trackersviewwidget.h
    tremotesf/ui/widgets/basetreeview.cpp
    tremotesf/ui/widgets/basetreeview.h
    tremotesf/ui/widgets/commondelegate.cpp
    tremotesf/ui/widgets/commondelegate.h
    tremotesf/ui/widgets/fileselectionwidget.cpp
    tremotesf/ui/widgets/fileselectionwidget.h
    tremotesf/ui/widgets/remotedirectoryselectionwidget.cpp
    tremotesf/ui/widgets/remotedirectoryselectionwidget.h
    tremotesf/ui/widgets/textinputdialog.cpp
    tremotesf/ui/widgets/textinputdialog.h
    tremotesf/ui/widgets/torrentfilesview.cpp
    tremotesf/ui/widgets/torrentfilesview.h
    ${QRC_FILES}
)

target_link_libraries(tremotesf PRIVATE libtremotesf Qt::Network Qt::Widgets KF5::WidgetsAddons)
target_include_directories(tremotesf PRIVATE ${Qt${TREMOTESF_QT_VERSION_MAJOR}Concurrent_INCLUDE_DIRS})
target_compile_definitions(
    tremotesf PRIVATE
    TREMOTESF_APP_ID="org.equeim.Tremotesf"
    TREMOTESF_APP_NAME="Tremotesf"
    TREMOTESF_VERSION="${PROJECT_VERSION}"
    TREMOTESF_QT_TRANSLATIONS_FILENAME="${TREMOTESF_QT_TRANSLATIONS_FILENAME}"
)

if (WIN32)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Svg)
    target_link_libraries(tremotesf PRIVATE Qt::Svg)
    target_sources(
        tremotesf PRIVATE
        tremotesf/startup/main_windows.cpp
        tremotesf/startup/main_windows.h
        tremotesf/ui/darkthemeapplier_windows.cpp
        tremotesf/ui/recoloringsvgiconengine.cpp
        tremotesf/ui/recoloringsvgiconengine.h
        tremotesf/ui/systemcolorsprovider_windows.cpp
        tremotesf/tremotesf.rc
        tremotesf/tremotesf.manifest
    )
    set(TREMOTESF_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/tremotesf/tremotesf.ico" PARENT_SCOPE)
    target_compile_definitions(tremotesf PRIVATE
        TREMOTESF_BUNDLED_ICONS_DIR="${TREMOTESF_BUNDLED_ICONS_DIR}"
        TREMOTESF_BUNDLED_ICON_THEME="${TREMOTESF_BUNDLED_ICON_THEME}"
        TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR="${TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR}"
        QT_STATICPLUGIN
    )
    set_target_properties(tremotesf PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)

    qt_import_plugins(
        tremotesf
        EXCLUDE_BY_TYPE bearer iconengines imageformats styles
    )
else()
    target_compile_definitions(
        tremotesf PRIVATE
        TREMOTESF_BUNDLED_ICONS_DIR=""
        TREMOTESF_BUNDLED_ICON_THEME=""
        TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR=""
    )
    target_sources(
        tremotesf PRIVATE
        tremotesf/ui/darkthemeapplier_noop.cpp
        tremotesf/ui/systemcolorsprovider_noop.cpp
    )
endif()

if (TREMOTESF_UNIX_FREEDESKTOP)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS DBus X11Extras)
    find_package(KF5WindowSystem REQUIRED)
    target_link_libraries(tremotesf PRIVATE Qt::DBus Qt::X11Extras KF5::WindowSystem)

    qt_add_dbus_adaptor(
        dbus_generated
        tremotesf/ipc/org.freedesktop.Application.xml
        tremotesf/ipc/ipcserver_dbus_service.h
        tremotesf::IpcDbusService
        tremotesf_dbus_generated/ipc/org.freedesktop.Application.adaptor
        OrgFreedesktopApplicationAdaptor
    )
    qt_add_dbus_interface(
        dbus_generated
        tremotesf/ipc/org.freedesktop.Application.xml
        tremotesf_dbus_generated/ipc/org.freedesktop.Application
    )
    qt_add_dbus_interface(
        dbus_generated
        tremotesf/org.freedesktop.Notifications.xml
        tremotesf_dbus_generated/org.freedesktop.Notifications
    )
    qt_add_dbus_interface(
        dbus_generated
        tremotesf/org.freedesktop.FileManager1.xml
        tremotesf_dbus_generated/org.freedesktop.FileManager1
    )

    target_sources(
        tremotesf PRIVATE
        tremotesf/ipc/ipcclient_dbus.cpp
        tremotesf/ipc/ipcserver_dbus_service.cpp
        tremotesf/ipc/ipcserver_dbus_service.h
        tremotesf/ipc/ipcserver_dbus.cpp
        tremotesf/ipc/ipcserver_dbus.h
        ${dbus_generated}
    )
else()
    target_sources(
        tremotesf PRIVATE
        tremotesf/ipc/ipcclient_socket.cpp
        tremotesf/ipc/ipcserver_socket.cpp
        tremotesf/ipc/ipcserver_socket.h
    )
endif()

if (WIN32)
    install(TARGETS tremotesf DESTINATION ".")
    if (MSVC)
        install(FILES $<TARGET_PDB_FILE:tremotesf> DESTINATION "." CONFIGURATIONS Debug)
    endif()
else()
    install(TARGETS tremotesf DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

set_common_options_on_targets()
