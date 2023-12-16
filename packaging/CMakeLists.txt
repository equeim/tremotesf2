# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (WIN32)
    add_subdirectory("windows")
elseif (APPLE)
    add_subdirectory("macos")
endif()
