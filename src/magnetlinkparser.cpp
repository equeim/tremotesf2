// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <ranges>
#include <stdexcept>

#include <QUrlQuery>

#include <fmt/ranges.h>

#include "magnetlinkparser.h"
#include "stdutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        constexpr auto xtKey = "xt"_L1;
        constexpr auto xtValuePrefix = "urn:btih:"_L1;
        constexpr auto trKey = "tr"_L1;
    }

    TorrentMagnetLink parseMagnetLink(const QUrl& url) {
        if (url.scheme() != magnetScheme) {
            throw std::runtime_error("URL scheme must be magnet");
        }

        const QUrlQuery query(url);

        const auto xtValues = query.allQueryItemValues(xtKey, QUrl::FullyDecoded);
        const auto infoHashV1Value =
            std::ranges::find_if(xtValues, [](const auto& value) { return value.startsWith(xtValuePrefix); });
        if (infoHashV1Value == xtValues.end()) {
            throw std::runtime_error("Did not find v1 info hash in the URL");
        }
        auto infoHashV1 = infoHashV1Value->mid(xtValuePrefix.size()).toLower();

        auto trackers = toContainer<std::vector>(
            query.allQueryItemValues(trKey, QUrl::FullyDecoded) |
            std::views::transform([](auto tracker) { return std::set{tracker}; })
        );

        return {.infoHashV1 = std::move(infoHashV1), .trackers = std::move(trackers)};
    }

    QDebug operator<<(QDebug debug, const TorrentMagnetLink& magnetLink) {
        const QDebugStateSaver saver(debug);
        debug.noquote() << fmt::format(impl::singleArgumentFormatString, magnetLink).c_str();
        return debug;
    }

}

fmt::format_context::iterator fmt::formatter<tremotesf::TorrentMagnetLink>::format(
    const tremotesf::TorrentMagnetLink& magnetLink, format_context& ctx
) const {
    return fmt::format_to(
        ctx.out(),
        "MagnetLink(infoHashV1={}, trackers={})",
        magnetLink.infoHashV1,
        magnetLink.trackers
    );
}
