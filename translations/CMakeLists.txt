find_package(Qt5LinguistTools CONFIG REQUIRED)
qt5_add_translation(qm_files
    en.ts
    es.ts
    nl_BE.ts
    nl.ts
    ru.ts
)
add_custom_target(translations DEPENDS ${qm_files})
add_dependencies(tremotesf translations)
install(FILES ${qm_files} DESTINATION "${TRANSLATIONS_PATH}")
