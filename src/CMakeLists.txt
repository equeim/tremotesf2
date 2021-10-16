set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Concurrent Network)

add_executable(tremotesf
    libtremotesf/peer.cpp
    libtremotesf/rpc.cpp
    libtremotesf/serversettings.cpp
    libtremotesf/serverstats.cpp
    libtremotesf/torrent.cpp
    libtremotesf/torrentfile.cpp
    libtremotesf/tracker.cpp
    alltrackersmodel.cpp
    baseproxymodel.cpp
    basetorrentfilesmodel.cpp
    bencodeparser.cpp
    commandlineparser.cpp
    downloaddirectoriesmodel.cpp
    ipcserver.h
    localtorrentfilesmodel.cpp
    main.cpp
    peersmodel.cpp
    servers.cpp
    serversmodel.cpp
    settings.cpp
    signalhandler.cpp
    statusfilterstats.cpp
    torrentfilesmodel.cpp
    torrentfilesmodelentry.cpp
    torrentfilesproxymodel.cpp
    torrentsmodel.cpp
    torrentsproxymodel.cpp
    trackersmodel.cpp
    trpc.cpp
    utils.cpp
    resources.qrc
    ../data/authors/authors.qrc
)

set_target_properties(tremotesf PROPERTIES
    OUTPUT_NAME "${TREMOTESF_NAME}"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(tremotesf Qt5::Network)
target_include_directories(tremotesf PRIVATE ${Qt5Concurrent_INCLUDE_DIRS})

target_compile_definitions(tremotesf PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x050600
    QT_MESSAGELOGCONTEXT
    TREMOTESF_VERSION="${PROJECT_VERSION}"
    TRANSLATIONS_PATH="${TRANSLATIONS_PATH}"
)

if (TEST_BUILD)
    target_compile_definitions(tremotesf PRIVATE TEST_BUILD)
endif()

if (MSVC)
    target_compile_options(tremotesf PRIVATE /W4)
else()
    target_compile_options(tremotesf PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wnon-virtual-dtor
        -Wcast-align
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wdouble-promotion
        -Wformat=2
        -Werror=format
    )
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(tremotesf PRIVATE -Wlogical-op)
    endif()
endif()

if (SAILFISHOS)
    find_package(Qt5 REQUIRED COMPONENTS Quick)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SailfishApp REQUIRED IMPORTED_TARGET sailfishapp)

    target_sources(tremotesf PRIVATE
        sailfishos/directorycontentmodel.cpp
        sailfishos/sailfishosutils.cpp
        sailfishos/selectionmodel.cpp
        ../qml/qml.qrc
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
        desktop/addtorrentdialog.cpp
        desktop/basetreeview.cpp
        desktop/commondelegate.cpp
        desktop/connectionsettingsdialog.cpp
        desktop/fileselectionwidget.cpp
        desktop/desktoputils.cpp
        desktop/mainwindow.cpp
        desktop/mainwindowsidebar.cpp
        desktop/mainwindowstatusbar.cpp
        desktop/remotedirectoryselectionwidget.cpp
        desktop/servereditdialog.cpp
        desktop/serversettingsdialog.cpp
        desktop/serverstatsdialog.cpp
        desktop/settingsdialog.cpp
        desktop/textinputdialog.cpp
        desktop/torrentfilesview.cpp
        desktop/torrentpropertiesdialog.cpp
        desktop/torrentsview.cpp
        desktop/trackersviewwidget.cpp
    )

    target_compile_definitions(tremotesf PRIVATE
        ICONS_PATH="${ICONS_PATH}"
        TREMOTESF_APP_ID="org.equeim.Tremotesf"
        TREMOTESF_APP_NAME="Tremotesf"
    )

    if (WIN32)
        target_sources(tremotesf PRIVATE desktop/tremotesf.rc)
        set_target_properties(tremotesf PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
        if (NOT build_type STREQUAL "debug")
            set_target_properties(tremotesf PROPERTIES WIN32_EXECUTABLE ON)
        endif()
    endif()
endif()

if (UNIX AND NOT APPLE)
    find_package(Qt5 REQUIRED COMPONENTS DBus)
    target_link_libraries(tremotesf Qt5::DBus)

    if (NOT SAILFISHOS)
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

if (NOT TEST_BUILD AND NOT WIN32 AND NOT APPLE)
    install(TARGETS tremotesf DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
