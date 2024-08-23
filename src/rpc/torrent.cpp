// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <map>
#include <ranges>
#include <stdexcept>

#include <QJsonArray>
#include <QJsonObject>

#include <fmt/ranges.h>

#include "rpc.h"
#include "serversettings.h"
#include "torrent.h"

#include "log/log.h"
#include "jsonutils.h"
#include "itemlistupdater.h"
#include "pathutils.h"
#include "stdutils.h"

namespace tremotesf {
    using namespace impl;

    enum class TorrentData::UpdateKey {
        Id,
        HashString,
        AddedDate,
        Name,
        MagnetLink,
        QueuePosition,
        TotalSize,
        CompletedSize,
        LeftUntilDone,
        SizeWhenDone,
        PercentDone,
        RecheckProgress,
        Eta,
        MetadataPercentComplete,
        DownloadSpeed,
        UploadSpeed,
        DownloadSpeedLimited,
        DownloadSpeedLimit,
        UploadSpeedLimited,
        UploadSpeedLimit,
        TotalDownloaded,
        TotalUploaded,
        Ratio,
        RatioLimitMode,
        RatioLimit,
        PeersSendingToUsCount,
        PeersGettingFromUsCount,
        WebSeeders,
        WebSeedersSendingToUsCount,
        Status,
        Error,
        ErrorString,
        ActivityDate,
        DoneDate,
        PeersLimit,
        HonorSessionLimits,
        BandwidthPriority,
        IdleSeedingLimitMode,
        IdleSeedingLimit,
        DownloadDirectory,
        Creator,
        CreationDate,
        Comment,
        TrackerStats,
        Count
    };

    namespace {
        constexpr QLatin1String updateKeyString(TorrentData::UpdateKey key) {
            switch (key) {
            case TorrentData::UpdateKey::Id:
                return "id"_l1;
            case TorrentData::UpdateKey::HashString:
                return "hashString"_l1;
            case TorrentData::UpdateKey::AddedDate:
                return "addedDate"_l1;
            case TorrentData::UpdateKey::Name:
                return "name"_l1;
            case TorrentData::UpdateKey::MagnetLink:
                return "magnetLink"_l1;
            case TorrentData::UpdateKey::QueuePosition:
                return "queuePosition"_l1;
            case TorrentData::UpdateKey::TotalSize:
                return "totalSize"_l1;
            case TorrentData::UpdateKey::CompletedSize:
                return "haveValid"_l1;
            case TorrentData::UpdateKey::LeftUntilDone:
                return "leftUntilDone"_l1;
            case TorrentData::UpdateKey::SizeWhenDone:
                return "sizeWhenDone"_l1;
            case TorrentData::UpdateKey::PercentDone:
                return "percentDone"_l1;
            case TorrentData::UpdateKey::RecheckProgress:
                return "recheckProgress"_l1;
            case TorrentData::UpdateKey::Eta:
                return "eta"_l1;
            case TorrentData::UpdateKey::MetadataPercentComplete:
                return "metadataPercentComplete"_l1;
            case TorrentData::UpdateKey::DownloadSpeed:
                return "rateDownload"_l1;
            case TorrentData::UpdateKey::UploadSpeed:
                return "rateUpload"_l1;
            case TorrentData::UpdateKey::DownloadSpeedLimited:
                return "downloadLimited"_l1;
            case TorrentData::UpdateKey::DownloadSpeedLimit:
                return "downloadLimit"_l1;
            case TorrentData::UpdateKey::UploadSpeedLimited:
                return "uploadLimited"_l1;
            case TorrentData::UpdateKey::UploadSpeedLimit:
                return "uploadLimit"_l1;
            case TorrentData::UpdateKey::TotalDownloaded:
                return "downloadedEver"_l1;
            case TorrentData::UpdateKey::TotalUploaded:
                return "uploadedEver"_l1;
            case TorrentData::UpdateKey::Ratio:
                return "uploadRatio"_l1;
            case TorrentData::UpdateKey::RatioLimitMode:
                return "seedRatioMode"_l1;
            case TorrentData::UpdateKey::RatioLimit:
                return "seedRatioLimit"_l1;
            case TorrentData::UpdateKey::PeersSendingToUsCount:
                return "peersSendingToUs"_l1;
            case TorrentData::UpdateKey::PeersGettingFromUsCount:
                return "peersGettingFromUs"_l1;
            case TorrentData::UpdateKey::WebSeeders:
                return "webseeds"_l1;
            case TorrentData::UpdateKey::WebSeedersSendingToUsCount:
                return "webseedsSendingToUs"_l1;
            case TorrentData::UpdateKey::Status:
                return "status"_l1;
            case TorrentData::UpdateKey::Error:
                return "error"_l1;
            case TorrentData::UpdateKey::ErrorString:
                return "errorString"_l1;
            case TorrentData::UpdateKey::ActivityDate:
                return "activityDate"_l1;
            case TorrentData::UpdateKey::DoneDate:
                return "doneDate"_l1;
            case TorrentData::UpdateKey::PeersLimit:
                return "peer-limit"_l1;
            case TorrentData::UpdateKey::HonorSessionLimits:
                return "honorsSessionLimits"_l1;
            case TorrentData::UpdateKey::BandwidthPriority:
                return "bandwidthPriority"_l1;
            case TorrentData::UpdateKey::IdleSeedingLimitMode:
                return "seedIdleMode"_l1;
            case TorrentData::UpdateKey::IdleSeedingLimit:
                return "seedIdleLimit"_l1;
            case TorrentData::UpdateKey::DownloadDirectory:
                return "downloadDir"_l1;
            case TorrentData::UpdateKey::Creator:
                return "creator"_l1;
            case TorrentData::UpdateKey::CreationDate:
                return "dateCreated"_l1;
            case TorrentData::UpdateKey::Comment:
                return "comment"_l1;
            case TorrentData::UpdateKey::TrackerStats:
                return "trackerStats"_l1;
            case TorrentData::UpdateKey::Count:
                return {};
            }
            return {};
        }

