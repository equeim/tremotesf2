lessThan(QT_MAJOR_VERSION, 5) {
    error("Requires Qt 5.2 or greather")
}

lessThan(QT_MINOR_VERSION, 2) {
    error("Requires Qt 5.2 or greather")
}

VERSION = 1.4.0
DEFINES += TREMOTESF_VERSION=\\\"$$VERSION\\\"

sailfishos {
    TARGET = harbour-tremotesf
} else {
    TARGET = tremotesf
}

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

include(data/data.pri)
include(src/src.pri)

sailfishos {
    qml.files = qml
    qml.path = $$PREFIX/share/$$TARGET
    INSTALLS += qml
}

include(translations/translations.pri)

OTHER_FILES += .clang-format \
               .gitignore \
               CHANGELOG.md \
               README.md \
               .tx/config \
               rpm/harbour-tremotesf.spec
