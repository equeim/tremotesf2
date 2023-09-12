// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LITERALS_H
#define TREMOTESF_LITERALS_H

#include <QLatin1String>

namespace tremotesf {
    // QLatin1String(const char*) is not guaranteed to be constexpr in Qt < 6.4 since it uses strlen()
    // Add user-defined literal to circumvent this (and it's nicer to use!)
    inline constexpr QLatin1String operator""_l1(const char* str, size_t length) {
        return QLatin1String(str, static_cast<QLatin1String::size_type>(length));
    }
}

#endif // TREMOTESF_LITERALS_H