        std::optional<TorrentData::UpdateKey> mapUpdateKey(const QString& stringKey) {
            static const auto mapping = [] {
                std::map<QLatin1String, TorrentData::UpdateKey, std::less<>> map{};
                for (int i = 0; i < static_cast<int>(TorrentData::UpdateKey::Count); ++i) {
                    const auto key = static_cast<TorrentData::UpdateKey>(i);
                    map.emplace(updateKeyString(key), key);
                }
                return map;
            }();
            const auto foundKey = mapping.find(stringKey);
            if (foundKey == mapping.end()) {
                warning().log("Unknown torrent field '{}'", stringKey);
                return {};
            }
            return static_cast<TorrentData::UpdateKey>(foundKey->second);
        }

        constexpr auto prioritiesKey = "priorities"_l1;
        constexpr auto wantedFilesKey = "files-wanted"_l1;
        constexpr auto unwantedFilesKey = "files-unwanted"_l1;

        constexpr auto lowPriorityKey = "priority-low"_l1;
        constexpr auto normalPriorityKey = "priority-normal"_l1;
        constexpr auto highPriorityKey = "priority-high"_l1;

        constexpr auto addTrackerKey = "trackerAdd"_l1;
        constexpr auto replaceTrackerKey = "trackerReplace"_l1;
        constexpr auto removeTrackerKey = "trackerRemove"_l1;
        constexpr auto trackerListKey = "trackerList"_l1;

        constexpr auto statusMapper = EnumMapper(std::array{
            EnumMapping(TorrentData::Status::Paused, 0),
            EnumMapping(TorrentData::Status::QueuedForChecking, 1),
            EnumMapping(TorrentData::Status::Checking, 2),
            EnumMapping(TorrentData::Status::QueuedForDownloading, 3),
            EnumMapping(TorrentData::Status::Downloading, 4),
            EnumMapping(TorrentData::Status::QueuedForSeeding, 5),
            EnumMapping(TorrentData::Status::Seeding, 6)
        });

