// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTest>

#include "magnetlinkparser.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    class MagnetLinkParserTest final : public QObject {
        Q_OBJECT
    private slots:
        void parseWikipediaLink() {
            const QUrl url(
                "magnet:?xt=urn:btih:779D06C72A13A72DA82611F44F07543AC691DC54&dn=enwiki-20240701-pages-"
                "articles-multistream.xml.bz2&tr=wss%3a%2f%2fwstracker.online"
            );
            const TorrentMagnetLink expected{
                .infoHashV1 = "779d06c72a13a72da82611f44f07543ac691dc54"_L1,
                .trackers = {{"wss://wstracker.online"_L1}}
            };
            QCOMPARE(expected, parseMagnetLink(url));
        }

        void parseAltWikipediaLink() {
            const QUrl url(
                "magnet:?xt=urn:btih:GVED7WSKNQJUIBE2KYU3SRWFRDEN4JVK&dn=simplewiki-20230820-pages-articles-"
                "multistream.xml.bz2&xl=283045562&tr=http%3A%2F%2Ftracker.opentrackr.org%3A1337%2Fannounce&"
                "tr=http%3A%2F%2Ftracker.opentrackr.org%3A1337%2Fannounce&tr=udp%3A%2F%2Ftracker.opentrackr."
                "org%3A1337&tr=udp%3A%2F%2Ftracker.openbittorrent.com%3A80%2Fannounce&tr=http%3A%2F%"
                "2Ffosstorrents.com%3A6969%2Fannounce&tr=udp%3A%2F%2Ffosstorrents.com%3A6969%2Fannounce"
            );
            const TorrentMagnetLink expected{
                .infoHashV1 = "gved7wsknqjuibe2kyu3srwfrden4jvk"_L1,
                .trackers = {
                    {"http://tracker.opentrackr.org:1337/announce"_L1},
                    {"http://tracker.opentrackr.org:1337/announce"_L1},
                    {"udp://tracker.opentrackr.org:1337"_L1},
                    {"udp://tracker.openbittorrent.com:80/announce"_L1},
                    {"http://fosstorrents.com:6969/announce"_L1},
                    {"udp://fosstorrents.com:6969/announce"_L1}
                }
            };
            QCOMPARE(expected, parseMagnetLink(url));
        }
    };
}

QTEST_GUILESS_MAIN(tremotesf::MagnetLinkParserTest)

#include "magnetlinkparser_test.moc"
