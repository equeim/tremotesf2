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
        const QString hashStringKey(QLatin1String("hashString"));
        const QString addedDateKey(QLatin1String("addedDate"));

        const QString nameKey(QLatin1String("name"));

        const QString errorStringKey(QLatin1String("errorString"));
        const QString queuePositionKey(QLatin1String("queuePosition"));

        const QString totalSizeKey(QLatin1String("totalSize"));
        const QString completedSizeKey(QLatin1String("haveValid"));
        const QString leftUntilDoneKey(QLatin1String("leftUntilDone"));
        const QString sizeWhenDoneKey(QLatin1String("sizeWhenDone"));
        const QString percentDoneKey(QLatin1String("percentDone"));
        const QString recheckProgressKey(QLatin1String("recheckProgress"));
        const QString etaKey(QLatin1String("eta"));

        const QString downloadSpeedKey(QLatin1String("rateDownload"));
        const QString uploadSpeedKey(QLatin1String("rateUpload"));

        const QString downloadSpeedLimitedKey(QLatin1String("downloadLimited"));
        const QString downloadSpeedLimitKey(QLatin1String("downloadLimit"));
        const QString uploadSpeedLimitedKey(QLatin1String("uploadLimited"));
        const QString uploadSpeedLimitKey(QLatin1String("uploadLimit"));

        const QString totalDownloadedKey(QLatin1String("downloadedEver"));
        const QString totalUploadedKey(QLatin1String("uploadedEver"));
        const QString ratioKey(QLatin1String("uploadRatio"));
        const QString ratioLimitModeKey(QLatin1String("seedRatioMode"));
        const QString ratioLimitKey(QLatin1String("seedRatioLimit"));

        const QString seedersKey(QLatin1String("peersSendingToUs"));
        const QString leechersKey(QLatin1String("peersGettingFromUs"));

        const QString errorKey(QLatin1String("error"));
        const QString statusKey(QLatin1String("status"));

        const QString activityDateKey(QLatin1String("activityDate"));
        const QString doneDateKey(QLatin1String("doneDate"));

        const QString peersLimitKey(QLatin1String("peer-limit"));
        const QString honorSessionLimitsKey(QLatin1String("honorsSessionLimits"));
        const QString bandwidthPriorityKey(QLatin1String("bandwidthPriority"));
        const QString idleSeedingLimitModeKey(QLatin1String("seedIdleMode"));
        const QString idleSeedingLimitKey(QLatin1String("seedRatioLimit"));
        const QString downloadDirectoryKey(QLatin1String("downloadDir"));
        const QString creatorKey(QLatin1String("creator"));
        const QString creationDateKey(QLatin1String("dateCreated"));
        const QString commentKey(QLatin1String("comment"));

        const QString trackerStatsKey(QLatin1String("trackerStats"));
        const QString trackerIdKey(QLatin1String("id"));

        const QString filesKey(QLatin1String("files"));
        const QString fileStatsKey(QLatin1String("fileStats"));

        const QString peersKey(QLatin1String("peers"));

        const QString wantedFilesKey(QLatin1String("files-wanted"));
        const QString unwantedFilesKey(QLatin1String("files-unwanted"));

        const QString lowPriorityKey(QLatin1String("priority-low"));
        const QString normalPriorityKey(QLatin1String("priority-normal"));
        const QString highPriorityKey(QLatin1String("priority-high"));

        const QString addTrackerKey(QLatin1String("trackerAdd"));
        const QString replaceTrackerKey(QLatin1String("trackerReplace"));
        const QString removeTrackerKey(QLatin1String("trackerRemove"));
    }

    const QString Torrent::idKey(QLatin1String("id"));

    Torrent::Torrent(int id, const QVariantMap& torrentMap, Rpc* rpc)
        : mId(id),
          mHashString(torrentMap.value(hashStringKey).toString()),
          mAddedDate(QDateTime::fromMSecsSinceEpoch(torrentMap.value(addedDateKey).toLongLong() * 1000)),
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
            return qApp->translate("tremotesf", "Paused", "Torrent status");
        case Downloading:
#ifdef TREMOTESF_SAILFISHOS
            return qApp->translate("tremotesf", "Downloading from %n peers", nullptr, mSeeders);
#endif
        case StalledDownloading:
            return qApp->translate("tremotesf", "Downloading", "Torrent status");
        case Seeding:
#ifdef TREMOTESF_SAILFISHOS
            return qApp->translate("tremotesf", "Seeding to %n peers", nullptr, mLeechers);