        constexpr auto errorMapper = EnumMapper(std::array{
            EnumMapping(TorrentData::Error::None, 0),
            EnumMapping(TorrentData::Error::TrackerWarning, 1),
            EnumMapping(TorrentData::Error::TrackerError, 2),
            EnumMapping(TorrentData::Error::LocalError, 3)
        });

        constexpr auto priorityMapper = EnumMapper(std::array{
            EnumMapping(TorrentData::Priority::Low, -1),
            EnumMapping(TorrentData::Priority::Normal, 0),
            EnumMapping(TorrentData::Priority::High, 1)
        });

        constexpr auto ratioLimitModeMapper = EnumMapper(std::array{
            EnumMapping(TorrentData::RatioLimitMode::Global, 0),
            EnumMapping(TorrentData::RatioLimitMode::Single, 1),
            EnumMapping(TorrentData::RatioLimitMode::Unlimited, 2)
        });

        constexpr auto idleSeedingLimitModeMapper = EnumMapper(std::array{
            EnumMapping(TorrentData::IdleSeedingLimitMode::Global, 0),
            EnumMapping(TorrentData::IdleSeedingLimitMode::Single, 1),
            EnumMapping(TorrentData::IdleSeedingLimitMode::Unlimited, 2)
        });
    }

    int TorrentData::priorityToInt(Priority value) { return priorityMapper.toJsonConstant(value); }

    bool TorrentData::update(const QJsonObject& object, bool firstTime, const Rpc* rpc) {
        bool changed = false;
        for (auto i = object.begin(), end = object.end(); i != end; ++i) {
            const auto key = mapUpdateKey(i.key());
            if (key.has_value()) {
                updateProperty(*key, i.value(), changed, firstTime, rpc);
            }
        }
        return changed;
    }

    bool TorrentData::update(
        std::span<const std::optional<TorrentData::UpdateKey>> keys,
        const QJsonArray& values,
        bool firstTime,
        const Rpc* rpc
    ) {
        bool changed = false;
        const auto count = std::min(keys.size(), static_cast<size_t>(values.size()));
        for (size_t i = 0; i < count; ++i) {
            const auto key = keys[i];
            if (key.has_value()) {
                updateProperty(*key, values[static_cast<QJsonArray::size_type>(i)], changed, firstTime, rpc);
            }
        }
        return changed;
    }

