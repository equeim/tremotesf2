CONFIG += c++11
QT = core network concurrent gui

sailfishos {
    QT += qml quick
    CONFIG += link_pkgconfig
    PKGCONFIG = sailfishapp
    DEFINES += TREMOTESF_SAILFISHOS
} else {
    QT += widgets KWidgetsAddons
    unix {
        QT += dbus
    }

    win32 {
        RC_ICONS = $$PWD/desktop/tremotesf.ico

        CONFIG(debug, debug|release) {
            CONFIG += console
        }
    }
}

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
QMAKE_CXXFLAGS_DEBUG += -ggdb

DEFINES += QT_DEPRECATED_WARNINGS QT_DISABLE_DEPRECATED_BEFORE=0x050800

INCLUDEPATH += ..
LIBS += -L../libtremotesf -ltremotesf

HEADERS = $$PWD/alltrackersmodel.h \
          $$PWD/baseproxymodel.h \
          $$PWD/basetorrentfilesmodel.h \
          $$PWD/downloaddirectoriesmodel.h \
          $$PWD/ipcserver.h \
          $$PWD/localtorrentfilesmodel.h \
          $$PWD/peersmodel.h \
          $$PWD/rpc.h \
          $$PWD/servers.h \
          $$PWD/serversmodel.h \
          $$PWD/settings.h \
          $$PWD/statusfilterstats.h \
          $$PWD/torrentfileparser.h \
          $$PWD/torrentfilesmodelentry.h \
          $$PWD/torrentfilesmodel.h \
          $$PWD/torrentfilesproxymodel.h \
          $$PWD/torrentsmodel.h \
          $$PWD/torrentsproxymodel.h \
          $$PWD/trackersmodel.h \
          $$PWD/utils.h

SOURCES = $$PWD/alltrackersmodel.cpp \
          $$PWD/baseproxymodel.cpp \
          $$PWD/basetorrentfilesmodel.cpp \
          $$PWD/downloaddirectoriesmodel.cpp \
          $$PWD/ipcserver.cpp \
          $$PWD/localtorrentfilesmodel.cpp \
          $$PWD/main.cpp \
          $$PWD/peersmodel.cpp \
          $$PWD/rpc.cpp \
          $$PWD/servers.cpp \
          $$PWD/serversmodel.cpp \
          $$PWD/settings.cpp \
          $$PWD/statusfilterstats.cpp \
          $$PWD/torrentfileparser.cpp \
          $$PWD/torrentfilesmodel.cpp \
          $$PWD/torrentfilesmodelentry.cpp \
          $$PWD/torrentfilesproxymodel.cpp \
          $$PWD/torrentsmodel.cpp \
          $$PWD/torrentsproxymodel.cpp \
          $$PWD/trackersmodel.cpp \
          $$PWD/utils.cpp

sailfishos {
    HEADERS += $$PWD/sailfishos/directorycontentmodel.h \
               $$PWD/sailfishos/selectionmodel.h

    SOURCES += $$PWD/sailfishos/directorycontentmodel.cpp \
               $$PWD/sailfishos/selectionmodel.cpp
} else {
    HEADERS += $$PWD/desktop/aboutdialog.h \
               $$PWD/desktop/addtorrentdialog.h \
               $$PWD/desktop/basetreeview.h \
               $$PWD/desktop/commondelegate.h \
               $$PWD/desktop/fileselectionwidget.h \
               $$PWD/desktop/mainwindow.h \
               $$PWD/desktop/mainwindowsidebar.h \
               $$PWD/desktop/mainwindowstatusbar.h \
               $$PWD/desktop/servereditdialog.h \
               $$PWD/desktop/serversdialog.h \
               $$PWD/desktop/serversettingsdialog.h \
               $$PWD/desktop/serverstatsdialog.h \
               $$PWD/desktop/settingsdialog.h \
               $$PWD/desktop/textinputdialog.h \
               $$PWD/desktop/torrentfilesview.h \
               $$PWD/desktop/torrentpropertiesdialog.h \
               $$PWD/desktop/torrentsview.h \
               $$PWD/desktop/trackersviewwidget.h

    SOURCES += $$PWD/desktop/aboutdialog.cpp \
               $$PWD/desktop/addtorrentdialog.cpp \
               $$PWD/desktop/basetreeview.cpp \
               $$PWD/desktop/commondelegate.cpp \
               $$PWD/desktop/fileselectionwidget.cpp \
               $$PWD/desktop/mainwindow.cpp \
               $$PWD/desktop/mainwindowsidebar.cpp \
               $$PWD/desktop/mainwindowstatusbar.cpp \
               $$PWD/desktop/servereditdialog.cpp \
               $$PWD/desktop/serversdialog.cpp \
               $$PWD/desktop/serversettingsdialog.cpp \
               $$PWD/desktop/serverstatsdialog.cpp \
               $$PWD/desktop/settingsdialog.cpp \
               $$PWD/desktop/textinputdialog.cpp \
               $$PWD/desktop/torrentfilesview.cpp \
               $$PWD/desktop/torrentpropertiesdialog.cpp \
               $$PWD/desktop/torrentsview.cpp \
               $$PWD/desktop/trackersviewwidget.cpp
}

RESOURCES = $$PWD/resources.qrc

target.path = $$PREFIX/bin
INSTALLS += target
