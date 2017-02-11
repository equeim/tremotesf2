/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_SERVERSETTINGS_H
#define TREMOTESF_SERVERSETTINGS_H

#include <QObject>
#include <QTime>
#include <QVariantMap>

namespace tremotesf
{
    class Rpc;

    class ServerSettings : public QObject
    {
        Q_OBJECT

        Q_ENUMS(AlternativeSpeedLimitsDays)
        Q_ENUMS(EncryptionMode)

        Q_PROPERTY(QString downloadDirectory READ downloadDirectory WRITE setDownloadDirectory)
        Q_PROPERTY(bool startAddedTorrents READ startAddedTorrents WRITE setStartAddedTorrents)
        Q_PROPERTY(bool trashTorrentFiles READ trashTorrentFiles WRITE setTrashTorrentFiles)
        Q_PROPERTY(bool renameIncompleteFiles READ renameIncompleteFiles WRITE setRenameIncompleteFiles)
        Q_PROPERTY(bool incompleteDirectoryEnabled READ isIncompleteDirectoryEnabled WRITE setIncompleteDirectoryEnabled)
        Q_PROPERTY(QString incompleteDirectory READ incompleteDirectory WRITE setIncompleteDirectory)

        Q_PROPERTY(bool ratioLimited READ isRatioLimited WRITE setRatioLimited)
        Q_PROPERTY(float ratioLimit READ ratioLimit WRITE setRatioLimit)
        Q_PROPERTY(bool idleSeedingLimited READ isIdleSeedingLimited WRITE setIdleSeedingLimited)
        Q_PROPERTY(int idleSeedingLimit READ idleSeedingLimit WRITE setIdleSeedingLimit)

        Q_PROPERTY(bool downloadQueueEnabled READ isDownloadQueueEnabled WRITE setDownloadQueueEnabled)
        Q_PROPERTY(int downloadQueueSize READ downloadQueueSize WRITE setDownloadQueueSize)
        Q_PROPERTY(bool seedQueueEnabled READ isSeedQueueEnabled WRITE setSeedQueueEnabled)
        Q_PROPERTY(int seedQueueSize READ seedQueueSize WRITE setSeedQueueSize)
        Q_PROPERTY(bool idleQueueLimited READ isIdleQueueLimited WRITE setIdleQueueLimited)
        Q_PROPERTY(int idleQueueLimit READ idleQueueLimit WRITE setIdleQueueLimit)

        Q_PROPERTY(bool downloadSpeedLimited READ isDownloadSpeedLimited WRITE setDownloadSpeedLimited)
        Q_PROPERTY(int downloadSpeedLimit READ downloadSpeedLimit WRITE setDownloadSpeedLimit)
        Q_PROPERTY(bool uploadSpeedLimited READ isUploadSpeedLimited WRITE setUploadSpeedLimited)
        Q_PROPERTY(int uploadSpeedLimit READ uploadSpeedLimit WRITE setUploadSpeedLimit)
        Q_PROPERTY(bool alternativeSpeedLimitsEnabled READ isAlternativeSpeedLimitsEnabled WRITE setAlternativeSpeedLimitsEnabled)
        Q_PROPERTY(int alternativeDownloadSpeedLimit READ alternativeDownloadSpeedLimit WRITE setAlternativeDownloadSpeedLimit)
        Q_PROPERTY(int alternativeUploadSpeedLimit READ alternativeUploadSpeedLimit WRITE setAlternativeUploadSpeedLimit)
        Q_PROPERTY(bool alternativeSpeedLimitsScheduled READ isAlternativeSpeedLimitsScheduled WRITE setAlternativeSpeedLimitsScheduled)
        Q_PROPERTY(QTime alternativeSpeedLimitsBeginTime READ alternativeSpeedLimitsBeginTime WRITE setAlternativeSpeedLimitsBeginTime)
        Q_PROPERTY(QTime alternativeSpeedLimitsEndTime READ alternativeSpeedLimitsEndTime WRITE setAlternativeSpeedLimitsEndTime)
        Q_PROPERTY(AlternativeSpeedLimitsDays alternativeSpeedLimitsDays READ alternativeSpeedLimitsDays WRITE setAlternativeSpeedLimitsDays)

