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

#ifndef LIBTREMOTESF_TORRENT_H
#define LIBTREMOTESF_TORRENT_H

#include <vector>

#include <QDateTime>
#include <QObject>

#include <fmt/core.h>

#include "peer.h"
#include "stdutils.h"
#include "torrentfile.h"
#include "tracker.h"

class QJsonObject;

namespace libtremotesf
{
    class Rpc;

    struct TorrentData
    {
        Q_GADGET
    public:
        enum Status
        {
            Paused,
            Downloading,
            Seeding,
            StalledDownloading,
            StalledSeeding,
            QueuedForDownloading,
            QueuedForSeeding,
            Checking,
            QueuedForChecking,
            Errored
        };
        Q_ENUM(Status)

        enum Priority
        {
            LowPriority = -1,
            NormalPriority,
            HighPriority
        };
        Q_ENUM(Priority)

        enum RatioLimitMode
        {
            GlobalRatioLimit,
            SingleRatioLimit,
            UnlimitedRatio
        };
        Q_ENUM(RatioLimitMode)

        enum IdleSeedingLimitMode
        {
            GlobalIdleSeedingLimit,
            SingleIdleSeedingLimit,
            UnlimitedIdleSeeding
        };
        Q_ENUM(IdleSeedingLimitMode)

        bool update(const QJsonObject& torrentMap);

        int id = 0;
        QString hashString;
        QString name;
        QString magnetLink;

        QString errorString;
        Status status = Paused;
        int queuePosition = 0;

        long long totalSize = 0;
        long long completedSize = 0;
        long long leftUntilDone = 0;
        long long sizeWhenDone = 0;

        double percentDone = 0.0;
        double recheckProgress = 0.0;
        int eta = 0;

        bool metadataComplete = false;

        long long downloadSpeed = 0;
        long long uploadSpeed = 0;

        bool downloadSpeedLimited = false;
        int downloadSpeedLimit = 0; // kB/s
        bool uploadSpeedLimited = false;
        int uploadSpeedLimit = 0; // kB/s

        long long totalDownloaded = 0;
        long long totalUploaded = 0;
        double ratio = 0.0;
        double ratioLimit = 0.0;
        RatioLimitMode ratioLimitMode = GlobalRatioLimit;

        int seeders = 0;
        int leechers = 0;
        int peersLimit = 0;

        QDateTime addedDate;
        long long addedDateTime = -1;
        QDateTime activityDate;
        long long activityDateTime = -1;
        QDateTime doneDate;
        long long doneDateTime = -1;

        IdleSeedingLimitMode idleSeedingLimitMode = GlobalIdleSeedingLimit;
        int idleSeedingLimit = 0;
        QString downloadDirectory;
        QString comment;
        QString creator;
        QDateTime creationDate;
        long long creationDateTime = -1;
        Priority bandwidthPriority = NormalPriority;
        bool honorSessionLimits = false;

        bool singleFile = true;

        bool trackersAnnounceUrlsChanged = false;

        std::vector<Tracker> trackers;

        std::vector<QString> webSeeders;
        int activeWebSeeders = 0;
    };

    class Torrent : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(int id READ id CONSTANT)
        Q_PROPERTY(QString hashString READ hashString CONSTANT)
        Q_PROPERTY(QString name READ name NOTIFY changed)

        Q_PROPERTY(libtremotesf::TorrentData::Status status READ status NOTIFY changed)
        Q_PROPERTY(QString errorString READ errorString NOTIFY changed)
        Q_PROPERTY(int queuePosition READ queuePosition NOTIFY changed)

        Q_PROPERTY(long long totalSize READ totalSize NOTIFY changed)
        Q_PROPERTY(long long completedSize READ completedSize NOTIFY changed)
        Q_PROPERTY(long long leftUntilDone READ leftUntilDone NOTIFY changed)
        Q_PROPERTY(long long sizeWhenDone READ sizeWhenDone NOTIFY changed)
        Q_PROPERTY(double percentDone READ percentDone NOTIFY changed)
        Q_PROPERTY(bool finished READ isFinished NOTIFY changed)
        Q_PROPERTY(double recheckProgress READ recheckProgress NOTIFY changed)
        Q_PROPERTY(int eta READ eta NOTIFY changed)

        Q_PROPERTY(long long downloadSpeed READ downloadSpeed NOTIFY changed)
        Q_PROPERTY(long long uploadSpeed READ uploadSpeed NOTIFY changed)

