lessThan(QT_MAJOR_VERSION, 5) {
    error("Requires Qt 5.6 or greather")
}

lessThan(QT_MINOR_VERSION, 6) {
    error("Requires Qt 5.6 or greather")
}

TEMPLATE = subdirs
SUBDIRS = libtremotesf tremotesf
tremotesf.depends = libtremotesf

OTHER_FILES += .clang-format \
               .gitignore \
               CHANGELOG.md \
               README.md \
               .tx/config \
               rpm/harbour-tremotesf.spec
