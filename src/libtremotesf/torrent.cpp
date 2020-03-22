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

        const auto metadataCompleteKey(QJsonKeyStringInit("metadataPercentComplete"));

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
        const auto idleSeedingLimitKey(QJsonKeyStringInit("seedIdleLimit"));
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
    }

    const QJsonKeyString Torrent::idKey(QJsonKeyStringInit("id"));

    bool TorrentData::update(const QJsonObject& torrentMap, const Rpc* rpc)
    {
        bool changed = false;

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

        setChanged(metadataComplete, torrentMap.value(metadataCompleteKey).toInt() == 1, changed);

        setChanged(downloadSpeed, static_cast<long long>(torrentMap.value(downloadSpeedKey).toDouble()), changed);
        setChanged(uploadSpeed, static_cast<long long>(torrentMap.value(uploadSpeedKey).toDouble()), changed);

        setChanged(downloadSpeedLimited, torrentMap.value(downloadSpeedLimitedKey).toBool(), changed);
        setChanged(downloadSpeedLimit, rpc->serverSettings()->toKibiBytes(torrentMap.value(downloadSpeedLimitKey).toInt()), changed);
        setChanged(uploadSpeedLimited, torrentMap.value(uploadSpeedLimitedKey).toBool(), changed);
        setChanged(uploadSpeedLimit, rpc->serverSettings()->toKibiBytes(torrentMap.value(uploadSpeedLimitKey).toInt()), changed);

        setChanged(totalDownloaded, static_cast<long long>(torrentMap.value(totalDownloadedKey).toDouble()), changed);
        setChanged(totalUploaded, static_cast<long long>(torrentMap.value(totalUploadedKey).toDouble()), changed);
        setChanged(ratio, torrentMap.value(ratioKey).toDouble(), changed);

        setChanged(ratioLimitMode, [&] {
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

        const auto newActivityDateTime = static_cast<long long>(torrentMap.value(activityDateKey).toDouble()) * 1000;
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
        const auto newDoneDateTime = static_cast<long long>(torrentMap.value(doneDateKey).toDouble()) * 1000;
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
        setChanged(bandwidthPriority, [&] {
            switch (int priority = torrentMap.value(bandwidthPriorityKey).toInt()) {
            case LowPriority:
            case NormalPriority:
            case HighPriority:
                return static_cast<Priority>(priority);
            default:
                return NormalPriority;
            }
        }(), changed);
        setChanged(idleSeedingLimitMode, [&] {
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
        setChanged(creator, torrentMap.value(creatorKey).toString(), changed);

        const auto newCreationDateTime = static_cast<long long>(torrentMap.value(creationDateKey).toDouble()) * 1000;
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
        newTrackers.reserve(static_cast<size_t>(trackerJsons.size()));
        for (const QJsonValue& trackerJson : trackerJsons) {
            const QJsonObject trackerMap(trackerJson.toObject());
            const int trackerId = trackerMap.value(QJsonKeyStringInit("id")).toInt();

            const auto found(std::find_if(trackers.begin(), trackers.end(), [&](const Tracker& tracker) {
                return tracker.id() == trackerId;
            }));

            if (found == trackers.end()) {
                newTrackers.emplace_back(id, trackerMap);
                trackersAddedOrRemoved = true;
            } else {
                if (found->update(trackerMap)) {
                    changed = true;
                }
                newTrackers.push_back(std::move(*found));
            }
        }
        if (newTrackers.size() != trackers.size()) {
            trackersAddedOrRemoved = true;
        }
        if (trackersAddedOrRemoved) {
            changed = true;
        }
        trackers = std::move(newTrackers);

        return changed;
    }

    Torrent::Torrent(int id, const QJsonObject& torrentMap, Rpc* rpc)
        : mRpc(rpc)
    {
        mData.id = id;
        mData.hashString = torrentMap.value(hashStringKey).toString();
        mData.addedDate = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(torrentMap.value(addedDateKey).toDouble()) * 1000);
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

    bool Torrent::isMetadataComplete() const
    {
        return mData.metadataComplete;
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
        addTrackers(QStringList{announce});
    }

    void Torrent::addTrackers(const std::vector<QString>& announceUrls)
    {
        QStringList list;
        list.reserve(static_cast<int>(announceUrls.size()));
        for (const QString& url : announceUrls) {
            list.push_back(url);
        }
        addTrackers(list);
    }

    void Torrent::addTrackers(const QStringList& announceUrls)
    {
        mRpc->setTorrentProperty(id(), addTrackerKey, announceUrls, true);
    }

    void Torrent::setTracker(int trackerId, const QString& announce)
    {
        mRpc->setTorrentProperty(id(), replaceTrackerKey, QVariantList{trackerId, announce}, true);
    }

    void Torrent::removeTrackers(const QVariantList& ids)
    {
        mRpc->setTorrentProperty(id(), removeTrackerKey, ids, true);
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

    bool Torrent::update(const QJsonObject& torrentMap)
    {
        mFilesUpdated = false;
        mPeersUpdated = false;
        const bool c = mData.update(torrentMap, mRpc);
        emit updated();
        if (c) {
            emit changed();
        }
        return c;
    }

    void Torrent::updateFiles(const QJsonObject &torrentMap)
    {
        std::vector<int> changed;

        const QJsonArray fileStats(torrentMap.value(QJsonKeyStringInit("fileStats")).toArray());
        if (!fileStats.isEmpty()) {
            if (mFiles.empty()) {
                const QJsonArray fileJsons(torrentMap.value(QJsonKeyStringInit("files")).toArray());
                mFiles.reserve(static_cast<size_t>(fileStats.size()));
                changed.reserve(static_cast<size_t>(fileStats.size()));
                for (int i = 0, max = fileStats.size(); i < max; ++i) {
                    mFiles.emplace_back(i, fileJsons[i].toObject(), fileStats[i].toObject());
                    changed.push_back(i);
                }
            } else {
                for (int i = 0, max = fileStats.size(); i < max; ++i) {
                    TorrentFile& file = mFiles[static_cast<size_t>(i)];
                    if (file.update(fileStats[i].toObject())) {
                        changed.push_back(i);
                    }
                }
            }
        }

        mFilesUpdated = true;

        emit filesUpdated(changed);
        emit mRpc->torrentFilesUpdated(this, changed);
    }

    void Torrent::updatePeers(const QJsonObject &torrentMap)
    {
        std::vector<std::tuple<QJsonObject, QString, bool>> newPeers;
        {
            const QJsonArray peerJsons(torrentMap.value(QJsonKeyStringInit("peers")).toArray());
            newPeers.reserve(static_cast<size_t>(peerJsons.size()));
            for (const QJsonValue& peerValue : peerJsons) {
                QJsonObject peerJson(peerValue.toObject());
                QString address(peerJson.value(Peer::addressKey).toString());
                newPeers.emplace_back(std::move(peerJson), std::move(address), false);
            }
        }

        std::vector<int> removed;
        if (newPeers.size() < mPeers.size()) {
            removed.reserve(mPeers.size() - newPeers.size());
        }
        std::vector<int> changed;
        {
            const auto newPeersBegin(newPeers.begin());
            const auto newPeersEnd(newPeers.end());
            VectorBatchRemover<Peer> remover(mPeers, &removed, &changed);
            for (int i = static_cast<int>(mPeers.size()) - 1; i >= 0; --i) {
                Peer& peer = mPeers[static_cast<size_t>(i)];
                const auto found(std::find_if(newPeersBegin, newPeersEnd, [&peer](const auto& p) {
                    return std::get<1>(p) == peer.address;
                }));
                if (found == newPeersEnd) {
                    remover.remove(i);
                } else {
                    std::get<2>(*found) = true;
                    if (peer.update(std::get<0>(*found))) {
                        changed.push_back(i);
                    }
                }
            }
            remover.doRemove();
        }
        std::reverse(changed.begin(), changed.end());

        int added = 0;
        if (newPeers.size() > mPeers.size()) {
            added = static_cast<int>(newPeers.size() - mPeers.size());
            mPeers.reserve(newPeers.size());
            for (auto& p : newPeers) {
                const QJsonObject& peerJson = std::get<0>(p);
                QString& address = std::get<1>(p);
                const bool existing = std::get<2>(p);
                if (!existing) {
                    mPeers.emplace_back(std::move(address), peerJson);
                }
            }
        }

        mPeersUpdated = true;

        emit peersUpdated(removed, changed, added);
        emit mRpc->torrentPeersUpdated(this, removed, changed, added);
    }

    void Torrent::checkSingleFile(const QJsonObject& torrentMap)
    {
        mData.singleFile = (torrentMap.value(prioritiesKey).toArray().size() == 1);
    }
}
