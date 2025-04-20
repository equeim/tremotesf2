// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tracker.h"

#include <QJsonObject>
#include <QHostAddress>
#include <QUrl>

#ifndef TREMOTESF_REGISTRABLE_DOMAIN_QT
#    include <libpsl.h>
#endif

#include "jsonutils.h"
#include "stdutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    using namespace impl;
    namespace {
        constexpr auto statusMapper = EnumMapper(
            std::array{
                EnumMapping(Tracker::Status::Inactive, 0),
                EnumMapping(Tracker::Status::WaitingForUpdate, 1),
                EnumMapping(Tracker::Status::QueuedForUpdate, 2),
                EnumMapping(Tracker::Status::Updating, 3)
            }
        );
    }

    Tracker::Tracker(int id, const QJsonObject& trackerMap) : mId(id) { update(trackerMap); }

    bool Tracker::update(const QJsonObject& trackerMap) {
        bool changed = false;

        QString announce(trackerMap.value("announce"_L1).toString());
        if (announce != mAnnounce) {
            changed = true;
            mAnnounce = std::move(announce);
            mSite = registrableDomainFromUrl(QUrl(mAnnounce));
        }

        setChanged(mTier, trackerMap.value("tier"_L1).toInt(), changed);

        const bool announceError =
            (!trackerMap.value("lastAnnounceSucceeded"_L1).toBool()
             && trackerMap.value("lastAnnounceTime"_L1).toInt() != 0);
        if (announceError) {
            setChanged(mErrorMessage, trackerMap.value("lastAnnounceResult"_L1).toString(), changed);
        } else {
            setChanged(mErrorMessage, {}, changed);
        }

        constexpr auto announceStateKey = "announceState"_L1;
        setChanged(mStatus, statusMapper.fromJsonValue(trackerMap.value(announceStateKey), announceStateKey), changed);

        setChanged(mPeers, trackerMap.value("lastAnnouncePeerCount"_L1).toInt(), changed);
        setChanged(
            mSeeders,
            [&] {
                if (auto seeders = trackerMap.value("seederCount"_L1).toInt(); seeders >= 0) {
                    return seeders;
                }
                return 0;
            }(),
            changed
        );
        setChanged(
            mLeechers,
            [&] {
                if (auto leechers = trackerMap.value("leecherCount"_L1).toInt(); leechers >= 0) {
                    return leechers;
                }
                return 0;
            }(),
            changed
        );
        updateDateTime(mNextUpdateTime, trackerMap.value("nextAnnounceTime"_L1), changed);

        return changed;
    }
}

#ifdef TREMOTESF_REGISTRABLE_DOMAIN_QT
// Private Qt API
bool qIsEffectiveTLD(QStringView domain);

namespace {
    QString registrableDomainFromDomain(const QString& fullDomain, [[maybe_unused]] const QUrl& url) {
        QStringView domain = fullDomain;
        QStringView previousDomain = fullDomain;
        while (!domain.isEmpty()) {
            if (qIsEffectiveTLD(domain)) {
                return previousDomain.toString();
            }
            const auto dotIndex = domain.indexOf('.');
            if (dotIndex == -1) {
                break;
            }
            previousDomain = domain;
            domain = domain.sliced(dotIndex + 1);
        }
        return fullDomain;
    }
}
#else
namespace {
    QString registrableDomainFromDomain(const QString& fullDomain, [[maybe_unused]] const QUrl& url) {
        const auto fullDomainUtf8 = fullDomain.toUtf8();
        const auto psl = psl_builtin();
        if (!psl) {
            return fullDomain;
        }
        const auto registrable = psl_registrable_domain(psl, fullDomainUtf8);
        return registrable ? QString(registrable) : fullDomain;
    }
}
#endif

QString tremotesf::impl::registrableDomainFromUrl(const QUrl& url) {
    auto host = url.host().toLower().normalized(QString::NormalizationForm_KC);
    if (host.isEmpty()) {
        return {};
    }
    if (const bool isIpAddress = !QHostAddress(host).isNull(); isIpAddress) {
        return host;
    }
    return registrableDomainFromDomain(host, url);
}
