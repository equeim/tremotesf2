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
    torrentfileparser.cpp
    torrentfilesmodel.cpp
    torrentfilesmodelentry.cpp
    torrentfilesproxymodel.cpp
    torrentsmodel.cpp
    torrentsproxymodel.cpp
    trackersmodel.cpp
    trpc.cpp
    utils.cpp
    resources.qrc
)

set_target_properties(tremotesf PROPERTIES
    OUTPUT_NAME "${TREMOTESF_NAME}"
    CXX_STANDARD 14
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
        -Wlogical-op
        -Wdouble-promotion
        -Wformat=2
        -Werror=format
    )
endif()

if (SAILFISHOS)
    find_package(Qt5 REQUIRED COMPONENTS Quick)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SAILFISHAPP REQUIRED sailfishapp)

    target_sources(tremotesf PRIVATE
        sailfishos/directorycontentmodel.cpp
        sailfishos/selectionmodel.cpp
    )
    target_link_libraries(tremotesf Qt5::Quick ${SAILFISHAPP_LDFLAGS})
    target_compile_definitions(tremotesf PRIVATE TREMOTESF_SAILFISHOS)
    target_include_directories(tremotesf PRIVATE ${SAILFISHAPP_INCLUDE_DIRS})
    target_compile_options(tremotesf PRIVATE ${SAILFISHAPP_CFLAGS_OTHER})
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
        desktop/fileselectionwidget.cpp
        desktop/mainwindow.cpp
        desktop/mainwindowsidebar.cpp
        desktop/mainwindowstatusbar.cpp
        desktop/remotedirectoryselectionwidget.cpp
        desktop/servereditdialog.cpp
        desktop/serversdialog.cpp
        desktop/serversettingsdialog.cpp
        desktop/serverstatsdialog.cpp
        desktop/settingsdialog.cpp
        desktop/textinputdialog.cpp
        desktop/torrentfilesview.cpp
        desktop/torrentpropertiesdialog.cpp
        desktop/torrentsview.cpp
        desktop/trackersviewwidget.cpp
    )

    target_compile_definitions(tremotesf PRIVATE ICONS_PATH="${ICONS_PATH}")

    if (WIN32)
        target_sources(tremotesf PRIVATE desktop/tremotesf.rc)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type)
        if (NOT build_type STREQUAL "debug")
            set_target_properties(tremotesf PROPERTIES WIN32_EXECUTABLE ON)
        endif()
    endif()
endif()

if (UNIX AND NOT APPLE)
    find_package(Qt5 REQUIRED COMPONENTS DBus)
    target_link_libraries(tremotesf Qt5::DBus)
    target_sources(tremotesf PRIVATE ipcserver_dbus.cpp ipcclient_dbus.cpp)
else()
    target_sources(tremotesf PRIVATE ipcserver_socket.cpp ipcclient_socket.cpp)
endif()

if (NOT TEST_BUILD AND NOT WIN32 AND NOT APPLE)
    install(TARGETS tremotesf DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
