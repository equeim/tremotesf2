TRANSLATIONS = $$PWD/en.ts \
               $$PWD/nl.ts \
               $$PWD/nl_BE.ts \
               $$PWD/ru.ts

lupdate_only {
    TRANSLATIONS += $$PWD/source.ts
}

lrelease.input = TRANSLATIONS
lrelease.output = $$shadowed(${QMAKE_FILE_BASE}.qm)
lrelease.commands = $$[QT_INSTALL_BINS]/lrelease -removeidentical ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
lrelease.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += lrelease

for (file, TRANSLATIONS) {
    file = $$shadowed($$file)
    TRANSLATIONS_QM += $$sprintf("%1.qm", $$str_member($$file, 0, $$num_add($$str_size($$file), -4)))
}

translations.files = $$TRANSLATIONS_QM
translations.path = $$PREFIX/share/$$TARGET/translations
translations.CONFIG = no_check_exist
INSTALLS += translations
DEFINES += TRANSLATIONS_PATH=\\\"$$translations.path\\\"
