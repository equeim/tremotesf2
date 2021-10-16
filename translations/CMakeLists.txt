find_package(Qt5 REQUIRED COMPONENTS LinguistTools)
# Make sure to also change translations.qrc
qt5_add_translation(qm_files
    en.ts
    es.ts
    fr.ts
    it_IT.ts
    nl_BE.ts
    nl.ts
    pl.ts
    ru.ts
    zh_CN.ts
)
add_custom_target(translations ALL DEPENDS ${qm_files})

configure_file(translations.qrc "${CMAKE_BINARY_DIR}" COPYONLY)
list(APPEND QRC_FILES "${CMAKE_BINARY_DIR}/translations.qrc")
set(QRC_FILES ${QRC_FILES} PARENT_SCOPE)
