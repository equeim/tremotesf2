// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <QTest>

#include <fmt/format.h>

#include "tracker.h"
#include "torrent.h"

using namespace tremotesf::impl;

class TrackerTest final : public QObject {
    Q_OBJECT

private slots:
    void registrableDomainTest() {
        QCOMPARE(registrableDomainFromUrl(QUrl("https://doc.qt.io")), QString("qt.io"));
        QCOMPARE(registrableDomainFromUrl(QUrl("https://github.com")), QString("github.com"));
        QCOMPARE(registrableDomainFromUrl(QUrl("https://www.bbc.co.uk/")), QString("bbc.co.uk"));
        QCOMPARE(registrableDomainFromUrl(QUrl("https://en.wikipedia.org/wiki/Main_Page")), QString("wikipedia.org"));
        QCOMPARE(registrableDomainFromUrl(QUrl("https://forgot.his.name")), QString("forgot.his.name"));
    }

    void registrableDomainIpTest() {
        constexpr std::array ips{"127.0.0.1", "2001:0db8:0000:0000:0000:ff00:0042:8329"};
        for (const auto& ip : ips) {
            const QHostAddress expectedIp(ip);
            QUrl url{};
            url.setScheme("https");
            if (expectedIp.protocol() == QAbstractSocket::IPv6Protocol) {
                url.setHost(fmt::format("[{}]", ip).c_str());
            } else {
                url.setHost(ip);
            }
            const auto actualIpString = registrableDomainFromUrl(url);
            const QHostAddress actualIp(actualIpString);
            QCOMPARE(actualIp, expectedIp);
        }
    }

    void registrableDomainUnknownTest() {
        QCOMPARE(registrableDomainFromUrl(QUrl("https://foobar")), QString("foobar"));
        QCOMPARE(registrableDomainFromUrl(QUrl("https://")), QString());
    }

    void mergingTackersCompletelyNewTest() {
        const auto existingTrackers = std::vector{std::set{QString("foo")}, std::set{QString("bar")}};
        const auto newTrackers = std::vector{std::set{QString("lol")}, std::set{QString("nope")}};
        const auto expectedResult = std::vector{
            std::set{QString("foo")},
            std::set{QString("bar")},
            std::set{QString("lol")},
            std::set{QString("nope")}
        };
        const auto merged = mergeTrackers(existingTrackers, newTrackers);
        QCOMPARE(merged, expectedResult);
    }

    void mergingTrackersAddingToExistingTierTest() {
        const auto existingTrackers = std::vector{std::set{QString("foo")}, std::set{QString("bar")}};
        const auto newTrackers =
            std::vector{std::set{QString("foo"), QString("foo.alt1"), QString("foo.alt2")}, std::set{QString("nope")}};
        const auto expectedResult = std::vector{
            std::set{QString("foo"), QString("foo.alt1"), QString("foo.alt2")},
            std::set{QString("bar")},
            std::set{QString("nope")}
        };
        const auto merged = mergeTrackers(existingTrackers, newTrackers);
        QCOMPARE(merged, expectedResult);
    }
};

QTEST_GUILESS_MAIN(TrackerTest)

#include "tracker_test.moc"
