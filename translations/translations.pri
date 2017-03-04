TRANSLATIONS = $$PWD/en.ts \
               $$PWD/ru.ts

lupdate_only {
    TRANSLATIONS += $$PWD/source.ts
}

lrelease.input = TRANSLATIONS
lrelease.output = $$shadowed(${QMAKE_FILE_BASE}.qm)
lrelease.commands = $$[QT_INSTALL_BINS]/lrelease -removeidentical ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
lrelease.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += lrelease

translations.files = $$shadowed(*.qm)
translations.path = $$PREFIX/share/$$TARGET/translations
translations.CONFIG = no_check_exist
INSTALLS += translations
DEFINES += TRANSLATIONS_PATH=\\\"$$translations.path\\\"
