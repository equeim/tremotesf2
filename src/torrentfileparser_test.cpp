// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <set>

#include <QDir>
#include <QTest>

#include <fmt/ranges.h>

#include "log/formatters.h"
#include "literals.h"
#include "torrentfileparser.h"
#include "stdutils.h"

QDebug operator<<(QDebug debug, const std::vector<std::set<QString>>& trackers) {
    const QDebugStateSaver saver(debug);
    debug.noquote() << fmt::format(tremotesf::impl::singleArgumentFormatString, trackers).c_str();
    return debug;
}

namespace tremotesf {
    class TorrentFileParserTest final : public QObject {
        Q_OBJECT
    private slots:
        void parseDebianTorrentFile() {
            auto torrentFile = parseTorrentFile(QDir::current().filePath("debian-10.9.0-amd64-netinst.iso.torrent"_l1));
            QCOMPARE(torrentFile.infoHashV1, "9f292c93eb0dbdd7ff7a4aa551aaa1ea7cafe004"_l1);

            const std::vector<std::set<QString>> expectedTrackers{{"http://bttracker.debian.org:6969/announce"_l1}};
            QCOMPARE(torrentFile.trackers, expectedTrackers);

            QCOMPARE(torrentFile.isSingleFile(), true);
            QCOMPARE(torrentFile.singleFileSize(), 353370112);
            QCOMPARE(torrentFile.rootFileName, "debian-10.9.0-amd64-netinst.iso"_l1);
        }

        void parseWikiTorrentFile() {
            auto torrentFile = parseTorrentFile(
                QDir::current().filePath("enwiki-20231220-pages-articles-multistream.xml.bz2.torrent"_l1)
            );
            QCOMPARE(torrentFile.infoHashV1, "80fb3b384728e950f2fd09e5929970d3d576270d"_l1);

            qInfo() << torrentFile.trackers;
            const std::vector<std::set<QString>> expectedTrackers{
                {"http://tracker.opentrackr.org:1337/announce"_l1},
                {"udp://tracker.opentrackr.org:1337"_l1},
                {"udp://tracker.openbittorrent.com:80/announce"_l1},
                {"http://fosstorrents.com:6969/announce"_l1},
                {"udp://fosstorrents.com:6969/announce"_l1}
            };
            QCOMPARE(torrentFile.trackers, expectedTrackers);

            QCOMPARE(torrentFile.isSingleFile(), true);
            QCOMPARE(torrentFile.singleFileSize(), 22711545577);
            QCOMPARE(torrentFile.rootFileName, "enwiki-20231220-pages-articles-multistream.xml.bz2"_l1);
        }

        void parseFedoraTorrentFile() {
            auto torrentFile =
                parseTorrentFile(QDir::current().filePath("Fedora-Workstation-Live-x86_64-34.torrent"_l1));
            QCOMPARE(torrentFile.infoHashV1, "2046e45fb6cf298cd25e4c0decbea40c6603d91b"_l1);

            const std::vector<std::set<QString>> expectedTrackers{{"http://torrent.fedoraproject.org:6969/announce"_l1}
            };
            QCOMPARE(torrentFile.trackers, expectedTrackers);

            QCOMPARE(torrentFile.isSingleFile(), false);
            QCOMPARE(torrentFile.rootFileName, "Fedora-Workstation-Live-x86_64-34"_l1);

            const auto files =
                toContainer<std::set>(torrentFile.files() | std::views::transform([](TorrentMetainfoFile::File file) {
                                          return std::pair{file.size, toContainer<std::vector>(file.path())};
                                      }));
            const std::set<std::pair<bencode::Integer, std::vector<QString>>> expected{
                {1062, {"Fedora-Workstation-34-1.2-x86_64-CHECKSUM"_l1}},
                {2007367680, {"Fedora-Workstation-Live-x86_64-34-1.2.iso"_l1}}
            };
            QCOMPARE(files, expected);
        }

        void parseHybridV2TorrentFile() {
            auto torrentFile = parseTorrentFile(QDir::current().filePath("bittorrent-v2-hybrid-test.torrent"_l1));
            QCOMPARE(torrentFile.infoHashV1, "631a31dd0a46257d5078c0dee4e66e26f73e42ac"_l1);

            const std::vector<std::set<QString>> expectedTrackers{};
            QCOMPARE(torrentFile.trackers, expectedTrackers);

            QCOMPARE(torrentFile.isSingleFile(), false);
            QCOMPARE(torrentFile.rootFileName, "bittorrent-v1-v2-hybrid-test"_l1);

            const auto files =
                toContainer<std::set>(torrentFile.files() | std::views::transform([](TorrentMetainfoFile::File file) {
                                          return std::pair{file.size, toContainer<std::vector>(file.path())};
                                      }));
            const std::set<std::pair<bencode::Integer, std::vector<QString>>> expected{
                {129434, {".pad"_l1, "129434"_l1}},
                {227380, {".pad"_l1, "227380"_l1}},
                {280339, {".pad"_l1, "280339"_l1}},
                {442368, {".pad"_l1, "442368"_l1}},
                {464896, {".pad"_l1, "464896"_l1}},
                {507162, {".pad"_l1, "507162"_l1}},
                {510995, {".pad"_l1, "510995"_l1}},
                {524227, {".pad"_l1, "524227"_l1}},
                {6535405, {"Darkroom (Stellar, 1994, Amiga ECS) HQ.mp4"_l1}},
                {20506624, {"Spaceballs-StateOfTheArt.avi"_l1}},
                {342230630, {"cncd_fairlight-ceasefire_(all_falls_down)-1080p.mp4"_l1}},
                {61638604, {"eld-dust.mkv"_l1}},
                {277889766, {"fairlight_cncd-agenda_circling_forth-1080p30lq.mp4"_l1}},
                {44577773, {"meet the deadline - Still _ Evoke 2014.mp4"_l1}},
                {61, {"readme.txt"_l1}},
                {26296320, {"tbl-goa.avi"_l1}},
                {115869700, {"tbl-tint.mpg"_l1}}
            };
            QCOMPARE(files, expected);
        }

        void parseV2TorrentFile() {
            try {
                auto torrentFile = parseTorrentFile(QDir::current().filePath("bittorrent-v2-test.torrent"_l1));
                QFAIL("parseTorrentFile must throw an exception");
            } catch (const bencode::Error&) {}
        }
    };
}

QTEST_GUILESS_MAIN(tremotesf::TorrentFileParserTest)

#include "torrentfileparser_test.moc"
