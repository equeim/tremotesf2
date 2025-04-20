// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_PRAGMAMACROS_H
#define TREMOTESF_PRAGMAMACROS_H

#if defined(__GNUC__) || defined(__clang__)
#    define SUPPRESS_DEPRECATED_WARNINGS_BEGIN \
        _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#    define SUPPRESS_DEPRECATED_WARNINGS_END _Pragma("GCC diagnostic pop")
#else
#    define SUPPRESS_DEPRECATED_WARNINGS_BEGIN _Pragma("warning(push)") _Pragma("warning(disable : 4996)")
#    define SUPPRESS_DEPRECATED_WARNINGS_END   _Pragma("warning(pop)")
#endif

#endif // TREMOTESF_PRAGMAMACROS_H
