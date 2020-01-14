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

        void update(const QJsonObject& torrentMap, const Rpc* rpc);

        int id = 0;
        QString hashString;
        QString name;

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

        long long downloadSpeed = 0;
        long long uploadSpeed = 0;

        bool downloadSpeedLimited = false;
        int downloadSpeedLimit = 0; // KiB/s
        bool uploadSpeedLimited = false;
        int uploadSpeedLimit = 0; // KiB/s

        long long totalDownloaded = 0;
        long long totalUploaded = 0;
        double ratio = 0.0;
        double ratioLimit = 0.0;
        RatioLimitMode ratioLimitMode = GlobalRatioLimit;

        int seeders = 0;
        int leechers = 0;
        int peersLimit = 0;

        QDateTime addedDate;
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
        bool singleFile = false;

        bool trackersAddedOrRemoved = false;

        bool changed;

        std::vector<Tracker> trackers;
    };

    class Torrent : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(int id READ id CONSTANT)
        Q_PROPERTY(QString hashString READ hashString CONSTANT)
        Q_PROPERTY(QString name READ name NOTIFY updated)

        Q_PROPERTY(libtremotesf::TorrentData::Status status READ status NOTIFY updated)
        Q_PROPERTY(QString errorString READ errorString NOTIFY updated)
        Q_PROPERTY(int queuePosition READ queuePosition NOTIFY updated)

        Q_PROPERTY(long long totalSize READ totalSize NOTIFY updated)
        Q_PROPERTY(long long completedSize READ completedSize NOTIFY updated)
        Q_PROPERTY(long long leftUntilDone READ leftUntilDone NOTIFY updated)
        Q_PROPERTY(long long sizeWhenDone READ sizeWhenDone NOTIFY updated)
        Q_PROPERTY(double percentDone READ percentDone NOTIFY updated)
        Q_PROPERTY(bool finished READ isFinished NOTIFY updated)
        Q_PROPERTY(double recheckProgress READ recheckProgress NOTIFY updated)
        Q_PROPERTY(int eta READ eta NOTIFY updated)

        Q_PROPERTY(long long downloadSpeed READ downloadSpeed NOTIFY updated)
        Q_PROPERTY(long long uploadSpeed READ uploadSpeed NOTIFY updated)

        Q_PROPERTY(bool downloadSpeedLimited READ isDownloadSpeedLimited WRITE setDownloadSpeedLimited)
        Q_PROPERTY(int downloadSpeedLimit READ downloadSpeedLimit WRITE setDownloadSpeedLimit)
        Q_PROPERTY(bool uploadSpeedLimited READ isUploadSpeedLimited WRITE setUploadSpeedLimited)
        Q_PROPERTY(int uploadSpeedLimit READ uploadSpeedLimit WRITE setUploadSpeedLimit)

        Q_PROPERTY(long long totalDownloaded READ totalDownloaded NOTIFY updated)
        Q_PROPERTY(long long totalUploaded READ totalUploaded NOTIFY updated)
        Q_PROPERTY(double ratio READ ratio NOTIFY updated)
        Q_PROPERTY(libtremotesf::TorrentData::RatioLimitMode ratioLimitMode READ ratioLimitMode WRITE setRatioLimitMode NOTIFY updated)
        Q_PROPERTY(double ratioLimit READ ratioLimit WRITE setRatioLimit NOTIFY updated)

        Q_PROPERTY(int seeders READ seeders NOTIFY updated)
        Q_PROPERTY(int leechers READ leechers NOTIFY updated)
        Q_PROPERTY(int peersLimit READ peersLimit WRITE setPeersLimit NOTIFY updated)

        Q_PROPERTY(QDateTime addedDate READ addedDate CONSTANT)
        Q_PROPERTY(QDateTime activityDate READ activityDate NOTIFY updated)
        Q_PROPERTY(QDateTime doneDate READ doneDate NOTIFY updated)

        Q_PROPERTY(bool honorSessionLimits READ honorSessionLimits WRITE setHonorSessionLimits)
        Q_PROPERTY(libtremotesf::TorrentData::Priority bandwidthPriority READ bandwidthPriority WRITE setBandwidthPriority)
        Q_PROPERTY(libtremotesf::TorrentData::IdleSeedingLimitMode idleSeedingLimitMode READ idleSeedingLimitMode WRITE setIdleSeedingLimitMode)
        Q_PROPERTY(int idleSeedingLimit READ idleSeedingLimit WRITE setIdleSeedingLimit)
        Q_PROPERTY(QString downloadDirectory READ downloadDirectory NOTIFY updated)
        Q_PROPERTY(bool singleFile READ isSingleFile NOTIFY updated)
        Q_PROPERTY(QString creator READ creator NOTIFY updated)
        Q_PROPERTY(QDateTime creationDate READ creationDate NOTIFY updated)
        Q_PROPERTY(QString comment READ comment NOTIFY updated)

    public:
        using Status = TorrentData::Status;
        using Priority = TorrentData::Priority;
        using RatioLimitMode = TorrentData::RatioLimitMode;
        using IdleSeedingLimitMode = TorrentData::IdleSeedingLimitMode;

        static const QJsonKeyString idKey;

        explicit Torrent(int id, const QJsonObject& torrentMap, Rpc* rpc);

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
        Q_INVOKABLE void setBandwidthPriority(Priority priority);
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
        bool isTrackersAddedOrRemoved() const;
        Q_INVOKABLE void addTracker(const QString& announce);
        Q_INVOKABLE void setTracker(int trackerId, const QString& announce);
        Q_INVOKABLE void removeTrackers(const QVariantList& ids);

        bool isChanged() const;

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

        void update(const QJsonObject& torrentMap);
        void updateFiles(const QJsonObject& torrentMap);
        void updatePeers(const QJsonObject& torrentMap);
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
        void filesUpdated(const std::vector<const libtremotesf::TorrentFile*>& changed);
        void peersUpdated(const std::vector<const libtremotesf::Peer*>& changed, const std::vector<const libtremotesf::Peer*>& added, const std::vector<int>& removed);
        void fileRenamed(const QString& filePath, const QString& newName);
        void limitsEdited();
    };
}

#endif // LIBTREMOTESF_TORRENT_H
