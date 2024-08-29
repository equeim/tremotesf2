// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_TORRENT_H
#define TREMOTESF_RPC_TORRENT_H

#include <optional>
#include <set>
#include <span>
#include <vector>

#include <QDateTime>
#include <QJsonArray>
#include <QObject>

#include "log/formatters.h"
#include "peer.h"
#include "torrentfile.h"
#include "tracker.h"

class QJsonObject;

namespace tremotesf {
    class Rpc;

    struct TorrentData {
        Q_GADGET
    public:
        enum class Status {
            Paused,
            QueuedForChecking,
            Checking,
            QueuedForDownloading,
            Downloading,
            QueuedForSeeding,
            Seeding
        };
        Q_ENUM(Status)

        enum class Error { None, TrackerWarning, TrackerError, LocalError };
        Q_ENUM(Error)

        enum class Priority { Low, Normal, High };
        Q_ENUM(Priority)
        static int priorityToInt(Priority value);

        enum class RatioLimitMode { Global, Single, Unlimited };
        Q_ENUM(RatioLimitMode)

        enum class IdleSeedingLimitMode { Global, Single, Unlimited };
        Q_ENUM(IdleSeedingLimitMode)

        [[nodiscard]] bool update(const QJsonObject& object, bool firstTime, const Rpc* rpc);

        enum class UpdateKey;
        [[nodiscard]] bool update(
            std::span<const std::optional<UpdateKey>> keys, const QJsonArray& values, bool firstTime, const Rpc* rpc
        );

        int id{};
        QString hashString{};
        QString name{};
        QString magnetLink{};

        Status status{};
        Error error{};
        QString errorString{};

        int queuePosition{};

        qint64 totalSize{};
        qint64 completedSize{};
        qint64 leftUntilDone{};
        qint64 sizeWhenDone{};

        double percentDone{};
        double recheckProgress{};
        int eta{};

        bool metadataComplete{};

        qint64 downloadSpeed{};
        qint64 uploadSpeed{};

        bool downloadSpeedLimited{};
        int downloadSpeedLimit{}; // kB/s
        bool uploadSpeedLimited{};
        int uploadSpeedLimit{}; // kB/s

        qint64 totalDownloaded{};
        qint64 totalUploaded{};
        double ratio{};
        double ratioLimit{};
        RatioLimitMode ratioLimitMode{};

        int totalSeedersFromTrackersCount{};
        int peersSendingToUsCount{};
        std::vector<QString> webSeeders{};
        int webSeedersSendingToUsCount{};
        int totalLeechersFromTrackersCount{};
        int peersGettingFromUsCount{};

        int peersLimit{};

        QDateTime addedDate{{}, {}, Qt::UTC};
        QDateTime activityDate{{}, {}, Qt::UTC};
        QDateTime doneDate{{}, {}, Qt::UTC};

        IdleSeedingLimitMode idleSeedingLimitMode{};
        int idleSeedingLimit{};
        QString downloadDirectory{};
        QString comment{};
        QString creator{};
        QDateTime creationDate{{}, {}, Qt::UTC};
        Priority bandwidthPriority{};
        bool honorSessionLimits;

        bool singleFile = true;

        std::vector<Tracker> trackers{};

        [[nodiscard]] bool hasError() const { return error != Error::None; }
        [[nodiscard]] bool isFinished() const { return leftUntilDone == 0; }
        [[nodiscard]] bool isDownloadingStalled() const {
            return (peersSendingToUsCount == 0 && webSeedersSendingToUsCount == 0);
        }
        [[nodiscard]] bool isSeedingStalled() const { return peersGettingFromUsCount == 0; }

    private:
        void updateProperty(
            TorrentData::UpdateKey key, const QJsonValue& value, bool& changed, bool firstTime, const Rpc* rpc
        );
        void applyTrackerErrorWorkaround(bool& changed);
    };

