// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_SERVERSETTINGS_H
#define TREMOTESF_RPC_SERVERSETTINGS_H

#include <QObject>
#include <QTime>

#include "pathutils.h"

class QJsonObject;

namespace tremotesf {
    class Rpc;

    struct ServerSettingsData {
        Q_GADGET
    public:
        enum class AlternativeSpeedLimitsDays {
            Sunday,
            Monday,
            Tuesday,
            Wednesday,
            Thursday,
            Friday,
            Saturday,
            Weekdays,
            Weekends,
            All
        };
        Q_ENUM(AlternativeSpeedLimitsDays)

        enum class EncryptionMode { Allowed, Preferred, Required };
        Q_ENUM(EncryptionMode)

        [[nodiscard]] bool canRenameFiles() const;
        [[nodiscard]] bool canShowFreeSpaceForPath() const;
        [[nodiscard]] bool hasSessionIdFile() const;
        [[nodiscard]] bool hasTableMode() const;
        [[nodiscard]] bool hasTrackerListProperty() const;
        [[nodiscard]] bool hasFileCountProperty() const;
        [[nodiscard]] bool hasLabelsProperty() const;

        int rpcVersion = 0;
        int minimumRpcVersion = 0;

        QString configDirectory;
        PathOs pathOs = PathOs::Unix;

        QString downloadDirectory;
        bool startAddedTorrents = false;
        bool trashTorrentFiles = false;
        bool renameIncompleteFiles = false;
        bool incompleteDirectoryEnabled = false;
        QString incompleteDirectory;

        bool ratioLimited = false;
        double ratioLimit = 0.0;
        bool idleSeedingLimited = false;
        int idleSeedingLimit = 0;

        bool downloadQueueEnabled = false;
        int downloadQueueSize = 0;
        bool seedQueueEnabled = false;
        int seedQueueSize = 0;
        bool idleQueueLimited = false;
        int idleQueueLimit = 0;

        bool downloadSpeedLimited = false;
        int downloadSpeedLimit = 0;
        bool uploadSpeedLimited = false;
        int uploadSpeedLimit = 0;
        bool alternativeSpeedLimitsEnabled = false;
        int alternativeDownloadSpeedLimit = 0;
        int alternativeUploadSpeedLimit = 0;
        bool alternativeSpeedLimitsScheduled = false;
        QTime alternativeSpeedLimitsBeginTime;
        QTime alternativeSpeedLimitsEndTime;
        AlternativeSpeedLimitsDays alternativeSpeedLimitsDays{};

        int peerPort = 0;
        bool randomPortEnabled = false;
        bool portForwardingEnabled = false;
        EncryptionMode encryptionMode{};
        bool utpEnabled = false;
        bool pexEnabled = false;
        bool dhtEnabled = false;
        bool lpdEnabled = false;
        int maximumPeersPerTorrent = 0;
        int maximumPeersGlobally = 0;
    };

    class ServerSettings final : public QObject {
        Q_OBJECT

    public:
        explicit ServerSettings(Rpc* rpc = nullptr, QObject* parent = nullptr);

        void setDownloadDirectory(const QString& directory);
        void setStartAddedTorrents(bool start);
        void setTrashTorrentFiles(bool trash);
        void setRenameIncompleteFiles(bool rename);
        void setIncompleteDirectoryEnabled(bool enabled);
        void setIncompleteDirectory(const QString& directory);

        void setRatioLimited(bool limited);
        void setRatioLimit(double limit);
        void setIdleSeedingLimited(bool limited);
        void setIdleSeedingLimit(int limit);

        void setDownloadQueueEnabled(bool enabled);
        void setDownloadQueueSize(int size);
        void setSeedQueueEnabled(bool enabled);
        void setSeedQueueSize(int size);
        void setIdleQueueLimited(bool limited);
        void setIdleQueueLimit(int limit);

        void setDownloadSpeedLimited(bool limited);
        void setDownloadSpeedLimit(int limit);
        void setUploadSpeedLimited(bool limited);
        void setUploadSpeedLimit(int limit);
        void setAlternativeSpeedLimitsEnabled(bool enabled);
        void setAlternativeDownloadSpeedLimit(int limit); // kB/s
        void setAlternativeUploadSpeedLimit(int limit);
        void setAlternativeSpeedLimitsScheduled(bool scheduled);
        void setAlternativeSpeedLimitsBeginTime(QTime time);
        void setAlternativeSpeedLimitsEndTime(QTime time);
        void setAlternativeSpeedLimitsDays(ServerSettingsData::AlternativeSpeedLimitsDays days);

        void setPeerPort(int port);
        void setRandomPortEnabled(bool enabled);
        void setPortForwardingEnabled(bool enabled);
        void setEncryptionMode(ServerSettingsData::EncryptionMode mode);
        void setUtpEnabled(bool enabled);
        void setPexEnabled(bool enabled);
        void setDhtEnabled(bool enabled);
        void setLpdEnabled(bool enabled);
        void setMaximumPeersPerTorrent(int peers);
        void setMaximumPeersGlobally(int peers);

        [[nodiscard]] bool saveOnSet() const;
        void setSaveOnSet(bool save);

        void update(const QJsonObject& serverSettings);
        void save() const;

        [[nodiscard]] const ServerSettingsData& data() const { return mData; };

    private:
        Rpc* mRpc;
        ServerSettingsData mData;
        bool mSaveOnSet;

    signals:
        void changed();
    };
}

#endif // TREMOTESF_RPC_SERVERSETTINGS_H
