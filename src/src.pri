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

HEADERS = $$PWD/*.h
SOURCES = $$PWD/*.cpp

sailfishos {
    HEADERS += $$PWD/sailfishos/*.h
    SOURCES += $$PWD/sailfishos/*.cpp
} else {
    HEADERS += $$PWD/desktop/*.h
    SOURCES += $$PWD/desktop/*.cpp
}

RESOURCES = $$PWD/resources.qrc

target.path = $$PREFIX/bin
INSTALLS += target
