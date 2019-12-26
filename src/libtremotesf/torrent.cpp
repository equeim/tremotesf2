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

#include <type_traits>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QLocale>

#include "rpc.h"
#include "serversettings.h"
#include "stdutils.h"
#include "tracker.h"

namespace libtremotesf
{
    namespace
    {
        const auto hashStringKey(QJsonKeyStringInit("hashString"));
        const auto addedDateKey(QJsonKeyStringInit("addedDate"));

        const auto nameKey(QJsonKeyStringInit("name"));

        const auto errorStringKey(QJsonKeyStringInit("errorString"));
        const auto queuePositionKey(QJsonKeyStringInit("queuePosition"));

        const auto totalSizeKey(QJsonKeyStringInit("totalSize"));
        const auto completedSizeKey(QJsonKeyStringInit("haveValid"));
        const auto leftUntilDoneKey(QJsonKeyStringInit("leftUntilDone"));
        const auto sizeWhenDoneKey(QJsonKeyStringInit("sizeWhenDone"));
        const auto percentDoneKey(QJsonKeyStringInit("percentDone"));
        const auto recheckProgressKey(QJsonKeyStringInit("recheckProgress"));
        const auto etaKey(QJsonKeyStringInit("eta"));

        const auto downloadSpeedKey(QJsonKeyStringInit("rateDownload"));
        const auto uploadSpeedKey(QJsonKeyStringInit("rateUpload"));

        const auto downloadSpeedLimitedKey(QJsonKeyStringInit("downloadLimited"));
        const auto downloadSpeedLimitKey(QJsonKeyStringInit("downloadLimit"));
        const auto uploadSpeedLimitedKey(QJsonKeyStringInit("uploadLimited"));
        const auto uploadSpeedLimitKey(QJsonKeyStringInit("uploadLimit"));

        const auto totalDownloadedKey(QJsonKeyStringInit("downloadedEver"));
        const auto totalUploadedKey(QJsonKeyStringInit("uploadedEver"));
        const auto ratioKey(QJsonKeyStringInit("uploadRatio"));
        const auto ratioLimitModeKey(QJsonKeyStringInit("seedRatioMode"));
        const auto ratioLimitKey(QJsonKeyStringInit("seedRatioLimit"));

        const auto seedersKey(QJsonKeyStringInit("peersSendingToUs"));
        const auto leechersKey(QJsonKeyStringInit("peersGettingFromUs"));

        const auto errorKey(QJsonKeyStringInit("error"));
        const auto statusKey(QJsonKeyStringInit("status"));

        const auto activityDateKey(QJsonKeyStringInit("activityDate"));
        const auto doneDateKey(QJsonKeyStringInit("doneDate"));

        const auto peersLimitKey(QJsonKeyStringInit("peer-limit"));
        const auto honorSessionLimitsKey(QJsonKeyStringInit("honorsSessionLimits"));
        const auto bandwidthPriorityKey(QJsonKeyStringInit("bandwidthPriority"));
        const auto idleSeedingLimitModeKey(QJsonKeyStringInit("seedIdleMode"));
        const auto idleSeedingLimitKey(QJsonKeyStringInit("seedRatioLimit"));
        const auto downloadDirectoryKey(QJsonKeyStringInit("downloadDir"));
        const auto prioritiesKey(QJsonKeyStringInit("priorities"));
        const auto creatorKey(QJsonKeyStringInit("creator"));
        const auto creationDateKey(QJsonKeyStringInit("dateCreated"));
        const auto commentKey(QJsonKeyStringInit("comment"));

        const QLatin1String wantedFilesKey("files-wanted");
        const QLatin1String unwantedFilesKey("files-unwanted");

        const QLatin1String lowPriorityKey("priority-low");
        const QLatin1String normalPriorityKey("priority-normal");
        const QLatin1String highPriorityKey("priority-high");

        const QLatin1String addTrackerKey("trackerAdd");
        const QLatin1String replaceTrackerKey("trackerReplace");
        const QLatin1String removeTrackerKey("trackerRemove");

        template<typename T, typename std::enable_if<std::is_scalar<T>::value && !std::is_floating_point<T>::value, int>::type = 0>
        void setChanged(T& value, T newValue, bool& changed)
        {
            if (newValue != value) {
                value = newValue;
                changed = true;
            }
        }

