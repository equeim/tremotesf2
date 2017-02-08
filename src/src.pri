CONFIG += c++11
QT = core network gui

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

    win32:CONFIG(debug, debug|release) {
        CONFIG += console
    }
}

QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
QMAKE_CXXFLAGS_DEBUG += -ggdb

HEADERS = $$PWD/accounts.h \
          $$PWD/accountsmodel.h \
          $$PWD/alltrackersmodel.h \
          $$PWD/peersmodel.h \
          $$PWD/baseproxymodel.h \
          $$PWD/basetorrentfilesmodel.h \
          $$PWD/ipcserver.h \
          $$PWD/jsonparser.h \
          $$PWD/localtorrentfilesmodel.h \
          $$PWD/torrentsmodel.h \
          $$PWD/rpc.h \
          $$PWD/serversettings.h \
          $$PWD/serverstats.h \
          $$PWD/settings.h \
          $$PWD/statusfilterstats.h \
          $$PWD/torrent.h \
          $$PWD/torrentfileparser.h \
          $$PWD/torrentfilesmodel.h \
          $$PWD/torrentfilesmodelentry.h \
          $$PWD/torrentfilesproxymodel.h \
          $$PWD/torrentsproxymodel.h \
          $$PWD/tracker.h \
          $$PWD/trackersmodel.h \
          $$PWD/utils.h

SOURCES = $$PWD/main.cpp \
          $$PWD/accounts.cpp \
          $$PWD/accountsmodel.cpp \
          $$PWD/alltrackersmodel.cpp \
          $$PWD/peersmodel.cpp \
          $$PWD/baseproxymodel.cpp \
          $$PWD/basetorrentfilesmodel.cpp \
          $$PWD/ipcserver.cpp \
          $$PWD/jsonparser.cpp \
          $$PWD/localtorrentfilesmodel.cpp \
          $$PWD/torrentsmodel.cpp \
          $$PWD/serversettings.cpp \
          $$PWD/rpc.cpp \
          $$PWD/serverstats.cpp \
          $$PWD/settings.cpp \
          $$PWD/statusfilterstats.cpp \
          $$PWD/torrentfileparser.cpp \
          $$PWD/torrent.cpp \
          $$PWD/torrentfilesmodel.cpp \
          $$PWD/torrentfilesmodelentry.cpp \
          $$PWD/torrentfilesproxymodel.cpp \
          $$PWD/torrentsproxymodel.cpp \
          $$PWD/tracker.cpp \
          $$PWD/trackersmodel.cpp \
          $$PWD/utils.cpp

sailfishos {
    HEADERS += $$PWD/sailfishos/directorycontentmodel.h \
               $$PWD/sailfishos/selectionmodel.h

    SOURCES += $$PWD/sailfishos/directorycontentmodel.cpp \
               $$PWD/sailfishos/selectionmodel.cpp
} else {
    HEADERS += $$PWD/desktop/aboutdialog.h \
               $$PWD/desktop/accounteditdialog.h \
               $$PWD/desktop/accountsdialog.h \
               $$PWD/desktop/addtorrentdialog.h \
               $$PWD/desktop/basetreeview.h \
               $$PWD/desktop/commondelegate.h \
               $$PWD/desktop/fileselectionwidget.h \
               $$PWD/desktop/mainwindow.h \
               $$PWD/desktop/mainwindowsidebar.h \
               $$PWD/desktop/mainwindowstatusbar.h \
               $$PWD/desktop/serversettingsdialog.h \
               $$PWD/desktop/settingsdialog.h \
               $$PWD/desktop/torrentfilesview.h \
               $$PWD/desktop/torrentpropertiesdialog.h \
               $$PWD/desktop/torrentsview.h \
               $$PWD/desktop/trackersviewwidget.h

    SOURCES += $$PWD/desktop/aboutdialog.cpp \
               $$PWD/desktop/accounteditdialog.cpp \
               $$PWD/desktop/accountsdialog.cpp \
               $$PWD/desktop/addtorrentdialog.cpp \
               $$PWD/desktop/basetreeview.cpp \
               $$PWD/desktop/commondelegate.cpp \
               $$PWD/desktop/fileselectionwidget.cpp \
               $$PWD/desktop/mainwindow.cpp \
               $$PWD/desktop/mainwindowsidebar.cpp \
               $$PWD/desktop/mainwindowstatusbar.cpp \
               $$PWD/desktop/serversettingsdialog.cpp \
               $$PWD/desktop/settingsdialog.cpp \
               $$PWD/desktop/torrentfilesview.cpp \
               $$PWD/desktop/torrentpropertiesdialog.cpp \
               $$PWD/desktop/torrentsview.cpp \
               $$PWD/desktop/trackersviewwidget.cpp
}

RESOURCES = $$PWD/resources.qrc

target.path = $$PREFIX/bin
INSTALLS += target
