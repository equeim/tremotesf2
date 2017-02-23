lessThan(QT_VERSION, 5.2) {
    error("Requires Qt 5.2 or greather")
}

VERSION = 1.2.1
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

    OTHER_FILES += rpm/harbour-tremotesf.spec
}

include(translations/translations.pri)
