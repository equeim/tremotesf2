// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAGNETLINKPARSER_H
#define TREMOTESF_MAGNETLINKPARSER_H

#include <set>
#include <vector>

#include "log/formatters.h"

class QUrl;

namespace tremotesf {
    inline constexpr auto magnetScheme = QLatin1String("magnet");

    struct TorrentMagnetLink {
        QString infoHashV1;
        std::vector<std::set<QString>> trackers;

        bool operator==(const TorrentMagnetLink&) const = default;
    };

    /**
     * @throws std::runtime_error
     */
    TorrentMagnetLink parseMagnetLink(const QUrl& url);

    QDebug operator<<(QDebug debug, const TorrentMagnetLink& magnetLink);
}

template<>
struct fmt::formatter<tremotesf::TorrentMagnetLink> : tremotesf::SimpleFormatter {
    format_context::iterator format(const tremotesf::TorrentMagnetLink& magnetLink, format_context& ctx) const;
};

#endif // TREMOTESF_MAGNETLINKPARSER_H