    class Torrent final : public QObject {
        Q_OBJECT

    public:
        explicit Torrent(int id, const QJsonObject& object, Rpc* rpc, QObject* parent = nullptr);
        explicit Torrent(
            int id,
            std::span<const std::optional<TorrentData::UpdateKey>> keys,
            const QJsonArray& values,
            Rpc* rpc,
            QObject* parent = nullptr
        );
        // For testing only
        explicit Torrent() = default;

        [[nodiscard]] static QJsonArray updateFields();
        [[nodiscard]] static std::optional<int> idFromJson(const QJsonObject& object);
        [[nodiscard]] static std::optional<QJsonArray::size_type>
        idKeyIndex(std::span<const std::optional<TorrentData::UpdateKey>> keys);
        [[nodiscard]] static std::vector<std::optional<TorrentData::UpdateKey>>
        mapUpdateKeys(const QJsonArray& stringKeys);

        void setDownloadSpeedLimited(bool limited);
        void setDownloadSpeedLimit(int limit);
        void setUploadSpeedLimited(bool limited);
        void setUploadSpeedLimit(int limit);
        void setRatioLimitMode(TorrentData::RatioLimitMode mode);
        void setRatioLimit(double limit);
        void setPeersLimit(int limit);
        void setHonorSessionLimits(bool honor);
        void setBandwidthPriority(TorrentData::Priority priority);
        void setIdleSeedingLimitMode(TorrentData::IdleSeedingLimitMode mode);
        void setIdleSeedingLimit(int limit);

        void addTrackers(std::span<const std::set<QString>> announceUrls);
        void setTracker(int trackerId, const QString& announce);
        void removeTrackers(std::span<const int> trackerIds);

        [[nodiscard]] const TorrentData& data() const { return mData; };

        [[nodiscard]] bool isFilesEnabled() const { return mFilesEnabled; };
        void setFilesEnabled(bool enabled);
        [[nodiscard]] const std::vector<TorrentFile>& files() const { return mFiles; };

        void setFilesWanted(std::span<const int> fileIds, bool wanted);
        void setFilesPriority(std::span<const int> fileIds, TorrentFile::Priority priority);
        void renameFile(const QString& path, const QString& newName);

        [[nodiscard]] bool isPeersEnabled() const { return mPeersEnabled; };
        void setPeersEnabled(bool enabled);
        [[nodiscard]] const std::vector<Peer>& peers() const { return mPeers; };

        [[nodiscard]] bool update(const QJsonObject& object);
        [[nodiscard]] bool
        update(std::span<const std::optional<TorrentData::UpdateKey>> keys, const QJsonArray& values);
        void updateFiles(const QJsonObject& torrentMap);
        void updatePeers(const QJsonObject& torrentMap);

        void checkSingleFile(const QJsonObject& torrentMap);

    private:
        Rpc* mRpc{};

        TorrentData mData{};

        std::vector<TorrentFile> mFiles{};
        bool mFilesEnabled{};

        std::vector<Peer> mPeers{};
        bool mPeersEnabled{};
    signals:
        void updated();
        void changed();

        void filesUpdated(const std::vector<int>& changedIndexes);
        void peersUpdated(
            const std::vector<std::pair<int, int>>& removedIndexRanges,
            const std::vector<std::pair<int, int>>& changedIndexRanges,
            int addedCount
        );
        void fileRenamed(const QString& filePath, const QString& newName);
    };

    namespace impl {
        std::vector<std::set<QString>> mergeTrackers(
            const std::vector<std::set<QString>>& existingTrackers, std::span<const std::set<QString>> newTrackers
        );
    }
}

namespace fmt {
    template<>
    struct formatter<tremotesf::Torrent> : tremotesf::SimpleFormatter {
        format_context::iterator format(const tremotesf::Torrent& torrent, format_context& ctx) const;
    };
}

#endif // TREMOTESF_RPC_TORRENT_H
