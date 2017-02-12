/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "torrent.h"

#include <QCoreApplication>
#include <QLocale>

#include "rpc.h"
#include "serversettings.h"
#include "tracker.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        const QString downloadSpeedLimitedKey(QStringLiteral("downloadLimited"));
        const QString downloadSpeedLimitKey(QStringLiteral("downloadLimit"));
        const QString uploadSpeedLimitedKey(QStringLiteral("uploadLimited"));
        const QString uploadSpeedLimitKey(QStringLiteral("uploadLimit"));
        const QString ratioLimitModeKey(QStringLiteral("seedRatioMode"));
        const QString ratioLimitKey(QStringLiteral("seedRatioLimit"));
        const QString peersLimitKey(QStringLiteral("peer-limit"));
        const QString honorSessionLimitsKey(QStringLiteral("honorsSessionLimits"));
        const QString bandwidthPriorityKey(QStringLiteral("bandwidthPriority"));
        const QString idleSeedingLimitModeKey(QStringLiteral("seedIdleMode"));
        const QString idleSeedingLimitKey(QStringLiteral("seedRatioLimit"));
    }

    Torrent::Torrent(int id, const QVariantMap& torrentMap, Rpc* rpc)
        : mId(id),
          mHashString(torrentMap.value(QStringLiteral("hashString")).toString()),
          mAddedDate(QDateTime::fromTime_t(torrentMap.value(QStringLiteral("addedDate")).toUInt())),
          mFilesEnabled(false),
          mPeersEnabled(false),
          mRpc(rpc)
    {
        update(torrentMap);
    }

    int Torrent::id() const
    {
        return mId;
    }

    const QString& Torrent::hashString() const
    {
        return mHashString;
    }

    const QString& Torrent::name() const
    {
        return mName;
    }

    Torrent::Status Torrent::status() const
    {
        return mStatus;
    }

    QString Torrent::statusString() const
    {
        switch (mStatus) {
        case Paused:
            return qApp->translate("tremotesf", "Paused", "Paused torrent");
        case Downloading:
#ifdef TREMOTESF_SAILFISHOS
            return qApp->translate("tremotesf", "Downloading from %n peer(s)", nullptr, mSeeders);
#endif
        case StalledDownloading:
            return qApp->translate("tremotesf", "Downloading", "Torrent that is being downloaded");
        case Seeding:
#ifdef TREMOTESF_SAILFISHOS
            return qApp->translate("tremotesf", "Seeding to %n peer(s)", nullptr, mLeechers);
#endif
        case StalledSeeding:
            return qApp->translate("tremotesf", "Seeding", "Torrent that is being seeded");
        case QueuedForDownloading:
        case QueuedForSeeding:
            return qApp->translate("tremotesf", "Queued");
        case Checking:
            return qApp->translate("tremotesf", "Checking", "Torrent that is being checked");
        case QueuedForChecking:
            return qApp->translate("tremotesf", "Queued for checking");
        case Errored:
            return mErrorString;
        }

        return QString();
    }

    int Torrent::queuePosition() const
    {
        return mQueuePosition;
    }

    long long Torrent::totalSize() const
    {
        return mTotalSize;
    }

    long long Torrent::completedSize() const
    {
        return mCompletedSize;
    }

    long long Torrent::leftUntilDone() const
    {
        return mLeftUntilDone;
    }

    long long Torrent::sizeWhenDone() const
    {
        return mSizeWhenDone;
    }

    float Torrent::percentDone() const
    {
        return mPercentDone;
    }

    float Torrent::recheckProgress() const
    {
        return mRecheckProgress;
    }

    int Torrent::eta() const
    {
        return mEta;
    }

    long long Torrent::downloadSpeed() const
    {
        return mDownloadSpeed;
    }

    long long Torrent::uploadSpeed() const
    {
        return mUploadSpeed;
    }

    bool Torrent::isDownloadSpeedLimited() const
    {
        return mDownloadSpeedLimited;
    }

    void Torrent::setDownloadSpeedLimited(bool limited)
    {
        mDownloadSpeedLimited = limited;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, downloadSpeedLimitedKey, mDownloadSpeedLimited);
    }

    int Torrent::downloadSpeedLimit() const
    {
        return mDownloadSpeedLimit;
    }

    void Torrent::setDownloadSpeedLimit(int limit)
    {
        mDownloadSpeedLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, downloadSpeedLimitKey, mRpc->serverSettings()->kiloBytesOrKibiBytes(mDownloadSpeedLimit));
    }

    bool Torrent::isUploadSpeedLimited() const
    {
        return mUploadSpeedLimited;
    }

    void Torrent::setUploadSpeedLimited(bool limited)
    {
        mUploadSpeedLimited = limited;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, uploadSpeedLimitedKey, mUploadSpeedLimited);
    }

    int Torrent::uploadSpeedLimit() const
    {
        return mUploadSpeedLimit;
    }

    void Torrent::setUploadSpeedLimit(int limit)
    {
        mUploadSpeedLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, uploadSpeedLimitKey, mRpc->serverSettings()->kiloBytesOrKibiBytes(mUploadSpeedLimit));
    }

    long long Torrent::totalDownloaded() const
    {
        return mTotalDownloaded;
    }

    long long Torrent::totalUploaded() const
    {
        return mTotalUploaded;
    }

    float Torrent::ratio() const
    {
        return mRatio;
    }

    Torrent::RatioLimitMode Torrent::ratioLimitMode() const
    {
        return mRatioLimitMode;
    }

    void Torrent::setRatioLimitMode(Torrent::RatioLimitMode mode)
    {
        mRatioLimitMode = mode;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, ratioLimitModeKey, mRatioLimitMode);
    }

    float Torrent::ratioLimit() const
    {
        return mRatioLimit;
    }

    void Torrent::setRatioLimit(float limit)
    {
        mRatioLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, ratioLimitKey, mRatioLimit);
    }

    int Torrent::seeders() const
    {
        return mSeeders;
    }

    int Torrent::leechers() const
    {
        return mLeechers;
    }

    int Torrent::peersLimit() const
    {
        return mPeersLimit;
    }

    void Torrent::setPeersLimit(int limit)
    {
        mPeersLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, peersLimitKey, mPeersLimit);
    }

    const QDateTime& Torrent::addedDate() const
    {
        return mAddedDate;
    }

    const QDateTime& Torrent::activityDate() const
    {
        return mActivityDate;
    }

    const QDateTime& Torrent::doneDate() const
    {
        return mDoneDate;
    }

    bool Torrent::honorSessionLimits() const
    {
        return mHonorSessionLimits;
    }

    void Torrent::setHonorSessionLimits(bool honor)
    {
        mHonorSessionLimits = honor;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, honorSessionLimitsKey, mHonorSessionLimits);
    }

    Torrent::Priority Torrent::bandwidthPriority() const
    {
        return mBandwidthPriority;
    }

    void Torrent::setBandwidthPriority(Torrent::Priority priority)
    {
        mBandwidthPriority = priority;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, bandwidthPriorityKey, mBandwidthPriority);
    }

    Torrent::IdleSeedingLimitMode Torrent::idleSeedingLimitMode() const
    {
        return mIdleSeedingLimitMode;
    }

    void Torrent::setIdleSeedingLimitMode(Torrent::IdleSeedingLimitMode mode)
    {
        mIdleSeedingLimitMode = mode;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, idleSeedingLimitModeKey, mIdleSeedingLimitMode);
    }

    int Torrent::idleSeedingLimit() const
    {
        return mIdleSeedingLimit;
    }

    void Torrent::setIdleSeedingLimit(int limit)
    {
        mIdleSeedingLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(mId, idleSeedingLimitKey, mIdleSeedingLimit);
    }

    const QString& Torrent::downloadDirectory() const
    {
        return mDownloadDirectory;
    }

    const QString& Torrent::creator() const
    {
        return mCreator;
    }

    const QDateTime& Torrent::creationDate() const
    {
        return mCreationDate;
    }

    const QString& Torrent::comment() const
    {
        return mComment;
    }

    bool Torrent::isFilesEnabled() const
    {
        return mFilesEnabled;
    }

    void Torrent::setFilesEnabled(bool enabled)
    {
        if (enabled != mFilesEnabled) {
            mFilesEnabled = enabled;
            if (mFilesEnabled) {
                mRpc->getTorrentFiles(mId, false);
            }
        }
    }

    bool Torrent::isFilesUpdated() const
    {
        return mFilesUpdated;
    }

    const QVariantList& Torrent::files() const
    {
        return mFiles;
    }

    const QVariantList& Torrent::filesStats() const
    {
        return mFileStats;
    }

    void Torrent::setFilesWanted(const QVariantList& files, bool wanted)
    {
        mRpc->setTorrentProperty(mId,
                                 wanted ? QStringLiteral("files-wanted")
                                        : QStringLiteral("files-unwanted"),
                                 files);
    }

    void Torrent::setFilesPriority(const QVariantList& files, TorrentFilesModelEntryEnums::Priority priority)
    {
        QString propertyName;
        switch (priority) {
        case TorrentFilesModelEntryEnums::LowPriority:
            propertyName = "priority-low";
            break;
        case TorrentFilesModelEntryEnums::NormalPriority:
            propertyName = "priority-normal";
            break;
        case TorrentFilesModelEntryEnums::HighPriority:
            propertyName = "priority-high";
            break;
        case TorrentFilesModelEntryEnums::MixedPriority:
            return;
        }
        mRpc->setTorrentProperty(mId, propertyName, files);
    }

    void Torrent::renameFile(const QString& path, const QString& newName)
    {
        mRpc->renameTorrentFile(mId, path, newName);
    }

    const QList<std::shared_ptr<Tracker>>& Torrent::trackers() const
    {
        return mTrackers;
    }

    void Torrent::addTracker(const QString& announce)
    {
        mRpc->setTorrentProperty(mId, QStringLiteral("trackerAdd"), QVariantList{announce});
    }

    void Torrent::setTracker(int trackerId, const QString& announce)
    {
        mRpc->setTorrentProperty(mId, QStringLiteral("trackerReplace"), QVariantList{trackerId, announce});
    }

    void Torrent::removeTrackers(const QVariantList& ids)
    {
        mRpc->setTorrentProperty(mId, QStringLiteral("trackerRemove"), ids);
    }

    bool Torrent::isPeersEnabled() const
    {
        return mPeersEnabled;
    }

    void Torrent::setPeersEnabled(bool enabled)
    {
        if (enabled != mPeersEnabled) {
            mPeersEnabled = enabled;
            if (mPeersEnabled) {
                mRpc->getTorrentPeers(mId, false);
            }
        }
    }

    bool Torrent::isPeersUpdated() const
    {
        return mPeersUpdated;
    }

    bool Torrent::isUpdated() const
    {
        bool updated = true;
        if (mFilesEnabled && !mFilesUpdated) {
            updated = false;
        }
        if (mPeersEnabled && !mPeersUpdated) {
            updated = false;
        }
        return updated;
    }

    void Torrent::update(const QVariantMap& torrentMap)
    {
        mName = torrentMap.value(QStringLiteral("name")).toString();

        mErrorString = torrentMap.value(QStringLiteral("errorString")).toString();
        mQueuePosition = torrentMap.value(QStringLiteral("queuePosition")).toInt();

        mTotalSize = torrentMap.value(QStringLiteral("totalSize")).toLongLong();
        mCompletedSize = torrentMap.value(QStringLiteral("haveValid")).toLongLong();
        mLeftUntilDone = torrentMap.value(QStringLiteral("leftUntilDone")).toLongLong();
        mSizeWhenDone = torrentMap.value(QStringLiteral("sizeWhenDone")).toLongLong();
        mPercentDone = torrentMap.value(QStringLiteral("percentDone")).toFloat();
        mRecheckProgress = torrentMap.value(QStringLiteral("recheckProgress")).toFloat();
        mEta = torrentMap.value(QStringLiteral("eta")).toInt();

        mDownloadSpeed = torrentMap.value(QStringLiteral("rateDownload")).toLongLong();
        mUploadSpeed = torrentMap.value(QStringLiteral("rateUpload")).toLongLong();

        mDownloadSpeedLimited = torrentMap.value(downloadSpeedLimitedKey).toBool();
        mDownloadSpeedLimit = mRpc->serverSettings()->kibiBytes(torrentMap.value(downloadSpeedLimitKey).toInt());
        mUploadSpeedLimited = torrentMap.value(uploadSpeedLimitedKey).toBool();
        mUploadSpeedLimit = mRpc->serverSettings()->kibiBytes(torrentMap.value(uploadSpeedLimitKey).toInt());

        mTotalDownloaded = torrentMap.value(QStringLiteral("downloadedEver")).toLongLong();
        mTotalUploaded = torrentMap.value(QStringLiteral("uploadedEver")).toLongLong();
        mRatio = torrentMap.value(QStringLiteral("uploadRatio")).toFloat();
        mRatioLimitMode = static_cast<RatioLimitMode>(torrentMap.value(ratioLimitModeKey).toInt());
        mRatioLimit = torrentMap.value(ratioLimitKey).toFloat();

        mSeeders = torrentMap.value(QStringLiteral("peersSendingToUs")).toInt();
        mLeechers = torrentMap.value(QStringLiteral("peersGettingFromUs")).toInt();

        const bool stalled = (mSeeders == 0 && mLeechers == 0);
        if (torrentMap.value(QStringLiteral("error")).toInt() == 0) {
            switch (torrentMap.value(QStringLiteral("status")).toInt()) {
            case 0:
                mStatus = Paused;
                break;
            case 1:
                mStatus = QueuedForChecking;
                break;
            case 2:
                mStatus = Checking;
                break;
            case 3:
                mStatus = QueuedForDownloading;
                break;
            case 4:
                if (stalled) {
                    mStatus = StalledDownloading;
                } else {
                    mStatus = Downloading;
                }
                break;
            case 5:
                mStatus = QueuedForSeeding;
                break;
            case 6:
                if (stalled) {
                    mStatus = StalledSeeding;
                } else {
                    mStatus = Seeding;
                }
            }
        } else {
            mStatus = Errored;
        }

        mPeersLimit = torrentMap.value(peersLimitKey).toInt();

        const int activityDate = torrentMap.value(QStringLiteral("activityDate")).toInt();
        if (activityDate > 0) {
            mActivityDate.setTime_t(activityDate);
        } else {
            mActivityDate = QDateTime();
        }
        const int doneDate = torrentMap.value(QStringLiteral("doneDate")).toInt();
        if (doneDate > 0) {
            mDoneDate.setTime_t(doneDate);
        } else {
            mDoneDate = QDateTime();
        }

        mHonorSessionLimits = torrentMap.value(honorSessionLimitsKey).toBool();
        mBandwidthPriority = static_cast<Priority>(torrentMap.value(bandwidthPriorityKey).toInt());
        mIdleSeedingLimitMode = static_cast<IdleSeedingLimitMode>(torrentMap.value(idleSeedingLimitModeKey).toInt());
        mIdleSeedingLimit = torrentMap.value(idleSeedingLimitKey).toInt();
        mDownloadDirectory = torrentMap.value(QStringLiteral("downloadDir")).toString();
        mCreator = torrentMap.value(QStringLiteral("creator")).toString();

        const int creationDate = torrentMap.value(QStringLiteral("dateCreated")).toInt();
        if (creationDate > 0) {
            mCreationDate.setTime_t(creationDate);
        } else {
            mCreationDate = QDateTime();
        }

        mComment = torrentMap.value(QStringLiteral("comment")).toString();

        QList<std::shared_ptr<Tracker>> trackers;
        for (const QVariant& trackerVariant : torrentMap.value(QStringLiteral("trackerStats")).toList()) {
            const QVariantMap trackerMap(trackerVariant.toMap());
            const int id = trackerMap.value(QStringLiteral("id")).toInt();

            std::shared_ptr<Tracker> tracker;
            for (const std::shared_ptr<Tracker>& existingTracker : mTrackers) {
                if (existingTracker->id() == id) {
                    tracker = existingTracker;
                    break;
                }
            }
            if (tracker) {
                tracker->update(trackerMap);
            } else {
                tracker = std::make_shared<Tracker>(id, trackerMap);
            }

            trackers.append(tracker);
        }
        mTrackers = trackers;

        mFilesUpdated = false;
        mPeersUpdated = false;

        emit updated();
    }

    void Torrent::updateFiles(const QVariantMap& torrentMap)
    {
        mFiles = torrentMap.value(QStringLiteral("files")).toList();
        mFileStats = torrentMap.value(QStringLiteral("fileStats")).toList();
        mFilesUpdated = true;
        emit filesUpdated(mFiles, mFileStats);
    }

    void Torrent::updatePeers(const QVariantMap& torrentMap)
    {
        mPeersUpdated = true;
        emit peersUpdated(torrentMap.value(QStringLiteral("peers")).toList());
    }
}
