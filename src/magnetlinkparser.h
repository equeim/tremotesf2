// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAGNETLINKPARSER_H
#define TREMOTESF_MAGNETLINKPARSER_H

#include <set>
#include <vector>

#include "log/formatters.h"
#include "literals.h"

class QUrl;

namespace tremotesf {
    inline constexpr auto magnetScheme = "magnet"_l1;

    struct MagnetLink {
        QString infoHashV1;
        std::vector<std::set<QString>> trackers;

        bool operator==(const MagnetLink&) const = default;
    };

    /**
     * @throws std::runtime_error
     */
    MagnetLink parseMagnetLink(const QUrl& url);

    QDebug operator<<(QDebug debug, const MagnetLink& magnetLink);
}

template<>
struct fmt::formatter<tremotesf::MagnetLink> : tremotesf::SimpleFormatter {
    format_context::iterator format(const tremotesf::MagnetLink& magnetLink, format_context& ctx) const;
};

#endif // TREMOTESF_MAGNETLINKPARSER_H
