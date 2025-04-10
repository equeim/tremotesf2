// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "droppedtorrents.h"

#include <QMimeData>
#include <QUrl>

#include "log/log.h"
#include "magnetlinkparser.h"

using namespace Qt::StringLiterals;

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf {
    namespace {
        constexpr auto torrentFileSuffix = ".torrent"_L1;
        constexpr auto httpScheme = "http"_L1;
        constexpr auto httpsScheme = "https"_L1;
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
            const auto lines = QStringView(text).split(u'\n', Qt::SkipEmptyParts);
            for (auto line : lines) {
                processUrl(QUrl(line.toString()));
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
            if (scheme == magnetScheme) {
                try {
                    parseMagnetLink(url);
                    urls.push_back(url.toString());
                } catch (const std::runtime_error& e) {
                    warning().logWithException(e, "Failed to parse URL {} as magnet link", url);
                }
            } else if (scheme == httpScheme || scheme == httpsScheme) {
                urls.push_back(url.toString());
            }
        }
    }
}
