sailfishos {
    icons.files = $$PWD/sailfishos/hicolor
    desktop.files = $$PWD/sailfishos/harbour-tremotesf.desktop

    cover.files = $$PWD/sailfishos/cover.svg
    cover.path = $$PREFIX/share/$$TARGET
    INSTALLS += cover
} else {
    icons.files = $$PWD/desktop/hicolor
    desktop.files = $$PWD/desktop/tremotesf.desktop

    status_icons.files = $$PWD/desktop/status/*.png
    status_icons.path = $$PREFIX/share/$$TARGET/icons
    INSTALLS += status_icons
    DEFINES += ICONS_PATH=\\\"$$status_icons.path\\\"
}

icons.path = $$PREFIX/share/icons
INSTALLS += icons

desktop.path = $$PREFIX/share/applications
INSTALLS += desktop
