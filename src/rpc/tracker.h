// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_TRACKER_H
#define TREMOTESF_RPC_TRACKER_H

#include <QDateTime>
#include <QObject>
#include <QString>

class QJsonObject;
class QUrl;

namespace tremotesf {
    class Tracker {
        Q_GADGET
    public:
        enum class Status {
            // Tracker is inactive, possibly due to error
            Inactive,
            // Waiting for announce/scrape
            WaitingForUpdate,
            // Queued for immediate announce/scrape
            QueuedForUpdate,
            // We are announcing/scraping
            Updating,
        };
        Q_ENUM(Status)

        explicit Tracker(int id, const QJsonObject& trackerMap);

        int id() const { return mId; };
        const QString& announce() const { return mAnnounce; };
        const QString& site() const { return mSite; };

        int tier() const { return mTier; }

        Status status() const { return mStatus; };
        const QString& errorMessage() const { return mErrorMessage; };

        int peers() const { return mPeers; };
        int seeders() const { return mSeeders; }
        int leechers() const { return mLeechers; }
        const QDateTime& nextUpdateTime() const { return mNextUpdateTime; };

        bool update(const QJsonObject& trackerMap);

        void replaceAnnounceUrl(const QString& announceUrl) { mAnnounce = announceUrl; }

        bool operator==(const Tracker& other) const = default;

    private:
        QString mAnnounce{};
        QString mSite{};
        int mTier{};

        Status mStatus{};
        QString mErrorMessage{};

        QDateTime mNextUpdateTime{{}, {}, Qt::UTC};

        int mPeers{};
        int mSeeders{};
        int mLeechers{};

        int mId{};
    };

    namespace impl {
        QString registrableDomainFromUrl(const QUrl& url);
    }
}

#endif // TREMOTESF_RPC_TRACKER_H