#endif
        case StalledSeeding:
            return qApp->translate("tremotesf", "Seeding", "Torrent status");
        case QueuedForDownloading:
        case QueuedForSeeding:
            return qApp->translate("tremotesf", "Queued", "Torrent status");
        case Checking:
            return qApp->translate("tremotesf", "Checking", "Torrent status");
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
                                 wanted ? wantedFilesKey
                                        : unwantedFilesKey,
                                 files);
    }

    void Torrent::setFilesPriority(const QVariantList& files, TorrentFilesModelEntryEnums::Priority priority)
    {
        QString propertyName;
        switch (priority) {
        case TorrentFilesModelEntryEnums::LowPriority:
            propertyName = lowPriorityKey;
            break;
        case TorrentFilesModelEntryEnums::NormalPriority:
            propertyName = normalPriorityKey;
            break;
        case TorrentFilesModelEntryEnums::HighPriority:
            propertyName = highPriorityKey;
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
        mRpc->setTorrentProperty(mId, addTrackerKey, QVariantList{announce});
    }

    void Torrent::setTracker(int trackerId, const QString& announce)
    {
        mRpc->setTorrentProperty(mId, replaceTrackerKey, QVariantList{trackerId, announce});
    }

    void Torrent::removeTrackers(const QVariantList& ids)
    {
        mRpc->setTorrentProperty(mId, removeTrackerKey, ids);
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
        mName = torrentMap.value(nameKey).toString();

        mErrorString = torrentMap.value(errorStringKey).toString();
        mQueuePosition = torrentMap.value(queuePositionKey).toInt();

        mTotalSize = torrentMap.value(totalSizeKey).toLongLong();
        mCompletedSize = torrentMap.value(completedSizeKey).toLongLong();
        mLeftUntilDone = torrentMap.value(leftUntilDoneKey).toLongLong();
        mSizeWhenDone = torrentMap.value(sizeWhenDoneKey).toLongLong();
        mPercentDone = torrentMap.value(percentDoneKey).toFloat();
        mRecheckProgress = torrentMap.value(recheckProgressKey).toFloat();
        mEta = torrentMap.value(etaKey).toInt();

        mDownloadSpeed = torrentMap.value(downloadSpeedKey).toLongLong();
        mUploadSpeed = torrentMap.value(uploadSpeedKey).toLongLong();

        mDownloadSpeedLimited = torrentMap.value(downloadSpeedLimitedKey).toBool();
        mDownloadSpeedLimit = mRpc->serverSettings()->kibiBytes(torrentMap.value(downloadSpeedLimitKey).toInt());
        mUploadSpeedLimited = torrentMap.value(uploadSpeedLimitedKey).toBool();
        mUploadSpeedLimit = mRpc->serverSettings()->kibiBytes(torrentMap.value(uploadSpeedLimitKey).toInt());

        mTotalDownloaded = torrentMap.value(totalDownloadedKey).toLongLong();
        mTotalUploaded = torrentMap.value(totalUploadedKey).toLongLong();
        mRatio = torrentMap.value(ratioKey).toFloat();
        mRatioLimitMode = static_cast<RatioLimitMode>(torrentMap.value(ratioLimitModeKey).toInt());
        mRatioLimit = torrentMap.value(ratioLimitKey).toFloat();

        mSeeders = torrentMap.value(seedersKey).toInt();
        mLeechers = torrentMap.value(leechersKey).toInt();

        const bool stalled = (mSeeders == 0 && mLeechers == 0);
        if (torrentMap.value(errorKey).toInt() == 0) {
            switch (torrentMap.value(statusKey).toInt()) {
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

        const long long activityDate = torrentMap.value(activityDateKey).toLongLong() * 1000;
        if (activityDate > 0) {
            mActivityDate.setMSecsSinceEpoch(activityDate);
        } else {
            mActivityDate = QDateTime();
        }
        const long long doneDate = torrentMap.value(doneDateKey).toLongLong() * 1000;
        if (doneDate > 0) {
            mDoneDate.setMSecsSinceEpoch(doneDate);
        } else {
            mDoneDate = QDateTime();
        }

        mHonorSessionLimits = torrentMap.value(honorSessionLimitsKey).toBool();
        mBandwidthPriority = static_cast<Priority>(torrentMap.value(bandwidthPriorityKey).toInt());
        mIdleSeedingLimitMode = static_cast<IdleSeedingLimitMode>(torrentMap.value(idleSeedingLimitModeKey).toInt());
        mIdleSeedingLimit = torrentMap.value(idleSeedingLimitKey).toInt();
        mDownloadDirectory = torrentMap.value(downloadDirectoryKey).toString();
        mCreator = torrentMap.value(creatorKey).toString();

        const long long creationDate = torrentMap.value(creationDateKey).toLongLong() * 1000;
        if (creationDate > 0) {
            mCreationDate.setMSecsSinceEpoch(creationDate);
        } else {
            mCreationDate = QDateTime();
        }

        mComment = torrentMap.value(commentKey).toString();

        QList<std::shared_ptr<Tracker>> trackers;
        for (const QVariant& trackerVariant : torrentMap.value(trackerStatsKey).toList()) {
            const QVariantMap trackerMap(trackerVariant.toMap());
            const int id = trackerMap.value(trackerIdKey).toInt();

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
        mFiles = torrentMap.value(filesKey).toList();
        mFileStats = torrentMap.value(fileStatsKey).toList();
        mFilesUpdated = true;
        emit filesUpdated(mFiles, mFileStats);
    }

    void Torrent::updatePeers(const QVariantMap& torrentMap)
    {
        mPeersUpdated = true;
        emit peersUpdated(torrentMap.value(peersKey).toList());
    }
}
