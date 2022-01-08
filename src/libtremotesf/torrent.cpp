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

#include "itemlistupdater.h"
#include "rpc.h"
#include "serversettings.h"
#include "stdutils.h"
#include "torrent_qdebug.h"

namespace libtremotesf
{
    namespace
    {
        const auto hashStringKey(QJsonKeyStringInit("hashString"));
        const auto addedDateKey(QJsonKeyStringInit("addedDate"));

        const auto nameKey(QJsonKeyStringInit("name"));

        const auto magnetLinkKey(QJsonKeyStringInit("magnetLink"));

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

        const auto webSeedersKey(QJsonKeyStringInit("webseeds"));
        const auto activeWebSeedersKey(QJsonKeyStringInit("webseedsSendingToUs"));

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

    bool TorrentData::update(const QJsonObject& torrentMap)
    {
        bool changed = false;

        setChanged(name, torrentMap.value(nameKey).toString(), changed);
        setChanged(magnetLink, torrentMap.value(magnetLinkKey).toString(), changed);

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
        setChanged(downloadSpeedLimit, torrentMap.value(downloadSpeedLimitKey).toInt(), changed);
        setChanged(uploadSpeedLimited, torrentMap.value(uploadSpeedLimitedKey).toBool(), changed);
        setChanged(uploadSpeedLimit, torrentMap.value(uploadSpeedLimitKey).toInt(), changed);

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

        const bool hasActivePeers = (seeders != 0 || activeWebSeeders != 0 || leechers != 0);

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
                if (hasActivePeers) {
                    setChanged(status, Downloading, changed);
                } else {
                    setChanged(status, StalledDownloading, changed);
                }
                break;
            case 5:
                setChanged(status, QueuedForSeeding, changed);
                break;
            case 6:
                if (hasActivePeers) {
                    setChanged(status, Seeding, changed);
                } else {
                    setChanged(status, StalledSeeding, changed);
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

        trackersAnnounceUrlsChanged = false;
        std::vector<Tracker> newTrackers;
        const QJsonArray trackerJsons(torrentMap.value(QJsonKeyStringInit("trackerStats")).toArray());
        newTrackers.reserve(static_cast<size_t>(trackerJsons.size()));
        for (const auto& i : trackerJsons) {
            const QJsonObject trackerMap(i.toObject());
            const int trackerId = trackerMap.value(QJsonKeyStringInit("id")).toInt();

            const auto found(std::find_if(trackers.begin(), trackers.end(), [&](const auto& tracker) {
                return tracker.id() == trackerId;
            }));

            if (found == trackers.end()) {
                newTrackers.emplace_back(trackerId, trackerMap);
                trackersAnnounceUrlsChanged = true;
            } else {
                const auto result = found->update(trackerMap);
                if (result.changed) {
                    changed = true;
                }
                if (result.announceUrlChanged) {
                    trackersAnnounceUrlsChanged = true;
                }
                newTrackers.push_back(std::move(*found));
            }
        }
        if (newTrackers.size() != trackers.size()) {
            trackersAnnounceUrlsChanged = true;
        }
        if (trackersAnnounceUrlsChanged) {
            changed = true;
        }
        trackers = std::move(newTrackers);

        setChanged(activeWebSeeders, torrentMap.value(activeWebSeedersKey).toInt(), changed);
        {
            std::vector<QString> newWebSeeders;
            const auto webSeedersStrings = torrentMap.value(webSeedersKey).toArray();
            newWebSeeders.reserve(static_cast<size_t>(webSeedersStrings.size()));
            std::transform(webSeedersStrings.begin(), webSeedersStrings.end(), std::back_insert_iterator(newWebSeeders), [](const QJsonValue& i) { return i.toString(); });
            setChanged(webSeeders, std::move(newWebSeeders), changed);
        }

        return changed;
    }

    Torrent::Torrent(int id, const QJsonObject& torrentMap, Rpc* rpc, QObject* parent)
        : QObject(parent), mRpc(rpc)
    {
        mData.id = id;
        mData.hashString = torrentMap.value(hashStringKey).toString();
        const auto date = static_cast<long long>(torrentMap.value(addedDateKey).toDouble()) * 1000;
        mData.addedDate = QDateTime::fromMSecsSinceEpoch(date);
        mData.addedDateTime = date;
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
        mRpc->setTorrentProperty(id(), downloadSpeedLimitKey, limit);
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
        mRpc->setTorrentProperty(id(), uploadSpeedLimitKey, limit);
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

    bool Torrent::isTrackersAnnounceUrlsChanged() const
    {
        return mData.trackersAnnounceUrlsChanged;
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

    const std::vector<QString>& Torrent::webSeeders() const
    {
        return mData.webSeeders;
    }

    int Torrent::activeWebSeeders() const
    {
        return mData.activeWebSeeders;
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
                mRpc->getTorrentsFiles({id()}, false);
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
                mRpc->getTorrentsPeers({id()}, false);
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

    void Torrent::checkThatFilesUpdated()
    {
        if (mFilesEnabled && !mFilesUpdated) {
            qWarning() << "Warning: files were not updated for" << *this;
            mFilesUpdated = true;
        }
    }

    void Torrent::checkThatPeersUpdated()
    {
        if (mPeersEnabled && !mPeersUpdated) {
            qWarning() << "Warning: peers were not updated for" << *this;
            mPeersUpdated = true;
        }
    }

    bool Torrent::update(const QJsonObject& torrentMap)
    {
        mFilesUpdated = false;
        mPeersUpdated = false;
        const bool c = mData.update(torrentMap);
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
                for (QJsonArray::size_type i = 0, max = fileStats.size(); i < max; ++i) {
                    mFiles.emplace_back(i, fileJsons[i].toObject(), fileStats[i].toObject());
                    changed.push_back(static_cast<int>(i));
                }
            } else {
                for (QJsonArray::size_type i = 0, max = fileStats.size(); i < max; ++i) {
                    TorrentFile& file = mFiles[static_cast<size_t>(i)];
                    if (file.update(fileStats[i].toObject())) {
                        changed.push_back(static_cast<int>(i));
                    }
                }
            }
        }

        mFilesUpdated = true;

        emit filesUpdated(changed);
        emit mRpc->torrentFilesUpdated(this, changed);
    }

    namespace {
        using NewPeer = std::pair<QJsonObject, QString>;

        class PeersListUpdater : public ItemListUpdater<Peer, NewPeer, std::vector<NewPeer>> {
        public:
            std::vector<std::pair<int, int>> removedIndexRanges;
            std::vector<std::pair<int, int>> changedIndexRanges;
            int addedCount = 0;

        protected:
            std::vector<NewPeer>::iterator
            findNewItemForItem(std::vector<NewPeer> &newPeers, const Peer &peer) override {
                const auto &address = peer.address;
                return std::find_if(newPeers.begin(), newPeers.end(),
                                    [address](const auto &newPeer) {
                                        const auto&[json, newPeerAddress] = newPeer;
                                        return newPeerAddress == address;
                                    });
            }

            void onAboutToRemoveItems(size_t, size_t) override {};

            void onRemovedItems(size_t first, size_t last) override {
                removedIndexRanges.emplace_back(static_cast<int>(first), static_cast<int>(last));
            }

            bool updateItem(Peer &peer, NewPeer &&newPeer) override {
                const auto&[json, address] = newPeer;
                return peer.update(json);
            }

            void onChangedItems(size_t first, size_t last) override {
                changedIndexRanges.emplace_back(static_cast<int>(first), static_cast<int>(last));
            }

            Peer createItemFromNewItem(NewPeer &&newPeer) override {
                auto&[json, address] = newPeer;
                return Peer(std::move(address), json);
            }

            void onAboutToAddItems(size_t) override {}

            void onAddedItems(size_t count) override {
                addedCount = static_cast<int>(count);
            };
        };
    }

    void Torrent::updatePeers(const QJsonObject &torrentMap)
    {
        std::vector<NewPeer> newPeers;
        {
            const QJsonArray peers(torrentMap.value(QJsonKeyStringInit("peers")).toArray());
            newPeers.reserve(static_cast<size_t>(peers.size()));
            for (const auto& i : peers) {
                QJsonObject json = i.toObject();
                QString address(json.value(Peer::addressKey).toString());
                newPeers.emplace_back(std::move(json), std::move(address));
            }
        }

        PeersListUpdater updater;
        updater.update(mPeers, std::move(newPeers));

        mPeersUpdated = true;

        emit peersUpdated(updater.removedIndexRanges, updater.changedIndexRanges, updater.addedCount);
        emit mRpc->torrentPeersUpdated(this, updater.removedIndexRanges, updater.changedIndexRanges, updater.addedCount);
    }

    void Torrent::checkSingleFile(const QJsonObject& torrentMap)
    {
        mData.singleFile = (torrentMap.value(prioritiesKey).toArray().size() == 1);
    }
}