        Q_PROPERTY(int peerPort READ peerPort WRITE setPeerPort)
        Q_PROPERTY(bool randomPortEnabled READ isRandomPortEnabled WRITE setRandomPortEnabled)
        Q_PROPERTY(bool portForwardingEnabled READ isPortForwardingEnabled WRITE setPortForwardingEnabled)
        Q_PROPERTY(EncryptionMode encryptionMode READ encryptionMode WRITE setEncryptionMode)
        Q_PROPERTY(bool utpEnabled READ isUtpEnabled WRITE setUtpEnabled)
        Q_PROPERTY(bool pexEnabled READ isPexEnabled WRITE setPexEnabled)
        Q_PROPERTY(bool dhtEnabled READ isDhtEnabled WRITE setDhtEnabled)
        Q_PROPERTY(bool lpdEnabled READ isLpdEnabled WRITE setLpdEnabled)
        Q_PROPERTY(int maximumPeerPerTorrent READ maximumPeersPerTorrent WRITE setMaximumPeersPerTorrent)
        Q_PROPERTY(int maximumPeersGlobally READ maximumPeersGlobally WRITE setMaximumPeersGlobally)

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

        enum EncryptionMode
        {
            AllowedEncryption,
            PreferredEncryption,
            RequiredEncryption
        };

        explicit ServerSettings(Rpc* rpc, QObject* parent = nullptr);

        int rpcVersion() const;
        int minimumRpcVersion() const;

        const QString& downloadDirectory() const;
        void setDownloadDirectory(const QString& directory, bool saveImmediately = true);
        bool startAddedTorrents() const;
        void setStartAddedTorrents(bool start, bool saveImmediately = true);
        bool trashTorrentFiles() const;
        void setTrashTorrentFiles(bool trash, bool saveImmediately = true);
        bool renameIncompleteFiles() const;
        void setRenameIncompleteFiles(bool rename, bool saveImmediately = true);
        bool isIncompleteDirectoryEnabled() const;
        void setIncompleteDirectoryEnabled(bool enabled, bool saveImmediately = true);
        const QString& incompleteDirectory() const;
        void setIncompleteDirectory(const QString& directory, bool saveImmediately = true);

        bool isRatioLimited() const;
        void setRatioLimited(bool limited, bool saveImmediately = true);
        float ratioLimit() const;
        void setRatioLimit(float limit, bool saveImmediately = true);
        bool isIdleSeedingLimited() const;
        void setIdleSeedingLimited(bool limited, bool saveImmediately = true);
        int idleSeedingLimit() const;
        void setIdleSeedingLimit(int limit, bool saveImmediately = true);

        bool isDownloadQueueEnabled() const;
        void setDownloadQueueEnabled(bool enabled, bool saveImmediately = true);
        int downloadQueueSize() const;
        void setDownloadQueueSize(int size, bool saveImmediately = true);
        bool isSeedQueueEnabled() const;
        void setSeedQueueEnabled(bool enabled, bool saveImmediately = true);
        int seedQueueSize() const;
        void setSeedQueueSize(int size, bool saveImmediately = true);
        bool isIdleQueueLimited() const;
        void setIdleQueueLimited(bool limited, bool saveImmediately = true);
        int idleQueueLimit() const;
        void setIdleQueueLimit(int limit, bool saveImmediately = true);