        template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
        void setChanged(T& value, T newValue, bool& changed)
        {
            if (!qFuzzyCompare(newValue, value)) {
                value = newValue;
                changed = true;
            }
        }

        template<typename T, typename std::enable_if<!std::is_scalar<T>::value, int>::type = 0>
        void setChanged(T& value, T&& newValue, bool& changed)
        {
            if (newValue != value) {
                value = std::forward<T>(newValue);
                changed = true;
            }
        }
    }

    const QJsonKeyString Torrent::idKey(QJsonKeyStringInit("id"));

    TorrentFile::TorrentFile(const QJsonObject& fileMap, const QJsonObject& fileStatsMap)
        : size(fileMap.value(QJsonKeyStringInit("length")).toDouble())
    {
        QStringList p(fileMap.value(QJsonKeyStringInit("name")).toString().split(QLatin1Char('/'), QString::SkipEmptyParts));
        path.reserve(p.size());
        for (QString& part : p) {
            path.push_back(std::move(part));
        }
        update(fileStatsMap);
    }

    bool TorrentFile::update(const QJsonObject& fileStatsMap)
    {
        changed = false;
        setChanged(completedSize, static_cast<long long>(fileStatsMap.value(QJsonKeyStringInit("bytesCompleted")).toDouble()), changed);
        setChanged(wanted, fileStatsMap.value(QJsonKeyStringInit("wanted")).toBool(), changed);
        setChanged(priority, [&]() {
            switch (int priority = fileStatsMap.value(QJsonKeyStringInit("priority")).toInt()) {
            case TorrentFile::LowPriority:
            case TorrentFile::NormalPriority:
            case TorrentFile::HighPriority:
                return static_cast<TorrentFile::Priority>(priority);
            default:
                return TorrentFile::NormalPriority;
            }
        }(), changed);

        return changed;
     }

    Peer::Peer(QString&& address, const QJsonObject& peerMap)
        : address(std::move(address))
    {
        update(peerMap);
    }

    void Peer::update(const QJsonObject& peerMap)
    {
        downloadSpeed = peerMap.value(QJsonKeyStringInit("rateToClient")).toDouble();
        uploadSpeed = peerMap.value(QJsonKeyStringInit("rateToPeer")).toDouble();
        progress = peerMap.value(QJsonKeyStringInit("progress")).toDouble();
        flags = peerMap.value(QJsonKeyStringInit("flagStr")).toString();
        client = peerMap.value(QJsonKeyStringInit("clientName")).toString();
    }

    Torrent::Torrent(int id, const QJsonObject& torrentMap, Rpc* rpc)
        : mId(id),
          mHashString(torrentMap.value(hashStringKey).toString()),
          mAddedDate(QDateTime::fromMSecsSinceEpoch(torrentMap.value(addedDateKey).toDouble() * 1000)),
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

    const std::vector<TorrentFile>& Torrent::files() const
    {
        return mFiles;
    }

