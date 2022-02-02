set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Concurrent Network)

set(TREMOTESF_QT6 OFF CACHE BOOL "Build with Qt 6" FORCE)
include(libtremotesf/cmake/CommonOptions.cmake)
add_subdirectory(libtremotesf)

list(APPEND QRC_FILES resources.qrc)

add_executable(tremotesf
    3rdparty/cxxopts.hpp
    alltrackersmodel.cpp
    alltrackersmodel.h
    baseproxymodel.cpp
    baseproxymodel.h
    basetorrentfilesmodel.cpp
    basetorrentfilesmodel.h
    basetorrentsfilterssettingsmodel.cpp
    basetorrentsfilterssettingsmodel.h
    bencodeparser.cpp
    bencodeparser.h
    commandlineparser.cpp
    commandlineparser.h
    downloaddirectoriesmodel.cpp
    downloaddirectoriesmodel.h
    ipcclient.h
    ipcserver.h
    localtorrentfilesmodel.cpp
    localtorrentfilesmodel.h
    main.cpp
    modelutils.h
    peersmodel.cpp
    peersmodel.h
    servers.cpp
    servers.h
    serversmodel.cpp
    serversmodel.h
    settings.cpp
    settings.h
    signalhandler.cpp
    signalhandler.h
    statusfiltersmodel.cpp
    statusfiltersmodel.h
    stringlistmodel.cpp
    stringlistmodel.h
    torrentfilesmodel.cpp
    torrentfilesmodel.h
    torrentfilesmodelentry.cpp
    torrentfilesmodelentry.h
    torrentfilesproxymodel.cpp
    torrentfilesproxymodel.h
    torrentsmodel.cpp
    torrentsmodel.h
    torrentsproxymodel.cpp
    torrentsproxymodel.h
    trackersmodel.cpp
    trackersmodel.h
    trpc.cpp
    trpc.h
    utils.cpp
    utils.h
    ${QRC_FILES}
)

set_target_properties(tremotesf PROPERTIES OUTPUT_NAME "${TREMOTESF_NAME}")
target_link_libraries(tremotesf Qt5::Network libtremotesf)
target_include_directories(tremotesf PRIVATE ${Qt5Concurrent_INCLUDE_DIRS})
target_compile_definitions(
    tremotesf PRIVATE
    TREMOTESF_VERSION="${PROJECT_VERSION}"
    TREMOTESF_QT_TRANSLATIONS_FILENAME="${TREMOTESF_QT_TRANSLATIONS_FILENAME}"
)

