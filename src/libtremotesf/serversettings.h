/*
 * Tremotesf
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

#ifndef LIBTREMOTESF_SERVERSETTINGS_H
#define LIBTREMOTESF_SERVERSETTINGS_H

#include <QObject>
#include <QTime>

class QJsonObject;

namespace libtremotesf
{
    class Rpc;

    struct ServerSettingsData
    {
        Q_GADGET
    public:
        enum AlternativeSpeedLimitsDays
        {
            Sunday = (1 << 0),
            Monday = (1 << 1),
            Tuesday = (1 << 2),
            Wednesday = (1 << 3),
            Thursday = (1 << 4),
            Friday = (1 << 5),
            Saturday = (1 << 6),
            Weekdays = (Monday | Tuesday | Wednesday | Thursday | Friday),
            Weekends = (Sunday | Saturday),
            All = (Weekdays | Weekends)
        };
        Q_ENUM(AlternativeSpeedLimitsDays)

        enum EncryptionMode
        {
            AllowedEncryption,
            PreferredEncryption,
            RequiredEncryption
        };
        Q_ENUM(EncryptionMode)

        bool canRenameFiles() const;
        bool canShowFreeSpaceForPath() const;

        int rpcVersion = 0;
        int minimumRpcVersion = 0;

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
        AlternativeSpeedLimitsDays alternativeSpeedLimitsDays = All;

        int peerPort = 0;
        bool randomPortEnabled = false;
        bool portForwardingEnabled = false;
        EncryptionMode encryptionMode = PreferredEncryption;
        bool utpEnabled = false;
        bool pexEnabled = false;
        bool dhtEnabled = false;
        bool lpdEnabled = false;
        int maximumPeersPerTorrent = 0;
        int maximumPeersGlobally = 0;
    };

    class ServerSettings : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(bool canRenameFiles READ canRenameFiles NOTIFY changed)
        Q_PROPERTY(bool canShowFreeSpaceForPath READ canShowFreeSpaceForPath NOTIFY changed)

        Q_PROPERTY(QString downloadDirectory READ downloadDirectory WRITE setDownloadDirectory NOTIFY changed)
        Q_PROPERTY(bool startAddedTorrents READ startAddedTorrents WRITE setStartAddedTorrents NOTIFY changed)
        Q_PROPERTY(bool trashTorrentFiles READ trashTorrentFiles WRITE setTrashTorrentFiles NOTIFY changed)
        Q_PROPERTY(bool renameIncompleteFiles READ renameIncompleteFiles WRITE setRenameIncompleteFiles NOTIFY changed)
        Q_PROPERTY(bool incompleteDirectoryEnabled READ isIncompleteDirectoryEnabled WRITE setIncompleteDirectoryEnabled NOTIFY changed)
        Q_PROPERTY(QString incompleteDirectory READ incompleteDirectory WRITE setIncompleteDirectory NOTIFY changed)

        Q_PROPERTY(bool ratioLimited READ isRatioLimited WRITE setRatioLimited NOTIFY changed)
        Q_PROPERTY(double ratioLimit READ ratioLimit WRITE setRatioLimit NOTIFY changed)
        Q_PROPERTY(bool idleSeedingLimited READ isIdleSeedingLimited WRITE setIdleSeedingLimited NOTIFY changed)
        Q_PROPERTY(int idleSeedingLimit READ idleSeedingLimit WRITE setIdleSeedingLimit NOTIFY changed)

        Q_PROPERTY(bool downloadQueueEnabled READ isDownloadQueueEnabled WRITE setDownloadQueueEnabled NOTIFY changed)
        Q_PROPERTY(int downloadQueueSize READ downloadQueueSize WRITE setDownloadQueueSize NOTIFY changed)
        Q_PROPERTY(bool seedQueueEnabled READ isSeedQueueEnabled WRITE setSeedQueueEnabled NOTIFY changed)
        Q_PROPERTY(int seedQueueSize READ seedQueueSize WRITE setSeedQueueSize NOTIFY changed)
        Q_PROPERTY(bool idleQueueLimited READ isIdleQueueLimited WRITE setIdleQueueLimited NOTIFY changed)
        Q_PROPERTY(int idleQueueLimit READ idleQueueLimit WRITE setIdleQueueLimit NOTIFY changed)

        Q_PROPERTY(bool downloadSpeedLimited READ isDownloadSpeedLimited WRITE setDownloadSpeedLimited NOTIFY changed)
        Q_PROPERTY(int downloadSpeedLimit READ downloadSpeedLimit WRITE setDownloadSpeedLimit NOTIFY changed)
        Q_PROPERTY(bool uploadSpeedLimited READ isUploadSpeedLimited WRITE setUploadSpeedLimited NOTIFY changed)
        Q_PROPERTY(int uploadSpeedLimit READ uploadSpeedLimit WRITE setUploadSpeedLimit NOTIFY changed)
        Q_PROPERTY(bool alternativeSpeedLimitsEnabled READ isAlternativeSpeedLimitsEnabled WRITE setAlternativeSpeedLimitsEnabled NOTIFY changed)
        Q_PROPERTY(int alternativeDownloadSpeedLimit READ alternativeDownloadSpeedLimit WRITE setAlternativeDownloadSpeedLimit NOTIFY changed)
        Q_PROPERTY(int alternativeUploadSpeedLimit READ alternativeUploadSpeedLimit WRITE setAlternativeUploadSpeedLimit NOTIFY changed)
        Q_PROPERTY(bool alternativeSpeedLimitsScheduled READ isAlternativeSpeedLimitsScheduled WRITE setAlternativeSpeedLimitsScheduled NOTIFY changed)
        Q_PROPERTY(QTime alternativeSpeedLimitsBeginTime READ alternativeSpeedLimitsBeginTime WRITE setAlternativeSpeedLimitsBeginTime NOTIFY changed)
        Q_PROPERTY(QTime alternativeSpeedLimitsEndTime READ alternativeSpeedLimitsEndTime WRITE setAlternativeSpeedLimitsEndTime NOTIFY changed)
        Q_PROPERTY(libtremotesf::ServerSettingsData::AlternativeSpeedLimitsDays alternativeSpeedLimitsDays READ alternativeSpeedLimitsDays WRITE setAlternativeSpeedLimitsDays NOTIFY changed)

        Q_PROPERTY(int peerPort READ peerPort WRITE setPeerPort NOTIFY changed)
        Q_PROPERTY(bool randomPortEnabled READ isRandomPortEnabled WRITE setRandomPortEnabled NOTIFY changed)
        Q_PROPERTY(bool portForwardingEnabled READ isPortForwardingEnabled WRITE setPortForwardingEnabled NOTIFY changed)
        Q_PROPERTY(libtremotesf::ServerSettingsData::EncryptionMode encryptionMode READ encryptionMode WRITE setEncryptionMode NOTIFY changed)
        Q_PROPERTY(bool utpEnabled READ isUtpEnabled WRITE setUtpEnabled NOTIFY changed)
        Q_PROPERTY(bool pexEnabled READ isPexEnabled WRITE setPexEnabled NOTIFY changed)
        Q_PROPERTY(bool dhtEnabled READ isDhtEnabled WRITE setDhtEnabled NOTIFY changed)
        Q_PROPERTY(bool lpdEnabled READ isLpdEnabled WRITE setLpdEnabled NOTIFY changed)
        Q_PROPERTY(int maximumPeerPerTorrent READ maximumPeersPerTorrent WRITE setMaximumPeersPerTorrent NOTIFY changed)
        Q_PROPERTY(int maximumPeersGlobally READ maximumPeersGlobally WRITE setMaximumPeersGlobally NOTIFY changed)

    public:
        explicit ServerSettings(Rpc* rpc = nullptr, QObject* parent = nullptr);

        int rpcVersion() const;
        int minimumRpcVersion() const;

        bool canRenameFiles() const;
        bool canShowFreeSpaceForPath() const;

        const QString& downloadDirectory() const;
        Q_INVOKABLE void setDownloadDirectory(const QString& directory);
        bool startAddedTorrents() const;
        Q_INVOKABLE void setStartAddedTorrents(bool start);
        bool trashTorrentFiles() const;
        Q_INVOKABLE void setTrashTorrentFiles(bool trash);
        bool renameIncompleteFiles() const;
        Q_INVOKABLE void setRenameIncompleteFiles(bool rename);
        bool isIncompleteDirectoryEnabled() const;
        Q_INVOKABLE void setIncompleteDirectoryEnabled(bool enabled);
        const QString& incompleteDirectory() const;
        Q_INVOKABLE void setIncompleteDirectory(const QString& directory);

        bool isRatioLimited() const;
        Q_INVOKABLE void setRatioLimited(bool limited);
        double ratioLimit() const;
        Q_INVOKABLE void setRatioLimit(double limit);
        bool isIdleSeedingLimited() const;
        Q_INVOKABLE void setIdleSeedingLimited(bool limited);
        int idleSeedingLimit() const;
        Q_INVOKABLE void setIdleSeedingLimit(int limit);

        bool isDownloadQueueEnabled() const;
        Q_INVOKABLE void setDownloadQueueEnabled(bool enabled);
        int downloadQueueSize() const;
        Q_INVOKABLE void setDownloadQueueSize(int size);
        bool isSeedQueueEnabled() const;
        Q_INVOKABLE void setSeedQueueEnabled(bool enabled);
        int seedQueueSize() const;
        Q_INVOKABLE void setSeedQueueSize(int size);
        bool isIdleQueueLimited() const;
        Q_INVOKABLE void setIdleQueueLimited(bool limited);
        int idleQueueLimit() const;
        Q_INVOKABLE void setIdleQueueLimit(int limit);

        bool isDownloadSpeedLimited() const;
        Q_INVOKABLE void setDownloadSpeedLimited(bool limited);
        int downloadSpeedLimit() const; // kB/s
        Q_INVOKABLE void setDownloadSpeedLimit(int limit);
        bool isUploadSpeedLimited() const;
        Q_INVOKABLE void setUploadSpeedLimited(bool limited);
        int uploadSpeedLimit() const; // kB/s
        Q_INVOKABLE void setUploadSpeedLimit(int limit);
        bool isAlternativeSpeedLimitsEnabled() const;
        Q_INVOKABLE void setAlternativeSpeedLimitsEnabled(bool enabled);
        int alternativeDownloadSpeedLimit() const; // kB/s
        Q_INVOKABLE void setAlternativeDownloadSpeedLimit(int limit); // kB/s
        int alternativeUploadSpeedLimit() const; // kB/s
        Q_INVOKABLE void setAlternativeUploadSpeedLimit(int limit);
        bool isAlternativeSpeedLimitsScheduled() const;
        Q_INVOKABLE void setAlternativeSpeedLimitsScheduled(bool scheduled);
        QTime alternativeSpeedLimitsBeginTime() const;
        Q_INVOKABLE void setAlternativeSpeedLimitsBeginTime(QTime time);
        QTime alternativeSpeedLimitsEndTime() const;
        Q_INVOKABLE void setAlternativeSpeedLimitsEndTime(QTime time);
        ServerSettingsData::AlternativeSpeedLimitsDays alternativeSpeedLimitsDays() const;
        Q_INVOKABLE void setAlternativeSpeedLimitsDays(libtremotesf::ServerSettingsData::AlternativeSpeedLimitsDays days);

        int peerPort() const;
        Q_INVOKABLE void setPeerPort(int port);
        bool isRandomPortEnabled() const;
        Q_INVOKABLE void setRandomPortEnabled(bool enabled);
        bool isPortForwardingEnabled() const;
        Q_INVOKABLE void setPortForwardingEnabled(bool enabled);
        ServerSettingsData::EncryptionMode encryptionMode() const;
        Q_INVOKABLE void setEncryptionMode(libtremotesf::ServerSettingsData::EncryptionMode mode);
        bool isUtpEnabled() const;
        Q_INVOKABLE void setUtpEnabled(bool enabled);
        bool isPexEnabled() const;
        Q_INVOKABLE void setPexEnabled(bool enabled);
        bool isDhtEnabled() const;
        Q_INVOKABLE void setDhtEnabled(bool enabled);
        bool isLpdEnabled() const;
        Q_INVOKABLE void setLpdEnabled(bool enabled);
        int maximumPeersPerTorrent() const;
        Q_INVOKABLE void setMaximumPeersPerTorrent(int peers);
        int maximumPeersGlobally() const;
        Q_INVOKABLE void setMaximumPeersGlobally(int peers);

        bool saveOnSet() const;
        void setSaveOnSet(bool save);

        void update(const QJsonObject& serverSettings);
        void save() const;

        const ServerSettingsData& data() const;
    private:
        Rpc* mRpc;
        ServerSettingsData mData;
        bool mSaveOnSet;

    signals:
        void changed();
    };
}

#endif // LIBTREMOTESF_SERVERSETTINGS_H