    void TorrentData::updateProperty(
        TorrentData::UpdateKey intKey, const QJsonValue& value, bool& changed, bool firstTime, const Rpc* rpc
    ) {
        const auto key = static_cast<UpdateKey>(intKey);
        switch (static_cast<UpdateKey>(key)) {
        case TorrentData::UpdateKey::Id:
            return;
        case TorrentData::UpdateKey::HashString:
            if (firstTime) {
                hashString = value.toString();
            }
            return;
        case TorrentData::UpdateKey::AddedDate:
            updateDateTime(addedDate, value, changed);
            return;
        case TorrentData::UpdateKey::Name:
            setChanged(name, value.toString(), changed);
            return;
        case TorrentData::UpdateKey::MagnetLink:
            setChanged(magnetLink, value.toString(), changed);
            return;
        case TorrentData::UpdateKey::QueuePosition:
            setChanged(queuePosition, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::TotalSize:
            setChanged(totalSize, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::CompletedSize:
            setChanged(completedSize, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::LeftUntilDone:
            setChanged(leftUntilDone, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::SizeWhenDone:
            setChanged(sizeWhenDone, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::PercentDone:
            setChanged(percentDone, value.toDouble(), changed);
            return;
        case TorrentData::UpdateKey::RecheckProgress:
            setChanged(recheckProgress, value.toDouble(), changed);
            return;
        case TorrentData::UpdateKey::Eta:
            setChanged(eta, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::MetadataPercentComplete:
            setChanged(metadataComplete, value.toInt() == 1, changed);
            return;
        case TorrentData::UpdateKey::DownloadSpeed:
            setChanged(downloadSpeed, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::UploadSpeed:
            setChanged(uploadSpeed, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::DownloadSpeedLimited:
            setChanged(downloadSpeedLimited, value.toBool(), changed);
            return;
        case TorrentData::UpdateKey::DownloadSpeedLimit:
            setChanged(downloadSpeedLimit, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::UploadSpeedLimited:
            setChanged(uploadSpeedLimited, value.toBool(), changed);
            return;
        case TorrentData::UpdateKey::UploadSpeedLimit:
            setChanged(uploadSpeedLimit, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::TotalDownloaded:
            setChanged(totalDownloaded, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::TotalUploaded:
            setChanged(totalUploaded, toInt64(value), changed);
            return;
        case TorrentData::UpdateKey::Ratio:
            setChanged(ratio, value.toDouble(), changed);
            return;
        case TorrentData::UpdateKey::RatioLimitMode:
            setChanged(ratioLimitMode, ratioLimitModeMapper.fromJsonValue(value, updateKeyString(key)), changed);
            return;
        case TorrentData::UpdateKey::RatioLimit:
            setChanged(ratioLimit, value.toDouble(), changed);
            return;
        case TorrentData::UpdateKey::PeersSendingToUsCount:
            setChanged(peersSendingToUsCount, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::PeersGettingFromUsCount:
            setChanged(peersGettingFromUsCount, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::WebSeeders: {
            setChanged(
                webSeeders,
                toContainer<std::vector>(value.toArray() | std::views::transform([](auto value) {
                                             return value.toString();
                                         })),
                changed
            );
            return;
        }
        case TorrentData::UpdateKey::WebSeedersSendingToUsCount:
            setChanged(webSeedersSendingToUsCount, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::Status:
            setChanged(status, statusMapper.fromJsonValue(value, updateKeyString(key)), changed);
            return;
        case TorrentData::UpdateKey::Error:
            setChanged(error, errorMapper.fromJsonValue(value, updateKeyString(key)), changed);
            return;
        case TorrentData::UpdateKey::ErrorString:
            setChanged(errorString, value.toString(), changed);
            return;
        case TorrentData::UpdateKey::ActivityDate:
            updateDateTime(activityDate, value, changed);
            return;
        case TorrentData::UpdateKey::DoneDate:
            updateDateTime(doneDate, value, changed);
            return;
        case TorrentData::UpdateKey::PeersLimit:
            setChanged(peersLimit, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::HonorSessionLimits:
            setChanged(honorSessionLimits, value.toBool(), changed);
            return;
        case TorrentData::UpdateKey::BandwidthPriority:
            setChanged(bandwidthPriority, priorityMapper.fromJsonValue(value, updateKeyString(key)), changed);
            return;
        case TorrentData::UpdateKey::IdleSeedingLimitMode:
            setChanged(
                idleSeedingLimitMode,
                idleSeedingLimitModeMapper.fromJsonValue(value, updateKeyString(key)),
                changed
            );
            return;
        case TorrentData::UpdateKey::IdleSeedingLimit:
            setChanged(idleSeedingLimit, value.toInt(), changed);
            return;
        case TorrentData::UpdateKey::DownloadDirectory:
            setChanged(
                downloadDirectory,
                normalizePath(value.toString(), rpc->serverSettings()->data().pathOs),
                changed
            );
            return;
        case TorrentData::UpdateKey::Creator:
            setChanged(creator, value.toString(), changed);
            return;
        case TorrentData::UpdateKey::CreationDate:
            updateDateTime(creationDate, value, changed);
            return;
        case TorrentData::UpdateKey::Comment:
            setChanged(comment, value.toString(), changed);
            return;
        case TorrentData::UpdateKey::TrackerStats: {
            std::vector<Tracker> newTrackers{};
            const QJsonArray trackerJsons = value.toArray();
            newTrackers.reserve(static_cast<size_t>(trackerJsons.size()));
            int newTotalSeeders{};
            int newTotalLeechers{};
            for (const auto& i : trackerJsons) {
                const QJsonObject trackerMap = i.toObject();
                const int trackerId = trackerMap.value("id"_l1).toInt();
                const auto found = std::ranges::find(trackers, trackerId, &Tracker::id);
                if (found == trackers.end()) {
                    newTrackers.emplace_back(trackerId, trackerMap);
                    changed = true;
                } else {
                    if (found->update(trackerMap)) {
                        changed = true;
                    }
                    newTrackers.push_back(std::move(*found));
                }
                newTotalSeeders += newTrackers.back().seeders();
                newTotalLeechers += newTrackers.back().leechers();
            }
            trackers = std::move(newTrackers);
            setChanged(totalSeedersFromTrackersCount, newTotalSeeders, changed);
            setChanged(totalLeechersFromTrackersCount, newTotalLeechers, changed);
            return;
        }
        case TorrentData::UpdateKey::Count:
            throw std::logic_error("UpdateKey::Count should not be mapped");
        }
        throw std::logic_error(fmt::format("Can't update key {}", static_cast<int>(intKey)));
    }

    Torrent::Torrent(int id, const QJsonObject& object, Rpc* rpc, QObject* parent) : QObject(parent), mRpc(rpc) {
        mData.id = id;
        [[maybe_unused]] const bool changed = mData.update(object, true, rpc);
    }

    Torrent::Torrent(
        int id,
        std::span<const std::optional<TorrentData::UpdateKey>> keys,
        const QJsonArray& values,
        Rpc* rpc,
        QObject* parent
    )
        : QObject(parent), mRpc(rpc) {
        mData.id = id;
        [[maybe_unused]] const bool changed = mData.update(keys, values, true, rpc);
    }

    QJsonArray Torrent::updateFields() {
        QJsonArray fields{};
        for (int i = 0; i < static_cast<int>(TorrentData::UpdateKey::Count); ++i) {
            const auto key = static_cast<TorrentData::UpdateKey>(i);
            fields.push_back(updateKeyString(key));
        }
        return fields;
    }

    std::optional<int> Torrent::idFromJson(const QJsonObject& object) {
        const auto value = object.value(updateKeyString(TorrentData::UpdateKey::Id));
        if (value.isDouble()) {
            return value.toInt();
        }
        return {};
    }

    std::optional<QJsonArray::size_type> Torrent::idKeyIndex(std::span<const std::optional<TorrentData::UpdateKey>> keys
    ) {
        return indexOfCasted<QJsonArray::size_type>(keys, TorrentData::UpdateKey::Id);
    }

    std::vector<std::optional<TorrentData::UpdateKey>> Torrent::mapUpdateKeys(const QJsonArray& stringKeys) {
        return toContainer<std::vector>(stringKeys | std::views::transform([](auto value) {
                                            return mapUpdateKey(value.toString());
                                        }));
    }

    void Torrent::setDownloadSpeedLimited(bool limited) {
        mData.downloadSpeedLimited = limited;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::DownloadSpeedLimited), limited);
    }

    void Torrent::setDownloadSpeedLimit(int limit) {
        mData.downloadSpeedLimit = limit;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::DownloadSpeedLimit), limit);
    }

    void Torrent::setUploadSpeedLimited(bool limited) {
        mData.uploadSpeedLimited = limited;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::UploadSpeedLimited), limited);
    }

    void Torrent::setUploadSpeedLimit(int limit) {
        mData.uploadSpeedLimit = limit;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::UploadSpeedLimit), limit);
    }

    void Torrent::setRatioLimitMode(TorrentData::RatioLimitMode mode) {
        mData.ratioLimitMode = mode;
        mRpc->setTorrentProperty(
            mData.id,
            updateKeyString(TorrentData::UpdateKey::RatioLimitMode),
            ratioLimitModeMapper.toJsonConstant(mode)
        );
    }

    void Torrent::setRatioLimit(double limit) {
        mData.ratioLimit = limit;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::RatioLimit), limit);
    }

    void Torrent::setPeersLimit(int limit) {
        mData.peersLimit = limit;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::PeersLimit), limit);
    }

    void Torrent::setHonorSessionLimits(bool honor) {
        mData.honorSessionLimits = honor;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::HonorSessionLimits), honor);
    }

    void Torrent::setBandwidthPriority(TorrentData::Priority priority) {
        mData.bandwidthPriority = priority;
        mRpc->setTorrentProperty(
            mData.id,
            updateKeyString(TorrentData::UpdateKey::BandwidthPriority),
            priorityMapper.toJsonConstant(priority)
        );
    }

    void Torrent::setIdleSeedingLimitMode(TorrentData::IdleSeedingLimitMode mode) {
        mData.idleSeedingLimitMode = mode;
        mRpc->setTorrentProperty(
            mData.id,
            updateKeyString(TorrentData::UpdateKey::IdleSeedingLimitMode),
            idleSeedingLimitModeMapper.toJsonConstant(mode)
        );
    }

    void Torrent::setIdleSeedingLimit(int limit) {
        mData.idleSeedingLimit = limit;
        mRpc->setTorrentProperty(mData.id, updateKeyString(TorrentData::UpdateKey::IdleSeedingLimit), limit);
    }

    namespace {
        std::vector<std::set<QString>> toTieredAnnounceUrls(std::span<const Tracker> trackers) {
            std::map<int, std::set<QString>> tiered{};
            for (const auto& tracker : trackers) {
                tiered[tracker.id()].insert(tracker.announce());
            }
            return moveToContainer<std::vector>(std::views::values(tiered));
        }

        QString toTrackerList(std::span<const std::set<QString>> tieredAnnounceUrls) {
            QString trackerList{};
            bool processedFirstTier{};
            for (const auto& tier : tieredAnnounceUrls) {
                if (processedFirstTier) {
                    trackerList += "\n\n"_l1;
                }
                for (const auto& announceUrl : tier) {
                    trackerList += announceUrl;
                    trackerList += '\n';
                }
                processedFirstTier = true;
            }
            return trackerList;
        }

        bool isIntersect(const std::set<QString>& existingTier, const std::set<QString>& newTier) {
            return std::ranges::any_of(newTier, [&](const auto& announceUrl) {
                return existingTier.contains(announceUrl);
            });
        }

        QJsonArray filterOutExistingTrackers(
            std::span<const std::set<QString>> newTrackers, std::span<const Tracker> existingTrackers
        ) {
            QJsonArray trackersToAdd{};
            for (const auto& tier : newTrackers) {
                if (tier.empty()) continue;
                // Transmission adds each announce URL to each own tier when using trackerAdd property, so take first URL from each tier
                const auto& first = *tier.begin();
                const auto existingTracker = std::ranges::find(existingTrackers, first, &Tracker::announce);
                if (existingTracker == existingTrackers.end()) {
                    trackersToAdd.push_back(first);
                }
            }
            return trackersToAdd;
        }
    }

    namespace impl {
        std::vector<std::set<QString>> mergeTrackers(
            const std::vector<std::set<QString>>& existingTrackers, std::span<const std::set<QString>> newTrackers
        ) {
            auto merged = existingTrackers;
            for (const auto& newTier : newTrackers) {
                if (newTier.empty()) continue;
                const auto existingTier =
                    std::ranges::find_if(merged, [&](auto& tier) { return isIntersect(tier, newTier); });
                if (existingTier != merged.end()) {
                    existingTier->insert(newTier.begin(), newTier.end());
                } else {
                    merged.push_back(newTier);
                }
            }
            return merged;
        }
    }

    void Torrent::addTrackers(std::span<const std::set<QString>> announceUrls) {
        if (mRpc->serverSettings()->data().hasTrackerListProperty()) {
            const auto existingTrackers = toTieredAnnounceUrls(mData.trackers);
            debug().log("Merging exisiting trackers {} with {}", existingTrackers, announceUrls);
            const auto merged = mergeTrackers(existingTrackers, announceUrls);
            const bool changed = merged != existingTrackers;
            debug().log("Result is {}, changed: {}", merged, changed);
            if (changed) {
                mRpc->setTorrentProperty(mData.id, trackerListKey, toTrackerList(merged), true);
            }
        } else {
            auto trackersToAdd = filterOutExistingTrackers(announceUrls, mData.trackers);
            if (!trackersToAdd.empty()) {
                mRpc->setTorrentProperty(mData.id, addTrackerKey, std::move(trackersToAdd), true);
            }
        }
    }

    void Torrent::setTracker(int trackerId, const QString& announce) {
        if (!mRpc->serverSettings()->data().hasTrackerListProperty()) {
            mRpc->setTorrentProperty(mData.id, replaceTrackerKey, QJsonArray{trackerId, announce}, true);
            return;
        }
        auto trackers = mData.trackers;
        const auto tracker = std::ranges::find(trackers, trackerId, &Tracker::id);
        if (tracker == trackers.end()) {
            warning().log("setTracker: did not find tracker with id {}", trackerId);
            return;
        }
        if (tracker->announce() == announce) {
            return;
        }
        tracker->replaceAnnounceUrl(announce);
        mRpc->setTorrentProperty(mData.id, trackerListKey, toTrackerList(toTieredAnnounceUrls(trackers)), true);
    }

    void Torrent::removeTrackers(std::span<const int> ids) {
        if (!mRpc->serverSettings()->data().hasTrackerListProperty()) {
            mRpc->setTorrentProperty(mData.id, removeTrackerKey, toJsonArray(ids), true);
            return;
        }
        auto trackers = mData.trackers;
        const auto erased = std::erase_if(trackers, [ids](const auto& tracker) {
            return std::ranges::find(ids, tracker.id()) != ids.end();
        });
        if (erased == 0) {
            return;
        }
        mRpc->setTorrentProperty(mData.id, trackerListKey, toTrackerList(toTieredAnnounceUrls(trackers)), true);
    }

    void Torrent::setFilesEnabled(bool enabled) {
        if (enabled != mFilesEnabled) {
            mFilesEnabled = enabled;
            if (mFilesEnabled) {
                mRpc->getTorrentFiles(mData.id);
            } else {
                mFiles.clear();
            }
        }
    }

    void Torrent::setFilesWanted(std::span<const int> fileIds, bool wanted) {
        mRpc->setTorrentProperty(mData.id, wanted ? wantedFilesKey : unwantedFilesKey, toJsonArray(fileIds));
    }

    void Torrent::setFilesPriority(std::span<const int> fileIds, TorrentFile::Priority priority) {
        QLatin1String propertyName;
        switch (priority) {
        case TorrentFile::Priority::Low:
            propertyName = lowPriorityKey;
            break;
        case TorrentFile::Priority::Normal:
            propertyName = normalPriorityKey;
            break;
        case TorrentFile::Priority::High:
            propertyName = highPriorityKey;
            break;
        }
        mRpc->setTorrentProperty(mData.id, propertyName, toJsonArray(fileIds));
    }

    void Torrent::renameFile(const QString& path, const QString& newName) {
        mRpc->renameTorrentFile(mData.id, path, newName);
    }

    void Torrent::setPeersEnabled(bool enabled) {
        if (enabled != mPeersEnabled) {
            mPeersEnabled = enabled;
            if (mPeersEnabled) {
                mRpc->getTorrentPeers(mData.id);
            } else {
                mPeers.clear();
            }
        }
    }

    bool Torrent::update(const QJsonObject& object) {
        const bool c = mData.update(object, false, mRpc);
        emit updated();
        if (c) {
            emit changed();
        }
        return c;
    }

    bool Torrent::update(std::span<const std::optional<TorrentData::UpdateKey>> keys, const QJsonArray& values) {
        const bool c = mData.update(keys, values, false, mRpc);
        emit updated();
        if (c) {
            emit changed();
        }
        return c;
    }

    void Torrent::updateFiles(const QJsonObject& torrentMap) {
        std::vector<int> changed{};

        const QJsonArray fileStats = torrentMap.value("fileStats"_l1).toArray();
        if (!fileStats.isEmpty()) {
            if (mFiles.empty()) {
                const QJsonArray fileJsons = torrentMap.value("files"_l1).toArray();
                if (fileJsons.size() == fileStats.size()) {
                    const auto count = fileJsons.size();
                    mFiles.reserve(static_cast<size_t>(count));
                    changed.reserve(static_cast<size_t>(count));
                    for (QJsonArray::size_type i = 0; i < count; ++i) {
                        mFiles.emplace_back(i, fileJsons[i].toObject(), fileStats[i].toObject());
                        changed.push_back(static_cast<int>(i));
                    }
                } else {
                    warning().log("fileStats and files arrays have different sizes for torrent {}", *this);
                }
            } else {
                if (static_cast<size_t>(fileStats.size()) == mFiles.size()) {
                    for (QJsonArray::size_type i = 0, max = fileStats.size(); i < max; ++i) {
                        TorrentFile& file = mFiles[static_cast<size_t>(i)];
                        if (file.update(fileStats[i].toObject())) {
                            changed.push_back(static_cast<int>(i));
                        }
                    }
                } else {
                    warning().log("fileStats array has different size than in previous update for torrent {}", *this);
                }
            }
        }

        emit filesUpdated(changed);
    }

    namespace {
        struct NewPeer {
            QJsonObject json;
            QString address;
        };

        class PeersListUpdater final : public ItemListUpdater<Peer, std::vector<NewPeer>> {
        public:
            PeersListUpdater() = default;

            std::vector<std::pair<int, int>> removedIndexRanges{};
            std::vector<std::pair<int, int>> changedIndexRanges{};
            int addedCount{};

        protected:
            std::vector<NewPeer>::iterator
            findNewItemForItem(std::vector<NewPeer>& newPeers, const Peer& peer) override {
                return std::ranges::find(newPeers, peer.address, &NewPeer::address);
            }

            void onAboutToRemoveItems(size_t, size_t) override{};

            void onRemovedItems(size_t first, size_t last) override {
                removedIndexRanges.emplace_back(static_cast<int>(first), static_cast<int>(last));
            }

            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
            bool updateItem(Peer& peer, NewPeer&& newPeer) override {
                const auto& [json, address] = newPeer;
                return peer.update(json);
            }

            void onChangedItems(size_t first, size_t last) override {
                changedIndexRanges.emplace_back(static_cast<int>(first), static_cast<int>(last));
            }

            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
            Peer createItemFromNewItem(NewPeer&& newPeer) override {
                auto& [json, address] = newPeer;
                return Peer(std::move(address), json);
            }

            void onAboutToAddItems(size_t) override {}

            void onAddedItems(size_t count) override { addedCount = static_cast<int>(count); };
        };
    }

    void Torrent::updatePeers(const QJsonObject& torrentMap) {
        std::vector<NewPeer> newPeers;
        {
            const QJsonArray peers(torrentMap.value("peers"_l1).toArray());
            newPeers.reserve(static_cast<size_t>(peers.size()));
            for (const auto& i : peers) {
                QJsonObject json = i.toObject();
                QString address(json.value(Peer::addressKey).toString());
                newPeers.push_back(NewPeer{std::move(json), std::move(address)});
            }
        }

        PeersListUpdater updater{};
        updater.update(mPeers, std::move(newPeers));

        emit peersUpdated(updater.removedIndexRanges, updater.changedIndexRanges, updater.addedCount);
    }

    void Torrent::checkSingleFile(const QJsonObject& torrentMap) {
        mData.singleFile = (torrentMap.value(prioritiesKey).toArray().size() == 1);
    }
}

namespace fmt {
    format_context::iterator
    formatter<tremotesf::Torrent>::format(const tremotesf::Torrent& torrent, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "Torrent(id={}, name={})", torrent.data().id, torrent.data().name);
    }
}