if (TREMOTESF_SAILFISHOS)
    find_package(Qt5 REQUIRED COMPONENTS Quick)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SailfishApp REQUIRED IMPORTED_TARGET sailfishapp)

    target_sources(tremotesf PRIVATE
        sailfishos/directorycontentmodel.cpp
        sailfishos/directorycontentmodel.h
        sailfishos/sailfishosutils.cpp
        sailfishos/sailfishosutils.h
        sailfishos/selectionmodel.cpp
        sailfishos/selectionmodel.h
    )
    target_link_libraries(tremotesf Qt5::Quick PkgConfig::SailfishApp)
    if (CMAKE_VERSION VERSION_LESS "3.15.0")
        target_link_libraries(tremotesf ${SailfishApp_LDFLAGS_OTHER})
    endif()
    target_compile_definitions(tremotesf PRIVATE TREMOTESF_SAILFISHOS)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    find_package(KF5WidgetsAddons REQUIRED)

    target_link_libraries(tremotesf Qt5::Widgets KF5::WidgetsAddons)

    set_property(SOURCE utils.cpp PROPERTY SKIP_AUTOMOC ON)

    target_sources(tremotesf PRIVATE
        desktop/aboutdialog.cpp
        desktop/aboutdialog.h
        desktop/addtorrentdialog.cpp
        desktop/addtorrentdialog.h
        desktop/basetreeview.cpp
        desktop/basetreeview.h
        desktop/commondelegate.cpp
        desktop/commondelegate.h
        desktop/connectionsettingsdialog.cpp
        desktop/connectionsettingsdialog.h
        desktop/fileselectionwidget.cpp
        desktop/fileselectionwidget.h
        desktop/desktoputils.cpp
        desktop/desktoputils.h
        desktop/mainwindow.cpp
        desktop/mainwindow.h
        desktop/mainwindowsidebar.cpp
        desktop/mainwindowsidebar.h
        desktop/mainwindowstatusbar.cpp
        desktop/mainwindowstatusbar.h
        desktop/remotedirectoryselectionwidget.cpp
        desktop/remotedirectoryselectionwidget.h
        desktop/servereditdialog.cpp
        desktop/servereditdialog.h
        desktop/serversettingsdialog.cpp
        desktop/serversettingsdialog.h
        desktop/serverstatsdialog.cpp
        desktop/serverstatsdialog.h
        desktop/settingsdialog.cpp
        desktop/settingsdialog.h
        desktop/textinputdialog.cpp
        desktop/textinputdialog.h
        desktop/torrentfilesview.cpp
        desktop/torrentfilesview.h
        desktop/torrentpropertiesdialog.cpp
        desktop/torrentpropertiesdialog.h
        desktop/torrentsview.cpp
        desktop/torrentsview.h
        desktop/trackersviewwidget.cpp
        desktop/trackersviewwidget.h
    )

    target_compile_definitions(tremotesf PRIVATE
        TREMOTESF_APP_ID="org.equeim.Tremotesf"
        TREMOTESF_APP_NAME="Tremotesf"
    )

    if (WIN32)
        target_sources(
            tremotesf
            PRIVATE
            desktop/tremotesf.rc
            desktop/tremotesf.manifest
        )
        target_compile_definitions(tremotesf PRIVATE
            TREMOTESF_BUNDLED_ICONS_DIR="${TREMOTESF_BUNDLED_ICONS_DIR}"
            TREMOTESF_BUNDLED_ICON_THEME="${TREMOTESF_BUNDLED_ICON_THEME}"
            TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR="${TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR}"
        )
        set_target_properties(tremotesf PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
    endif()

    if (Qt5_VERSION VERSION_GREATER_EQUAL 5.14.0)
        qt5_import_plugins(tremotesf
            EXCLUDE_BY_TYPE imageformats
            EXCLUDE_BY_TYPE iconengines
            INCLUDE Qt::QSvgIconPlugin
        )
    endif()
endif()

if (UNIX AND NOT APPLE)
    find_package(Qt5 REQUIRED COMPONENTS DBus)
    target_link_libraries(tremotesf Qt5::DBus)

    if (NOT TREMOTESF_SAILFISHOS)
        find_package(Qt5 REQUIRED COMPONENTS X11Extras)
        find_package(KF5WindowSystem REQUIRED)
        target_link_libraries(tremotesf Qt5::X11Extras KF5::WindowSystem)
    endif()

    set_source_files_properties(org.freedesktop.Application.xml PROPERTIES CLASSNAME IpcDbusInterface NO_NAMESPACE ON)
    qt5_add_dbus_adaptor(dbus_generated org.freedesktop.Application.xml ipcserver_dbus_service.h tremotesf::IpcDbusService ipcserver_dbus_service_adaptor IpcDbusServiceAdaptor)
    qt5_add_dbus_interface(dbus_generated org.freedesktop.Application.xml ipcclient_dbus_interface)

    set_source_files_properties(org.equeim.Tremotesf.xml PROPERTIES CLASSNAME IpcDbusInterfaceDeprecated NO_NAMESPACE ON)
    qt5_add_dbus_adaptor(dbus_generated org.equeim.Tremotesf.xml ipcserver_dbus_service.h tremotesf::IpcDbusService ipcserver_dbus_service_deprecated_adaptor IpcDbusServiceDeprecatedAdaptor)
    qt5_add_dbus_interface(dbus_generated org.equeim.Tremotesf.xml ipcclient_dbus_interface_deprecated)

    set_source_files_properties(
        desktop/org.freedesktop.Notifications.xml
        desktop/org.freedesktop.FileManager1.xml
        desktop/org.xfce.FileManager.xml
        PROPERTIES NO_NAMESPACE ON
    )
    qt5_add_dbus_interface(dbus_generated desktop/org.freedesktop.Notifications.xml org.freedesktop.Notifications)
    qt5_add_dbus_interface(dbus_generated desktop/org.freedesktop.FileManager1.xml org.freedesktop.FileManager1)
    qt5_add_dbus_interface(dbus_generated desktop/org.xfce.FileManager.xml org.xfce.FileManager)

    if (Qt5_VERSION VERSION_LESS 5.9.2)
        set_source_files_properties(${dbus_generated} PROPERTIES SKIP_AUTOMOC ON)
    endif()

    target_sources(tremotesf PRIVATE ipcserver_dbus.cpp ipcserver_dbus_service.cpp ipcclient_dbus.cpp ${dbus_generated})
else()
    target_sources(tremotesf PRIVATE ipcserver_socket.cpp ipcclient_socket.cpp)
endif()

set_common_options_on_targets()

if (WIN32)
    install(TARGETS tremotesf DESTINATION ".")
else()
    install(TARGETS tremotesf DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