    bool Torrent::isFilesChanged()
    {
        return mFilesChanged;
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
        QLatin1String propertyName;
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

    const std::vector<Tracker>& Torrent::trackers() const
    {
        return mTrackers;
    }

    bool Torrent::isTrackersAddedOrRemoved() const
    {
        return mTrackersAddedOrRemoved;
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

    const std::vector<Peer>& Torrent::peers() const
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

    void Torrent::update(const QJsonObject& torrentMap)
    {
        mChanged = false;

        setChanged(mName, torrentMap.value(nameKey).toString(), mChanged);

        setChanged(mErrorString, torrentMap.value(errorStringKey).toString(), mChanged);
        setChanged(mQueuePosition, torrentMap.value(queuePositionKey).toInt(), mChanged);
        setChanged(mTotalSize, static_cast<long long>(torrentMap.value(totalSizeKey).toDouble()), mChanged);
        setChanged(mCompletedSize, static_cast<long long>(torrentMap.value(completedSizeKey).toDouble()), mChanged);
        setChanged(mLeftUntilDone, static_cast<long long>(torrentMap.value(leftUntilDoneKey).toDouble()), mChanged);
        setChanged(mSizeWhenDone, static_cast<long long>(torrentMap.value(sizeWhenDoneKey).toDouble()), mChanged);
        setChanged(mPercentDone, torrentMap.value(percentDoneKey).toDouble(), mChanged);
        setChanged(mRecheckProgress, torrentMap.value(recheckProgressKey).toDouble(), mChanged);
        setChanged(mEta, torrentMap.value(etaKey).toInt(), mChanged);

        setChanged(mDownloadSpeed, static_cast<long long>(torrentMap.value(downloadSpeedKey).toDouble()), mChanged);
        setChanged(mUploadSpeed, static_cast<long long>(torrentMap.value(uploadSpeedKey).toDouble()), mChanged);

        setChanged(mDownloadSpeedLimited, torrentMap.value(downloadSpeedLimitedKey).toBool(), mChanged);
        setChanged(mDownloadSpeedLimit, mRpc->serverSettings()->toKibiBytes(torrentMap.value(downloadSpeedLimitKey).toInt()), mChanged);
        setChanged(mUploadSpeedLimited, torrentMap.value(uploadSpeedLimitedKey).toBool(), mChanged);
        setChanged(mUploadSpeedLimit, mRpc->serverSettings()->toKibiBytes(torrentMap.value(uploadSpeedLimitKey).toInt()), mChanged);

        setChanged(mTotalDownloaded, static_cast<long long>(torrentMap.value(totalDownloadedKey).toDouble()), mChanged);
        setChanged(mTotalUploaded, static_cast<long long>(torrentMap.value(totalUploadedKey).toDouble()), mChanged);
        setChanged(mRatio, torrentMap.value(ratioKey).toDouble(), mChanged);

        setChanged(mRatioLimitMode, [&]() {
            switch (int mode = torrentMap.value(ratioLimitModeKey).toInt()) {
            case GlobalRatioLimit:
            case SingleRatioLimit:
            case UnlimitedRatio:
                return static_cast<RatioLimitMode>(mode);
            default:
                return GlobalRatioLimit;
            }
        }(), mChanged);
        setChanged(mRatioLimit, torrentMap.value(ratioLimitKey).toDouble(), mChanged);

        setChanged(mSeeders, torrentMap.value(seedersKey).toInt(), mChanged);
        setChanged(mLeechers, torrentMap.value(leechersKey).toInt(), mChanged);

        const bool stalled = (mSeeders == 0 && mLeechers == 0);
        if (torrentMap.value(errorKey).toInt() == 0) {
            switch (torrentMap.value(statusKey).toInt()) {
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

        setChanged(mPeersLimit, torrentMap.value(peersLimitKey).toInt(), mChanged);

        const long long activityDate = torrentMap.value(activityDateKey).toDouble() * 1000;
        if (activityDate > 0) {
            if (activityDate != mActivityDateTime) {
                mActivityDateTime = activityDate;
                mActivityDate.setMSecsSinceEpoch(activityDate);
                mChanged = true;
            }
        } else {
            if (!mActivityDate.isNull()) {
                mActivityDateTime = -1;
                mActivityDate = QDateTime();
                mChanged = true;
            }
        }
        const long long doneDate = torrentMap.value(doneDateKey).toDouble() * 1000;
        if (doneDate > 0) {
            if (doneDate != mDoneDateTime) {
                mDoneDateTime = doneDate;
                mDoneDate.setMSecsSinceEpoch(doneDate);
                mChanged = true;
            }
        } else {
            if (!mDoneDate.isNull()) {
                mDoneDateTime = -1;
                mDoneDate = QDateTime();
                mChanged = true;
            }
        }

        setChanged(mHonorSessionLimits, torrentMap.value(honorSessionLimitsKey).toBool(), mChanged);
        setChanged(mBandwidthPriority, [&]() {
            switch (int priority = torrentMap.value(bandwidthPriorityKey).toInt()) {
            case LowPriority:
            case NormalPriority:
            case HighPriority:
                return static_cast<Priority>(priority);
            default:
                return NormalPriority;
            }
        }(), mChanged);
        setChanged(mIdleSeedingLimitMode, [&]() {
            switch (int mode = torrentMap.value(idleSeedingLimitModeKey).toInt()) {
            case GlobalIdleSeedingLimit:
            case SingleIdleSeedingLimit:
            case UnlimitedIdleSeeding:
                return static_cast<IdleSeedingLimitMode>(mode);
            default:
                return GlobalIdleSeedingLimit;
            }
        }(), mChanged);
        setChanged(mIdleSeedingLimit, torrentMap.value(idleSeedingLimitKey).toInt(), mChanged);
        setChanged(mDownloadDirectory, torrentMap.value(downloadDirectoryKey).toString(), mChanged);
        setChanged(mSingleFile, torrentMap.value(prioritiesKey).toArray().size() == 1, mChanged);
        setChanged(mCreator, torrentMap.value(creatorKey).toString(), mChanged);

        const long long creationDate = torrentMap.value(creationDateKey).toDouble() * 1000;
        if (creationDate > 0) {
            if (creationDate != mCreationDateTime) {
                mCreationDateTime = creationDate;
                mCreationDate.setMSecsSinceEpoch(creationDate);
                mChanged = true;
            }
        } else {
            if (!mCreationDate.isNull()) {
                mCreationDateTime = -1;
                mCreationDate = QDateTime();
                mChanged = true;
            }
        }

        setChanged(mComment, torrentMap.value(commentKey).toString(), mChanged);

        mTrackersAddedOrRemoved = false;
        std::vector<Tracker> trackers;
        const QJsonArray trackersJson(torrentMap.value(QJsonKeyStringInit("trackerStats")).toArray());
        trackers.reserve(trackersJson.size());
        for (const QJsonValue& trackerVariant : trackersJson) {
            const QJsonObject trackerMap(trackerVariant.toObject());
            const int id = trackerMap.value(QJsonKeyStringInit("id")).toInt();

            const auto found(std::find_if(mTrackers.begin(), mTrackers.end(), [&](const Tracker& tracker) {
                return tracker.id() == id;
            }));

            if (found == mTrackers.end()) {
                trackers.emplace_back(id, trackerMap);
                mTrackersAddedOrRemoved = true;
            } else {
                found->update(trackerMap);
                trackers.push_back(std::move(*found));
            }
        }
        if (trackers.size() != mTrackers.size()) {
            mTrackersAddedOrRemoved = true;
        }
        mTrackers = std::move(trackers);

        mFilesUpdated = false;
        mPeersUpdated = false;

        emit updated();
    }

    void Torrent::updateFiles(const QJsonObject& torrentMap)
    {
        mFilesChanged = false;

        const QJsonArray fileStats(torrentMap.value(QJsonKeyStringInit("fileStats")).toArray());
        if (!fileStats.isEmpty()) {
            if (mFiles.empty()) {
                mFilesChanged = true;
                const QJsonArray files(torrentMap.value(QJsonKeyStringInit("files")).toArray());
                mFiles.reserve(fileStats.size());
                for (int i = 0, max = fileStats.size(); i < max; ++i) {
                    mFiles.emplace_back(files[i].toObject(), fileStats[i].toObject());
                }
            } else {
                for (int i = 0, max = fileStats.size(); i < max; ++i) {
                    if (mFiles[i].update(fileStats[i].toObject())) {
                        mFilesChanged = true;
                    }
                }
            }
        }

        mFilesUpdated = true;
        mFilesLoaded = true;
        emit filesUpdated(mFiles);
    }

    void Torrent::updatePeers(const QJsonObject& torrentMap)
    {
        std::vector<QString> addresses;
        const std::vector<QJsonObject> peers([&]() {
            std::vector<QJsonObject> p;
            const QJsonArray peerValues(torrentMap.value(QJsonKeyStringInit("peers")).toArray());
            p.reserve(peerValues.size());
            addresses.reserve(peerValues.size());

            for (const QJsonValue& peerValue : peerValues) {
                QJsonObject peerMap(peerValue.toObject());
                addresses.push_back(peerMap.value(QJsonKeyStringInit("address")).toString());
                p.push_back(std::move(peerMap));
            }

            return p;
        }());

        for (int i = mPeers.size() - 1; i >= 0; --i) {
            if (!tremotesf::contains(addresses, mPeers[i].address)) {
                mPeers.erase(mPeers.begin() + i);
            }
        }

        mPeers.reserve(peers.size());

        for (size_t i = 0, max = peers.size(); i < max; ++i) {
            const QJsonObject& peerMap = peers[i];
            QString& address = addresses[i];

            const auto found(std::find_if(mPeers.begin(), mPeers.end(), [&](const Peer& peer) {
                return peer.address == address;
            }));

            if (found == mPeers.end()) {
                mPeers.emplace_back(std::move(address), peerMap);
            } else {
                found->update(peerMap);
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