        Q_PROPERTY(bool downloadSpeedLimited READ isDownloadSpeedLimited WRITE setDownloadSpeedLimited NOTIFY changed)
        Q_PROPERTY(int downloadSpeedLimit READ downloadSpeedLimit WRITE setDownloadSpeedLimit NOTIFY changed)
        Q_PROPERTY(bool uploadSpeedLimited READ isUploadSpeedLimited WRITE setUploadSpeedLimited NOTIFY changed)
        Q_PROPERTY(int uploadSpeedLimit READ uploadSpeedLimit WRITE setUploadSpeedLimit NOTIFY changed)

        Q_PROPERTY(long long totalDownloaded READ totalDownloaded NOTIFY changed)
        Q_PROPERTY(long long totalUploaded READ totalUploaded NOTIFY changed)
        Q_PROPERTY(double ratio READ ratio NOTIFY changed)
        Q_PROPERTY(libtremotesf::TorrentData::RatioLimitMode ratioLimitMode READ ratioLimitMode WRITE setRatioLimitMode NOTIFY changed)
        Q_PROPERTY(double ratioLimit READ ratioLimit WRITE setRatioLimit NOTIFY changed)

        Q_PROPERTY(int seeders READ seeders NOTIFY changed)
        Q_PROPERTY(int leechers READ leechers NOTIFY changed)
        Q_PROPERTY(int peersLimit READ peersLimit WRITE setPeersLimit NOTIFY changed)

        Q_PROPERTY(QDateTime addedDate READ addedDate CONSTANT)
        Q_PROPERTY(QDateTime activityDate READ activityDate NOTIFY changed)
        Q_PROPERTY(QDateTime doneDate READ doneDate NOTIFY changed)

        Q_PROPERTY(bool honorSessionLimits READ honorSessionLimits WRITE setHonorSessionLimits NOTIFY changed)
        Q_PROPERTY(libtremotesf::TorrentData::Priority bandwidthPriority READ bandwidthPriority WRITE setBandwidthPriority NOTIFY changed)
        Q_PROPERTY(libtremotesf::TorrentData::IdleSeedingLimitMode idleSeedingLimitMode READ idleSeedingLimitMode WRITE setIdleSeedingLimitMode NOTIFY changed)
        Q_PROPERTY(int idleSeedingLimit READ idleSeedingLimit WRITE setIdleSeedingLimit NOTIFY changed)
        Q_PROPERTY(QString downloadDirectory READ downloadDirectory NOTIFY changed)
        Q_PROPERTY(bool singleFile READ isSingleFile NOTIFY changed)
        Q_PROPERTY(QString creator READ creator NOTIFY changed)
        Q_PROPERTY(QDateTime creationDate READ creationDate NOTIFY changed)
        Q_PROPERTY(QString comment READ comment NOTIFY changed)

        Q_PROPERTY(std::vector<QString> webSeeders READ webSeeders NOTIFY changed)
        Q_PROPERTY(int activeWebSeeders READ activeWebSeeders NOTIFY changed)

    public:
        using Status = TorrentData::Status;
        using Priority = TorrentData::Priority;
        using RatioLimitMode = TorrentData::RatioLimitMode;
        using IdleSeedingLimitMode = TorrentData::IdleSeedingLimitMode;

        static const QLatin1String idKey;

        explicit Torrent(int id, const QJsonObject& torrentMap, Rpc* rpc, QObject* parent = nullptr);

        int id() const;
        const QString& hashString() const;
        const QString& name() const;

        Status status() const;
        QString errorString() const;
        int queuePosition() const;

        long long totalSize() const;
        long long completedSize() const;
        long long leftUntilDone() const;
        long long sizeWhenDone() const;
        double percentDone() const;
        bool isFinished() const;
        double recheckProgress() const;
        int eta() const;

        bool isMetadataComplete() const;

        long long downloadSpeed() const;
        long long uploadSpeed() const;

        bool isDownloadSpeedLimited() const;
        Q_INVOKABLE void setDownloadSpeedLimited(bool limited);
        int downloadSpeedLimit() const;
        Q_INVOKABLE void setDownloadSpeedLimit(int limit);

        bool isUploadSpeedLimited() const;
        Q_INVOKABLE void setUploadSpeedLimited(bool limited);
        int uploadSpeedLimit() const;
        Q_INVOKABLE void setUploadSpeedLimit(int limit);

