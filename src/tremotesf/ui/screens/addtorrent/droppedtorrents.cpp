// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "droppedtorrents.h"

#include <QMimeData>
#include <QUrl>

#include "libtremotesf/literals.h"

namespace tremotesf {
    namespace {
        constexpr auto torrentFileSuffix = ".torrent"_l1;
        constexpr auto magnetScheme = "magnet"_l1;
        constexpr auto magnetQueryPrefixV1 = "xt=urn:btih:"_l1;
        constexpr auto magnetQueryPrefixV2 = "xt=urn:btmh:"_l1;
        constexpr auto httpScheme = "http"_l1;
        constexpr auto httpsScheme = "https"_l1;
    }

    DroppedTorrents::DroppedTorrents(const QMimeData* mime) {
        // We need to validate whether we are actually opening torrent file (based on extension)
        // or BitTorrent magnet link in order to not accept event and tell OS that we don't want it
        if (mime->hasUrls()) {
            const auto mimeUrls = mime->urls();
            for (const auto& url : mimeUrls) {
                processUrl(url);
            }
        } else if (mime->hasText()) {
            const auto text = mime->text();
            const auto lines = QStringView(text).split(u'\n');
            for (auto line : lines) {
                if (!line.isEmpty()) {
                    processUrl(QUrl(line.toString()));
                }
            }
        }
    }

    void DroppedTorrents::processUrl(const QUrl& url) {
        if (url.isLocalFile()) {
            if (auto path = url.toLocalFile(); path.endsWith(torrentFileSuffix)) {
                files.push_back(path);
            }
        } else {
            const auto scheme = url.scheme();
            if (scheme == magnetScheme && url.hasQuery()) {
                const auto query = url.query();
                if (query.startsWith(magnetQueryPrefixV1) || query.startsWith(magnetQueryPrefixV2)) {
                    urls.push_back(url.toString());
                }
            } else if (scheme == httpScheme || scheme == httpsScheme) {
                urls.push_back(url.toString());
            }
        }
    }
}
