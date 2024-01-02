# SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

include("${VCPKG_ROOT_DIR}/triplets/x64-windows-static.cmake")
# Always Use C++20 coroutines ABI
set(VCPKG_CXX_FLAGS /await:strict)
set(VCPKG_C_FLAGS /await:strict)