        bool isDownloadSpeedLimited() const;
        void setDownloadSpeedLimited(bool limited, bool saveImmediately = true);
        int downloadSpeedLimit() const;
        void setDownloadSpeedLimit(int limit, bool saveImmediately = true);
        bool isUploadSpeedLimited() const;
        void setUploadSpeedLimited(bool limited, bool saveImmediately = true);
        int uploadSpeedLimit() const;
        void setUploadSpeedLimit(int limit, bool saveImmediately = true);
        bool isAlternativeSpeedLimitsEnabled() const;
        void setAlternativeSpeedLimitsEnabled(bool enabled, bool saveImmediately = true);
        int alternativeDownloadSpeedLimit() const;
        void setAlternativeDownloadSpeedLimit(int limit, bool saveImmediately = true);
        int alternativeUploadSpeedLimit() const;
        void setAlternativeUploadSpeedLimit(int limit, bool saveImmediately = true);
        bool isAlternativeSpeedLimitsScheduled() const;
        void setAlternativeSpeedLimitsScheduled(bool scheduled, bool saveImmediately = true);
        const QTime& alternativeSpeedLimitsBeginTime() const;
        void setAlternativeSpeedLimitsBeginTime(const QTime& time, bool saveImmediately = true);
        const QTime& alternativeSpeedLimitsEndTime() const;
        void setAlternativeSpeedLimitsEndTime(const QTime& time, bool saveImmediately = true);
        AlternativeSpeedLimitsDays alternativeSpeedLimitsDays() const;
        void setAlternativeSpeedLimitsDays(AlternativeSpeedLimitsDays days, bool saveImmediately = true);

        int peerPort() const;
        void setPeerPort(int port, bool saveImmediately = true);
        bool isRandomPortEnabled() const;
        void setRandomPortEnabled(bool enabled, bool saveImmediately = true);
        bool isPortForwardingEnabled() const;
        void setPortForwardingEnabled(bool enabled, bool saveImmediately = true);
        EncryptionMode encryptionMode() const;
        void setEncryptionMode(EncryptionMode mode, bool saveImmediately = true);
        bool isUtpEnabled() const;
        void setUtpEnabled(bool enabled, bool saveImmediately = true);
        bool isPexEnabled() const;
        void setPexEnabled(bool enabled, bool saveImmediately = true);
        bool isDhtEnabled() const;
        void setDhtEnabled(bool enabled, bool saveImmediately = true);
        bool isLpdEnabled() const;
        void setLpdEnabled(bool enabled, bool saveImmediately = true);
        int maximumPeersPerTorrent() const;
        void setMaximumPeersPerTorrent(int peers, bool saveImmediately = true);
        int maximumPeersGlobally() const;
        void setMaximumPeersGlobally(int peers, bool saveImmediately = true);

        int kibiBytes(int kiloBytesOrKibiBytes) const;
        int kiloBytesOrKibiBytes(int kibiBytes) const;

        void update(const QVariantMap& serverSettings);
        void save() const;

    private:
        Rpc* mRpc;

        int mRpcVersion;
        int mMinimumRpcVersion;

        bool mUsingDecimalUnits;

        QString mDownloadDirectory;
        bool mStartAddedTorrents;
        bool mTrashTorrentFiles;
        bool mRenameIncompleteFiles;
        bool mIncompleteDirectoryEnabled;
        QString mIncompleteDirectory;

        bool mRatioLimited;
        float mRatioLimit;
        bool mIdleSeedingLimited;
        int mIdleSeedingLimit;

        bool mDownloadQueueEnabled;
        int mDownloadQueueSize;
        bool mSeedQueueEnabled;
        int mSeedQueueSize;
        bool mIdleQueueLimited;
        int mIdleQueueLimit;

        bool mDownloadSpeedLimited;
        int mDownloadSpeedLimit;
        bool mUploadSpeedLimited;
        int mUploadSpeedLimit;
        bool mAlternativeSpeedLimitsEnabled;
        int mAlternativeDownloadSpeedLimit;
        int mAlternativeUploadSpeedLimit;
        bool mAlternativeSpeedLimitsScheduled;
        QTime mAlternativeSpeedLimitsBeginTime;
        QTime mAlternativeSpeedLimitsEndTime;
        AlternativeSpeedLimitsDays mAlternativeSpeedLimitsDays;

        int mPeerPort;
        bool mRandomPortEnabled;
        bool mPortForwardingEnabled;
        EncryptionMode mEncryptionMode;
        bool mUtpEnabled;
        bool mPexEnabled;
        bool mDhtEnabled;
        bool mLpdEnabled;
        int mMaximumPeersPerTorrent;
        int mMaximumPeersGlobally;
    };
}

#endif
