/*
 * Libtremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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
#include "stdutils.h"
#include "tracker.h"

namespace libtremotesf
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
        const QString prioritiesKey(QLatin1String("priorities"));
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

        template<typename T>
        void setChanged(T& value, const T& newValue, bool& changed)
        {
            if (newValue != value) {
                value = newValue;
                changed = true;
            }
        }

        void setChanged(double& value, const double& newValue, bool& changed)
        {
            if (!qFuzzyCompare(newValue, value)) {
                value = newValue;
                changed = true;
            }
        }
    }

    const QString Torrent::idKey(QLatin1String("id"));

    TorrentFile::TorrentFile(const std::vector<QString>& path, long long size)
        : path(path),
          size(size),
          changed(false)
    {

    }

    Peer::Peer(const QString& address, const QVariantMap& peerMap)
        : address(address)
    {
        update(peerMap);
    }

    void Peer::update(const QVariantMap& peerMap)
    {
        downloadSpeed = peerMap["rateToClient"].toLongLong();
        uploadSpeed = peerMap["rateToPeer"].toLongLong();
        progress = peerMap["progress"].toDouble();
        flags = peerMap["flagStr"].toString();
        client = peerMap["clientName"].toString();
    }

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

    QString Torrent::errorString() const
    {
        return mErrorString;
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

    double Torrent::percentDone() const
    {
        return mPercentDone;
    }

    bool Torrent::isFinished() const
    {
        return mLeftUntilDone == 0;
    }

    double Torrent::recheckProgress() const
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
        mRpc->setTorrentProperty(mId, downloadSpeedLimitKey, mRpc->serverSettings()->fromKibiBytes(mDownloadSpeedLimit));
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
        mRpc->setTorrentProperty(mId, uploadSpeedLimitKey, mRpc->serverSettings()->fromKibiBytes(mUploadSpeedLimit));
    }

    long long Torrent::totalDownloaded() const
    {
        return mTotalDownloaded;
    }

    long long Torrent::totalUploaded() const
    {
        return mTotalUploaded;
    }

    double Torrent::ratio() const
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

    double Torrent::ratioLimit() const
    {
        return mRatioLimit;
    }

    void Torrent::setRatioLimit(double limit)
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

    bool Torrent::isSingleFile() const
    {
        return mSingleFile;
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

    bool Torrent::isFilesLoaded() const
    {
        return mFilesLoaded;
    }

    void Torrent::setFilesEnabled(bool enabled)
    {
        if (enabled != mFilesEnabled) {
            mFilesEnabled = enabled;
            if (mFilesEnabled) {
                mRpc->getTorrentFiles(mId, false);
            } else {
                mFiles.clear();
                mFilesLoaded = false;
            }
        }
    }

    bool Torrent::isFilesUpdated() const
    {
        return mFilesUpdated;
    }

    const std::vector<std::shared_ptr<TorrentFile>>& Torrent::files() const
    {
        return mFiles;
    }

    void Torrent::setFilesWanted(const QVariantList& files, bool wanted)
    {
        mRpc->setTorrentProperty(mId,
                                 wanted ? wantedFilesKey
                                        : unwantedFilesKey,
                                 files);
    }

    void Torrent::setFilesPriority(const QVariantList& files, TorrentFile::Priority priority)
    {
        QString propertyName;
        switch (priority) {
        case TorrentFile::LowPriority:
            propertyName = lowPriorityKey;
            break;
        case TorrentFile::NormalPriority:
            propertyName = normalPriorityKey;
            break;
        case TorrentFile::HighPriority:
            propertyName = highPriorityKey;
            break;
        }
        mRpc->setTorrentProperty(mId, propertyName, files);
    }

    void Torrent::renameFile(const QString& path, const QString& newName)
    {
        mRpc->renameTorrentFile(mId, path, newName);
    }

    const std::vector<std::shared_ptr<Tracker>>& Torrent::trackers() const
    {
        return mTrackers;
    }

    void Torrent::addTracker(const QString& announce)
    {
        mRpc->setTorrentProperty(mId, addTrackerKey, QVariantList{announce}, true);
    }

    void Torrent::setTracker(int trackerId, const QString& announce)
    {
        mRpc->setTorrentProperty(mId, replaceTrackerKey, QVariantList{trackerId, announce}, true);
    }

    void Torrent::removeTrackers(const QVariantList& ids)
    {
        mRpc->setTorrentProperty(mId, removeTrackerKey, ids, true);
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
            } else {
                mPeers.clear();
                mPeersLoaded = false;
            }
        }
    }

    bool Torrent::isPeersLoaded() const
    {
        return mPeersLoaded;
    }

    bool Torrent::isPeersUpdated() const
    {
        return mPeersUpdated;
    }

    const std::vector<std::shared_ptr<Peer>>& Torrent::peers() const
    {
        return mPeers;
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
        mChanged = false;

        setChanged(mName, torrentMap[nameKey].toString(), mChanged);

        setChanged(mErrorString, torrentMap[errorStringKey].toString(), mChanged);
        setChanged(mQueuePosition, torrentMap[queuePositionKey].toInt(), mChanged);
        setChanged(mTotalSize, torrentMap[totalSizeKey].toLongLong(), mChanged);
        setChanged(mCompletedSize, torrentMap[completedSizeKey].toLongLong(), mChanged);
        setChanged(mLeftUntilDone, torrentMap[leftUntilDoneKey].toLongLong(), mChanged);
        setChanged(mSizeWhenDone, torrentMap[sizeWhenDoneKey].toLongLong(), mChanged);
        setChanged(mPercentDone, torrentMap[percentDoneKey].toDouble(), mChanged);
        setChanged(mRecheckProgress, torrentMap[recheckProgressKey].toDouble(), mChanged);
        setChanged(mEta, torrentMap[etaKey].toInt(), mChanged);

        setChanged(mDownloadSpeed, torrentMap[downloadSpeedKey].toLongLong(), mChanged);
        setChanged(mUploadSpeed, torrentMap[uploadSpeedKey].toLongLong(), mChanged);

        setChanged(mDownloadSpeedLimited, torrentMap[downloadSpeedLimitedKey].toBool(), mChanged);
        setChanged(mDownloadSpeedLimit, mRpc->serverSettings()->toKibiBytes(torrentMap[downloadSpeedLimitKey].toInt()), mChanged);
        setChanged(mUploadSpeedLimited, torrentMap[uploadSpeedLimitedKey].toBool(), mChanged);
        setChanged(mUploadSpeedLimit, mRpc->serverSettings()->toKibiBytes(torrentMap[uploadSpeedLimitKey].toInt()), mChanged);

        setChanged(mTotalDownloaded, torrentMap[totalDownloadedKey].toLongLong(), mChanged);
        setChanged(mTotalUploaded, torrentMap[totalUploadedKey].toLongLong(), mChanged);
        setChanged(mRatio, torrentMap[ratioKey].toDouble(), mChanged);
        setChanged(mRatioLimitMode, static_cast<RatioLimitMode>(torrentMap[ratioLimitModeKey].toInt()), mChanged);
        setChanged(mRatioLimit, torrentMap[ratioLimitKey].toDouble(), mChanged);

        setChanged(mSeeders, torrentMap[seedersKey].toInt(), mChanged);
        setChanged(mLeechers, torrentMap[leechersKey].toInt(), mChanged);

        const bool stalled = (mSeeders == 0 && mLeechers == 0);
        if (torrentMap[errorKey].toInt() == 0) {
            switch (torrentMap[statusKey].toInt()) {
            case 0:
                setChanged(mStatus, Paused, mChanged);
                break;
            case 1:
                setChanged(mStatus, QueuedForChecking, mChanged);
                break;
            case 2:
                setChanged(mStatus, Checking, mChanged);
                break;
            case 3:
                setChanged(mStatus, QueuedForDownloading, mChanged);
                break;
            case 4:
                if (stalled) {
                    setChanged(mStatus, StalledDownloading, mChanged);
                } else {
                    setChanged(mStatus, Downloading, mChanged);
                }
                break;
            case 5:
                setChanged(mStatus, QueuedForSeeding, mChanged);
                break;
            case 6:
                if (stalled) {
                    setChanged(mStatus, StalledSeeding, mChanged);
                } else {
                    setChanged(mStatus, Seeding, mChanged);
                }
            }
        } else {
            setChanged(mStatus, Errored, mChanged);
        }

        setChanged(mPeersLimit, torrentMap[peersLimitKey].toInt(), mChanged);

        const long long activityDate = torrentMap[activityDateKey].toLongLong() * 1000;
        if (activityDate > 0) {
            if (activityDate != mActivityDate.toMSecsSinceEpoch()) {
                mActivityDate.setMSecsSinceEpoch(activityDate);
                mChanged = true;
            }
        } else {
            if (!mActivityDate.isNull()) {
                mActivityDate = QDateTime();
                mChanged = true;
            }
        }
        const long long doneDate = torrentMap[doneDateKey].toLongLong() * 1000;
        if (doneDate > 0) {
            if (doneDate != mDoneDate.toMSecsSinceEpoch()) {
                mDoneDate.setMSecsSinceEpoch(doneDate);
                mChanged = true;
            }
        } else {
            if (!mDoneDate.isNull()) {
                mDoneDate = QDateTime();
                mChanged = true;
            }
        }

        setChanged(mHonorSessionLimits, torrentMap[honorSessionLimitsKey].toBool(), mChanged);
        setChanged(mBandwidthPriority, static_cast<Priority>(torrentMap[bandwidthPriorityKey].toInt()), mChanged);
        setChanged(mIdleSeedingLimitMode, static_cast<IdleSeedingLimitMode>(torrentMap[idleSeedingLimitModeKey].toInt()), mChanged);
        setChanged(mIdleSeedingLimit, torrentMap[idleSeedingLimitKey].toInt(), mChanged);
        setChanged(mDownloadDirectory, torrentMap[downloadDirectoryKey].toString(), mChanged);
        setChanged(mSingleFile, torrentMap[prioritiesKey].toList().size() == 1, mChanged);
        setChanged(mCreator, torrentMap[creatorKey].toString(), mChanged);

        const long long creationDate = torrentMap[creationDateKey].toLongLong() * 1000;
        if (creationDate > 0) {
            if (creationDate != mCreationDate.toMSecsSinceEpoch()) {
                mCreationDate.setMSecsSinceEpoch(creationDate);
                mChanged = true;
            }
        } else {
            if (!mCreationDate.isNull()) {
                mCreationDate = QDateTime();
                mChanged = true;
            }
        }

        setChanged(mComment, torrentMap[commentKey].toString(), mChanged);

        std::vector<std::shared_ptr<Tracker>> trackers;
        for (const QVariant& trackerVariant : torrentMap[trackerStatsKey].toList()) {
            const QVariantMap trackerMap(trackerVariant.toMap());
            const int id = trackerMap[trackerIdKey].toInt();

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

            trackers.push_back(std::move(tracker));
        }
        mTrackers = std::move(trackers);

        mFilesUpdated = false;
        mPeersUpdated = false;

        emit updated();
    }

    void Torrent::updateFiles(const QVariantMap& torrentMap)
    {
        const QVariantList files(torrentMap[filesKey].toList());
        const QVariantList fileStats(torrentMap[fileStatsKey].toList());

        if (!files.isEmpty()) {
            const bool empty = mFiles.empty();
            for (int i = 0, max = files.size(); i < max; ++i) {
                const QVariantMap fileMap(files[i].toMap());
                const QVariantMap fileStatsMap(fileStats[i].toMap());
                if (empty) {
                    std::vector<QString> path;
                    for (QString& part : fileMap["name"].toString().split('/', QString::SkipEmptyParts)) {
                        path.push_back(std::move(part));
                    }
                    mFiles.push_back(std::make_shared<TorrentFile>(std::move(path), fileMap["length"].toLongLong()));
                }
                TorrentFile* file = mFiles[i].get();
                file->changed = false;
                setChanged(file->completedSize, fileStatsMap["bytesCompleted"].toLongLong(), file->changed);
                setChanged(file->wanted, fileStatsMap["wanted"].toBool(), file->changed);
                setChanged(file->priority, static_cast<TorrentFile::Priority>(fileStatsMap["priority"].toInt()), file->changed);
            }
        }

        mFilesUpdated = true;
        mFilesLoaded = true;
        emit filesUpdated(mFiles);
    }

    void Torrent::updatePeers(const QVariantMap& torrentMap)
    {
        const QVariantList peers(torrentMap[peersKey].toList());

        std::vector<QString> addresses;
        for (const QVariant& peer : peers) {
            addresses.push_back(peer.toMap()["address"].toString());
        }

        for (std::size_t i = 0, max = mPeers.size(); i < max; ++i) {
            if (!tremotesf::contains(addresses, mPeers[i]->address)) {
                mPeers.erase(mPeers.begin() + i);
                i--;
                max--;
            }
        }

        for (const QVariant& peerVariant : peers) {
            const QVariantMap peerMap(peerVariant.toMap());
            const QString address(peerMap["address"].toString());
            int row = -1;
            for (int i = 0, max = mPeers.size(); i < max; ++i) {
                if (mPeers[i]->address == address) {
                    row = i;
                    break;
                }
            }
            if (row == -1) {
                mPeers.push_back(std::make_shared<Peer>(address, peerMap));
            } else {
                mPeers[row]->update(peerMap);
            }
        }

        mPeersUpdated = true;
        mPeersLoaded = true;
        emit peersUpdated(mPeers);
    }

    bool Torrent::isChanged() const
    {
        return mChanged;
    }
}