        long long totalDownloaded() const;
        long long totalUploaded() const;
        double ratio() const;
        RatioLimitMode ratioLimitMode() const;
        Q_INVOKABLE void setRatioLimitMode(libtremotesf::Torrent::RatioLimitMode mode);
        double ratioLimit() const;
        Q_INVOKABLE void setRatioLimit(double limit);

        int seeders() const;
        int leechers() const;
        int peersLimit() const;
        Q_INVOKABLE void setPeersLimit(int limit);

        const QDateTime& addedDate() const;
        const QDateTime& activityDate() const;
        const QDateTime& doneDate() const;

        bool honorSessionLimits() const;
        Q_INVOKABLE void setHonorSessionLimits(bool honor);
        Priority bandwidthPriority() const;
        Q_INVOKABLE void setBandwidthPriority(libtremotesf::Torrent::Priority priority);
        IdleSeedingLimitMode idleSeedingLimitMode() const;
        Q_INVOKABLE void setIdleSeedingLimitMode(libtremotesf::Torrent::IdleSeedingLimitMode mode);
        int idleSeedingLimit() const;
        Q_INVOKABLE void setIdleSeedingLimit(int limit);
        const QString& downloadDirectory() const;
        bool isSingleFile() const;
        const QString& creator() const;
        const QDateTime& creationDate() const;
        const QString& comment() const;

        const std::vector<Tracker>& trackers() const;
        bool isTrackersAnnounceUrlsChanged() const;
        Q_INVOKABLE void addTrackers(const QStringList& announceUrls);
        Q_INVOKABLE void setTracker(int trackerId, const QString& announce);
        Q_INVOKABLE void removeTrackers(const QVariantList& ids);

        const std::vector<QString>& webSeeders() const;
        int activeWebSeeders() const;

        const TorrentData& data() const;

        bool isFilesEnabled() const;
        Q_INVOKABLE void setFilesEnabled(bool enabled);
        const std::vector<TorrentFile>& files() const;

        Q_INVOKABLE void setFilesWanted(const QVariantList& files, bool wanted);
        Q_INVOKABLE void setFilesPriority(const QVariantList& files, libtremotesf::TorrentFile::Priority priority);
        Q_INVOKABLE void renameFile(const QString& path, const QString& newName);

        bool isPeersEnabled() const;
        Q_INVOKABLE void setPeersEnabled(bool enabled);
        const std::vector<Peer>& peers() const;

        bool isUpdated() const;
        void checkThatFilesUpdated();
        void checkThatPeersUpdated();

        bool update(const QJsonObject& torrentMap);
        void updateFiles(const QJsonObject& torrentMap);
        void updatePeers(const QJsonObject& torrentMap);

        void checkSingleFile(const QJsonObject& torrentMap);
    private:
        Rpc* mRpc;

        TorrentData mData;

        std::vector<TorrentFile> mFiles;
        bool mFilesEnabled = false;
        bool mFilesUpdated = false;

        std::vector<Peer> mPeers;
        bool mPeersEnabled = false;
        bool mPeersUpdated = false;

    signals:
        void updated();
        void changed();

        void filesUpdated(const std::vector<int>& changedIndexes);
        void peersUpdated(const std::vector<std::pair<int, int>>& removedIndexRanges, const std::vector<std::pair<int, int>>& changedIndexRanges, int addedCount);
        void fileRenamed(const QString& filePath, const QString& newName);
        void limitsEdited();
    };
}

// SWIG can't parse it :(
#ifndef SWIG
template<>
struct fmt::formatter<libtremotesf::Torrent> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const libtremotesf::Torrent& torrent, FormatContext& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Torrent(id={}, name={})", torrent.id(), torrent.name());
    }
};

template<>
struct fmt::formatter<libtremotesf::Torrent*> : fmt::formatter<libtremotesf::Torrent> {
    template<typename FormatContext>
    auto format(const libtremotesf::Torrent* torrent, FormatContext& ctx) -> decltype(ctx.out()) {
        if (torrent) {
            return fmt::formatter<libtremotesf::Torrent>::format(*torrent, ctx);
        }
        return fmt::format_to(ctx.out(), "Torrent(nullptr)");
    }
};
#endif // SWIG

#endif // LIBTREMOTESF_TORRENT_H
