// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_WINRT_BASE_INCLUDE_WRAPPER_H
#define TREMOTESF_WINRT_BASE_INCLUDE_WRAPPER_H

#if defined(_MSC_VER) && !defined(__clang__)
#    pragma warning(push)
#    pragma warning(disable : 4365)
#endif
#include <guiddef.h>
#include <winrt/base.h>
#if defined(_MSC_VER) && !defined(__clang__)
#    pragma warning(pop)
#endif

#endif // TREMOTESF_WINRT_BASE_INCLUDE_WRAPPER_H
