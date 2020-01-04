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
        setChanged(wanted, fileStatsMap.value(QJsonKeyStringInit("wanted")).toBool(), changed);

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

    void TorrentData::update(const QJsonObject& torrentMap, const Rpc* rpc)
    {
        changed = false;

        setChanged(name, torrentMap.value(nameKey).toString(), changed);

        setChanged(errorString, torrentMap.value(errorStringKey).toString(), changed);
        setChanged(queuePosition, torrentMap.value(queuePositionKey).toInt(), changed);
        setChanged(totalSize, static_cast<long long>(torrentMap.value(totalSizeKey).toDouble()), changed);
        setChanged(completedSize, static_cast<long long>(torrentMap.value(completedSizeKey).toDouble()), changed);
        setChanged(leftUntilDone, static_cast<long long>(torrentMap.value(leftUntilDoneKey).toDouble()), changed);
        setChanged(sizeWhenDone, static_cast<long long>(torrentMap.value(sizeWhenDoneKey).toDouble()), changed);
        setChanged(percentDone, torrentMap.value(percentDoneKey).toDouble(), changed);
        setChanged(recheckProgress, torrentMap.value(recheckProgressKey).toDouble(), changed);
        setChanged(eta, torrentMap.value(etaKey).toInt(), changed);

        setChanged(downloadSpeed, static_cast<long long>(torrentMap.value(downloadSpeedKey).toDouble()), changed);
        setChanged(uploadSpeed, static_cast<long long>(torrentMap.value(uploadSpeedKey).toDouble()), changed);

        setChanged(downloadSpeedLimited, torrentMap.value(downloadSpeedLimitedKey).toBool(), changed);
        setChanged(downloadSpeedLimit, rpc->serverSettings()->toKibiBytes(torrentMap.value(downloadSpeedLimitKey).toInt()), changed);
        setChanged(uploadSpeedLimited, torrentMap.value(uploadSpeedLimitedKey).toBool(), changed);
        setChanged(uploadSpeedLimit, rpc->serverSettings()->toKibiBytes(torrentMap.value(uploadSpeedLimitKey).toInt()), changed);

        setChanged(totalDownloaded, static_cast<long long>(torrentMap.value(totalDownloadedKey).toDouble()), changed);
        setChanged(totalUploaded, static_cast<long long>(torrentMap.value(totalUploadedKey).toDouble()), changed);
        setChanged(ratio, torrentMap.value(ratioKey).toDouble(), changed);

        setChanged(ratioLimitMode, [&]() {
            switch (int mode = torrentMap.value(ratioLimitModeKey).toInt()) {
            case GlobalRatioLimit:
            case SingleRatioLimit:
            case UnlimitedRatio:
                return static_cast<RatioLimitMode>(mode);
            default:
                return GlobalRatioLimit;
            }
        }(), changed);
        setChanged(ratioLimit, torrentMap.value(ratioLimitKey).toDouble(), changed);

        setChanged(seeders, torrentMap.value(seedersKey).toInt(), changed);
        setChanged(leechers, torrentMap.value(leechersKey).toInt(), changed);

        const bool stalled = (seeders == 0 && leechers == 0);
        if (torrentMap.value(errorKey).toInt() == 0) {
            switch (torrentMap.value(statusKey).toInt()) {
            case 0:
                setChanged(status, Paused, changed);
                break;
            case 1:
                setChanged(status, QueuedForChecking, changed);
                break;
            case 2:
                setChanged(status, Checking, changed);
                break;
            case 3:
                setChanged(status, QueuedForDownloading, changed);
                break;
            case 4:
                if (stalled) {
                    setChanged(status, StalledDownloading, changed);
                } else {
                    setChanged(status, Downloading, changed);
                }
                break;
            case 5:
                setChanged(status, QueuedForSeeding, changed);
                break;
            case 6:
                if (stalled) {
                    setChanged(status, StalledSeeding, changed);
                } else {
                    setChanged(status, Seeding, changed);
                }
            }
        } else {
            setChanged(status, Errored, changed);
        }

        setChanged(peersLimit, torrentMap.value(peersLimitKey).toInt(), changed);

        const long long newActivityDateTime = torrentMap.value(activityDateKey).toDouble() * 1000;
        if (newActivityDateTime > 0) {
            if (newActivityDateTime != activityDateTime) {
                activityDateTime = newActivityDateTime;
                activityDate.setMSecsSinceEpoch(newActivityDateTime);
                changed = true;
            }
        } else {
            if (!activityDate.isNull()) {
                activityDateTime = -1;
                activityDate = QDateTime();
                changed = true;
            }
        }
        const long long newDoneDateTime = torrentMap.value(doneDateKey).toDouble() * 1000;
        if (newDoneDateTime > 0) {
            if (newDoneDateTime != doneDateTime) {
                doneDateTime = newDoneDateTime;
                doneDate.setMSecsSinceEpoch(newDoneDateTime);
                changed = true;
            }
        } else {
            if (!doneDate.isNull()) {
                doneDateTime = -1;
                doneDate = QDateTime();
                changed = true;
            }
        }

        setChanged(honorSessionLimits, torrentMap.value(honorSessionLimitsKey).toBool(), changed);
        setChanged(bandwidthPriority, [&]() {
            switch (int priority = torrentMap.value(bandwidthPriorityKey).toInt()) {
            case LowPriority:
            case NormalPriority:
            case HighPriority:
                return static_cast<Priority>(priority);
            default:
                return NormalPriority;
            }
        }(), changed);
        setChanged(idleSeedingLimitMode, [&]() {
            switch (int mode = torrentMap.value(idleSeedingLimitModeKey).toInt()) {
            case GlobalIdleSeedingLimit:
            case SingleIdleSeedingLimit:
            case UnlimitedIdleSeeding:
                return static_cast<IdleSeedingLimitMode>(mode);
            default:
                return GlobalIdleSeedingLimit;
            }
        }(), changed);
        setChanged(idleSeedingLimit, torrentMap.value(idleSeedingLimitKey).toInt(), changed);
        setChanged(downloadDirectory, torrentMap.value(downloadDirectoryKey).toString(), changed);
        setChanged(singleFile, torrentMap.value(prioritiesKey).toArray().size() == 1, changed);
        setChanged(creator, torrentMap.value(creatorKey).toString(), changed);

        const long long newCreationDateTime = torrentMap.value(creationDateKey).toDouble() * 1000;
        if (newCreationDateTime > 0) {
            if (newCreationDateTime != creationDateTime) {
                creationDateTime = newCreationDateTime;
                creationDate.setMSecsSinceEpoch(newCreationDateTime);
                changed = true;
            }
        } else {
            if (!creationDate.isNull()) {
                creationDateTime = -1;
                creationDate = QDateTime();
                changed = true;
            }
        }

        setChanged(comment, torrentMap.value(commentKey).toString(), changed);

        trackersAddedOrRemoved = false;
        std::vector<Tracker> newTrackers;
        const QJsonArray trackerJsons(torrentMap.value(QJsonKeyStringInit("trackerStats")).toArray());
        newTrackers.reserve(trackerJsons.size());
        for (const QJsonValue& trackerJson : trackerJsons) {
            const QJsonObject trackerMap(trackerJson.toObject());
            const int id = trackerMap.value(QJsonKeyStringInit("id")).toInt();

            const auto found(std::find_if(newTrackers.begin(), newTrackers.end(), [&](const Tracker& tracker) {
                return tracker.id() == id;
            }));

            if (found == newTrackers.end()) {
                newTrackers.emplace_back(id, trackerMap);
                trackersAddedOrRemoved = true;
            } else {
                found->update(trackerMap);
                newTrackers.push_back(std::move(*found));
            }
        }
        if (newTrackers.size() != trackers.size()) {
            trackersAddedOrRemoved = true;
        }
        trackers = std::move(newTrackers);
    }

    Torrent::Torrent(int id, const QJsonObject& torrentMap, Rpc* rpc)
        : mRpc(rpc)
    {
        mData.id = id;
        mData.hashString = torrentMap.value(hashStringKey).toString();
        mData.addedDate = QDateTime::fromMSecsSinceEpoch(torrentMap.value(addedDateKey).toDouble() * 1000);
        update(torrentMap);
    }

    int Torrent::id() const
    {
        return mData.id;
    }

    const QString& Torrent::hashString() const
    {
        return mData.hashString;
    }

    const QString& Torrent::name() const
    {
        return mData.name;
    }

    Torrent::Status Torrent::status() const
    {
        return mData.status;
    }

    QString Torrent::errorString() const
    {
        return mData.errorString;
    }

    int Torrent::queuePosition() const
    {
        return mData.queuePosition;
    }

    long long Torrent::totalSize() const
    {
        return mData.totalSize;
    }

    long long Torrent::completedSize() const
    {
        return mData.completedSize;
    }

    long long Torrent::leftUntilDone() const
    {
        return mData.leftUntilDone;
    }

    long long Torrent::sizeWhenDone() const
    {
        return mData.sizeWhenDone;
    }

    double Torrent::percentDone() const
    {
        return mData.percentDone;
    }

    bool Torrent::isFinished() const
    {
        return mData.leftUntilDone == 0;
    }

    double Torrent::recheckProgress() const
    {
        return mData.recheckProgress;
    }

    int Torrent::eta() const
    {
        return mData.eta;
    }

    long long Torrent::downloadSpeed() const
    {
        return mData.downloadSpeed;
    }

    long long Torrent::uploadSpeed() const
    {
        return mData.uploadSpeed;
    }

    bool Torrent::isDownloadSpeedLimited() const
    {
        return mData.downloadSpeedLimited;
    }

    void Torrent::setDownloadSpeedLimited(bool limited)
    {
        mData.downloadSpeedLimited = limited;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), downloadSpeedLimitedKey, limited);
    }

    int Torrent::downloadSpeedLimit() const
    {
        return mData.downloadSpeedLimit;
    }

    void Torrent::setDownloadSpeedLimit(int limit)
    {
        mData.downloadSpeedLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), downloadSpeedLimitKey, mRpc->serverSettings()->fromKibiBytes(limit));
    }

    bool Torrent::isUploadSpeedLimited() const
    {
        return mData.uploadSpeedLimited;
    }

    void Torrent::setUploadSpeedLimited(bool limited)
    {
        mData.uploadSpeedLimited = limited;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), uploadSpeedLimitedKey, limited);
    }

    int Torrent::uploadSpeedLimit() const
    {
        return mData.uploadSpeedLimit;
    }

    void Torrent::setUploadSpeedLimit(int limit)
    {
        mData.uploadSpeedLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), uploadSpeedLimitKey, mRpc->serverSettings()->fromKibiBytes(limit));
    }

    long long Torrent::totalDownloaded() const
    {
        return mData.totalDownloaded;
    }

    long long Torrent::totalUploaded() const
    {
        return mData.totalUploaded;
    }

    double Torrent::ratio() const
    {
        return mData.ratio;
    }

    Torrent::RatioLimitMode Torrent::ratioLimitMode() const
    {
        return mData.ratioLimitMode;
    }

    void Torrent::setRatioLimitMode(Torrent::RatioLimitMode mode)
    {
        mData.ratioLimitMode = mode;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), ratioLimitModeKey, mode);
    }

    double Torrent::ratioLimit() const
    {
        return mData.ratioLimit;
    }

    void Torrent::setRatioLimit(double limit)
    {
        mData.ratioLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), ratioLimitKey, limit);
    }

    int Torrent::seeders() const
    {
        return mData.seeders;
    }

    int Torrent::leechers() const
    {
        return mData.leechers;
    }

    int Torrent::peersLimit() const
    {
        return mData.peersLimit;
    }

    void Torrent::setPeersLimit(int limit)
    {
        mData.peersLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), peersLimitKey, limit);
    }

    const QDateTime& Torrent::addedDate() const
    {
        return mData.addedDate;
    }

    const QDateTime& Torrent::activityDate() const
    {
        return mData.activityDate;
    }

    const QDateTime& Torrent::doneDate() const
    {
        return mData.doneDate;
    }

    bool Torrent::honorSessionLimits() const
    {
        return mData.honorSessionLimits;
    }

    void Torrent::setHonorSessionLimits(bool honor)
    {
        mData.honorSessionLimits = honor;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), honorSessionLimitsKey, honor);
    }

    Torrent::Priority Torrent::bandwidthPriority() const
    {
        return mData.bandwidthPriority;
    }

    void Torrent::setBandwidthPriority(Priority priority)
    {
        mData.bandwidthPriority = priority;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), bandwidthPriorityKey, priority);
    }

    Torrent::IdleSeedingLimitMode Torrent::idleSeedingLimitMode() const
    {
        return mData.idleSeedingLimitMode;
    }

    void Torrent::setIdleSeedingLimitMode(Torrent::IdleSeedingLimitMode mode)
    {
        mData.idleSeedingLimitMode = mode;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), idleSeedingLimitModeKey, mode);
    }

    int Torrent::idleSeedingLimit() const
    {
        return mData.idleSeedingLimit;
    }

    void Torrent::setIdleSeedingLimit(int limit)
    {
        mData.idleSeedingLimit = limit;
        emit limitsEdited();
        mRpc->setTorrentProperty(id(), idleSeedingLimitKey, limit);
    }

    const QString& Torrent::downloadDirectory() const
    {
        return mData.downloadDirectory;
    }

    bool Torrent::isSingleFile() const
    {
        return mData.singleFile;
    }

    const QString& Torrent::creator() const
    {
        return mData.creator;
    }

    const QDateTime& Torrent::creationDate() const
    {
        return mData.creationDate;
    }

    const QString& Torrent::comment() const
    {
        return mData.comment;
    }

    const std::vector<Tracker>& Torrent::trackers() const
    {
        return mData.trackers;
    }

    bool Torrent::isTrackersAddedOrRemoved() const
    {
        return mData.trackersAddedOrRemoved;
    }

    void Torrent::addTracker(const QString& announce)
    {
        mRpc->setTorrentProperty(id(), addTrackerKey, QVariantList{announce}, true);
    }

    void Torrent::setTracker(int trackerId, const QString& announce)
    {
        mRpc->setTorrentProperty(id(), replaceTrackerKey, QVariantList{trackerId, announce}, true);
    }

    void Torrent::removeTrackers(const QVariantList& ids)
    {
        mRpc->setTorrentProperty(id(), removeTrackerKey, ids, true);
    }

    bool Torrent::isChanged() const
    {
        return mData.changed;
    }

    const TorrentData& Torrent::data() const
    {
        return mData;
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
                mRpc->getTorrentFiles(id(), false);
            } else {
                mFiles.clear();
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
        mRpc->setTorrentProperty(id(),
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
        mRpc->setTorrentProperty(id(), propertyName, files);
    }

    void Torrent::renameFile(const QString& path, const QString& newName)
    {
        mRpc->renameTorrentFile(id(), path, newName);
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
                mRpc->getTorrentPeers(id(), false);
            } else {
                mPeers.clear();
            }
        }
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
        mData.update(torrentMap, mRpc);
        mFilesUpdated = false;
        mPeersUpdated = false;
        emit updated();
    }

    void Torrent::updateFiles(const QJsonObject &torrentMap)
    {
        mFilesChanged = false;

        const QJsonArray fileStats(torrentMap.value(QJsonKeyStringInit("fileStats")).toArray());
        if (!fileStats.isEmpty()) {
            if (mFiles.empty()) {
                mFilesChanged = true;
                const QJsonArray fileJsons(torrentMap.value(QJsonKeyStringInit("files")).toArray());
                mFiles.reserve(fileStats.size());
                for (int i = 0, max = fileStats.size(); i < max; ++i) {
                    mFiles.emplace_back(fileJsons[i].toObject(), fileStats[i].toObject());
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
        emit filesUpdated(mFiles);
    }

    void Torrent::updatePeers(const QJsonObject &torrentMap)
    {
        std::vector<QString> addresses;
        const std::vector<QJsonObject> peerJsons([&]() {
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

        mPeers.reserve(peerJsons.size());

        for (size_t i = 0, max = peerJsons.size(); i < max; ++i) {
            const QJsonObject& peerJson = peerJsons[i];
            QString& address = addresses[i];

            const auto found(std::find_if(mPeers.begin(), mPeers.end(), [&](const Peer& peer) {
                return peer.address == address;
            }));

            if (found == mPeers.end()) {
                mPeers.emplace_back(std::move(address), peerJson);
            } else {
                found->update(peerJson);
            }
        }

        mPeersUpdated = true;
        emit peersUpdated(mPeers);
    }
}
