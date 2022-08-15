add_subdirectory(libtremotesf)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Concurrent Network)

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

target_link_libraries(tremotesf Qt::Network libtremotesf)
target_include_directories(tremotesf PRIVATE ${Qt${TREMOTESF_QT_VERSION_MAJOR}Concurrent_INCLUDE_DIRS})
target_compile_definitions(
    tremotesf PRIVATE
    TREMOTESF_APP_ID="org.equeim.Tremotesf"
    TREMOTESF_APP_NAME="Tremotesf"
    TREMOTESF_VERSION="${PROJECT_VERSION}"
    TREMOTESF_QT_TRANSLATIONS_FILENAME="${TREMOTESF_QT_TRANSLATIONS_FILENAME}"
)

# Leave the if for now
if (1)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Widgets)
    find_package(KF5WidgetsAddons REQUIRED)

    target_link_libraries(tremotesf Qt::Widgets KF5::WidgetsAddons)

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
        desktop/darkthemeapplier.h
        desktop/desktoputils.cpp
        desktop/desktoputils.h
        desktop/fileselectionwidget.cpp
        desktop/fileselectionwidget.h
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
        desktop/systemcolorsprovider.h
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

    if (WIN32)
        target_sources(
            tremotesf
            PRIVATE
            desktop/darkthemeapplier_windows.cpp
            desktop/systemcolorsprovider_windows.cpp
            desktop/tremotesf.rc
            desktop/tremotesf.manifest
        )
        set(TREMOTESF_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/desktop/tremotesf.ico" PARENT_SCOPE)
        target_compile_definitions(tremotesf PRIVATE
            TREMOTESF_BUNDLED_ICONS_DIR="${TREMOTESF_BUNDLED_ICONS_DIR}"
            TREMOTESF_BUNDLED_ICON_THEME="${TREMOTESF_BUNDLED_ICON_THEME}"
            TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR="${TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR}"
        )
        set_target_properties(tremotesf PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)
    else()
        target_sources(
            tremotesf
            PRIVATE
            desktop/darkthemeapplier_noop.cpp
            desktop/systemcolorsprovider_noop.cpp
        )
    endif()

    qt_import_plugins(tremotesf
        EXCLUDE_BY_TYPE imageformats
        EXCLUDE_BY_TYPE iconengines
        INCLUDE Qt::QSvgIconPlugin
    )
endif()

if (UNIX AND NOT APPLE)
    find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS DBus X11Extras)
    find_package(KF5WindowSystem REQUIRED)
    target_link_libraries(tremotesf Qt::DBus Qt::X11Extras KF5::WindowSystem)

    set_source_files_properties(org.freedesktop.Application.xml PROPERTIES CLASSNAME IpcDbusInterface NO_NAMESPACE ON)
    qt_add_dbus_adaptor(dbus_generated org.freedesktop.Application.xml ipcserver_dbus_service.h tremotesf::IpcDbusService ipcserver_dbus_service_adaptor IpcDbusServiceAdaptor)
    qt_add_dbus_interface(dbus_generated org.freedesktop.Application.xml ipcclient_dbus_interface)

    set_source_files_properties(
        desktop/org.freedesktop.Notifications.xml
        desktop/org.freedesktop.FileManager1.xml
        PROPERTIES NO_NAMESPACE ON
    )
    qt_add_dbus_interface(dbus_generated desktop/org.freedesktop.Notifications.xml org.freedesktop.Notifications)
    qt_add_dbus_interface(dbus_generated desktop/org.freedesktop.FileManager1.xml org.freedesktop.FileManager1)

    target_sources(tremotesf PRIVATE ipcserver_dbus.cpp ipcserver_dbus_service.cpp ipcclient_dbus.cpp ${dbus_generated})
else()
    target_sources(tremotesf PRIVATE ipcserver_socket.cpp ipcclient_socket.cpp)
endif()

set_common_options_on_targets()

if (WIN32)
    install(TARGETS tremotesf DESTINATION ".")
    if (MSVC)
        install(FILES $<TARGET_PDB_FILE:tremotesf> DESTINATION "." CONFIGURATIONS Debug)
    endif()
else()
    install(TARGETS tremotesf DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
