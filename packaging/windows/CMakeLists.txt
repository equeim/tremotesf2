# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include("../CPackCommon.cmake")

set(CPACK_GENERATOR ZIP WIX)
set(CPACK_PACKAGE_NAME "Tremotesf")
set(CPACK_PACKAGE_VENDOR "Tremotesf")
set(CPACK_PACKAGE_EXECUTABLES "tremotesf;Tremotesf")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Tremotesf")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/license.rtf")
set(CPACK_WIX_UPGRADE_GUID "67e0511b-d9de-4b3c-a604-0dd47522d451")
set(CPACK_WIX_PRODUCT_ICON "${TREMOTESF_WINDOWS_ICON}")
set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/registry.wxs")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/registry_feature_patch.wxs")

include(CPack)
