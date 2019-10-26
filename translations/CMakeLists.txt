find_package(Qt5 REQUIRED COMPONENTS LinguistTools)
qt5_add_translation(qm_files
    en.ts
    es.ts
    nl_BE.ts
    nl.ts
    ru.ts
)
add_custom_target(translations ALL DEPENDS ${qm_files})

if (TEST_BUILD)
    set(translations_path "${CMAKE_BINARY_DIR}/translations")
else()
    set(translations_path "${DATA_PATH}/translations")
    if (NOT WIN32)
        install(FILES ${qm_files} DESTINATION "${translations_path}")
    endif()
endif()
set(TRANSLATIONS_PATH "${translations_path}" PARENT_SCOPE)
