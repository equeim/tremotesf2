// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_STYLEHELPERS_H
#define TREMOTESF_STYLEHELPERS_H

#include <optional>

class QStyle;

namespace tremotesf {
    enum class KnownStyle { Breeze, macOS };

    std::optional<KnownStyle> determineStyle(const QStyle* style);
    std::optional<KnownStyle> determineStyle();
}

#endif // TREMOTESF_STYLEHELPERS_H
