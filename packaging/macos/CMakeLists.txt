# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

configure_file("Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" DESTINATION "${TREMOTESF_MACOS_BUNDLE_NAME}.app/Contents")

include("../CPackCommon.cmake")

set(CPACK_GENERATOR DragNDrop)
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (is_multi_config)
    set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/StripExecutable.cmake")
else()
    include(StripExecutable.cmake)
endif()

include(CPack)
