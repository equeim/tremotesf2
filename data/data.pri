sailfishos {
    icons.files = $$PWD/sailfishos/hicolor
    icons.path = $$PREFIX/share/icons
    INSTALLS += icons

    cover.files = $$PWD/sailfishos/cover.svg
    cover.path = $$PREFIX/share/$$TARGET
    INSTALLS += cover

    desktop.files = $$PWD/sailfishos/harbour-tremotesf.desktop
    desktop.path = $$PREFIX/share/applications
    INSTALLS += desktop
} else {
    icons.files = $$PWD/desktop/active.png \
                  $$PWD/desktop/checking.png \
                  $$PWD/desktop/downloading.png \
                  $$PWD/desktop/errored.png \
                  $$PWD/desktop/paused.png \
                  $$PWD/desktop/queued.png \
                  $$PWD/desktop/seeding.png \
                  $$PWD/desktop/stalled-downloading.png \
                  $$PWD/desktop/stalled-seeding.png
    icons.path = $$PREFIX/share/$$TARGET/icons
    INSTALLS += icons
    DEFINES += ICONS_PATH=\\\"$$icons.path\\\"

    app_icons.files = $$PWD/desktop/hicolor
    app_icons.path = $$PREFIX/share/icons
    INSTALLS += app_icons

    desktop.files = $$PWD/desktop/tremotesf.desktop
    desktop.path = $$PREFIX/share/applications
    INSTALLS += desktop
}
