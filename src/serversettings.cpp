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

#include "serversettings.h"

#include "rpc.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        const QString downloadDirectoryKey("download-dir");
        const QString trashTorrentFilesKey("trash-original-torrent-files");
        const QString startAddedTorrentsKey("start-added-torrents");
        const QString renameIncompleteFilesKey("rename-partial-files");
        const QString incompleteDirectoryEnabledKey("incomplete-dir-enabled");
        const QString incompleteDirectoryKey("incomplete-dir");

        const QString ratioLimitedKey("seedRatioLimited");
        const QString ratioLimitKey("seedRatioLimit");
        const QString idleSeedingLimitedKey("idle-seeding-limit-enabled");
        const QString idleSeedingLimitKey("idle-seeding-limit");

        const QString downloadQueueEnabledKey("download-queue-enabled");
        const QString downloadQueueSizeKey("download-queue-size");
        const QString seedQueueEnabledKey("seed-queue-enabled");
        const QString seedQueueSizeKey("seed-queue-size");
        const QString idleQueueLimitedKey("queue-stalled-enabled");
        const QString idleQueueLimitKey("queue-stalled-minutes");

        const QString downloadSpeedLimitedKey("speed-limit-down-enabled");
        const QString downloadSpeedLimitKey("speed-limit-down");
        const QString uploadSpeedLimitedKey("speed-limit-up-enabled");
        const QString uploadSpeedLimitKey("speed-limit-up");
        const QString alternativeSpeedLimitsEnabledKey("alt-speed-enabled");
        const QString alternativeDownloadSpeedLimitKey("alt-speed-down");
        const QString alternativeUploadSpeedLimitKey("alt-speed-up");
        const QString alternativeSpeedLimitsScheduledKey("alt-speed-time-enabled");
        const QString alternativeSpeedLimitsBeginTimeKey("alt-speed-time-begin");
        const QString alternativeSpeedLimitsEndTimeKey("alt-speed-time-end");
        const QString alternativeSpeedLimitsDaysKey("alt-speed-time-day");

        const QString peerPortKey("peer-port");
        const QString randomPortEnabledKey("peer-port-random-on-start");
        const QString portForwardingEnabledKey("port-forwarding-enabled");

        const QString encryptionModeKey("encryption");
        const QString encryptionModeAllowed("tolerated");
        const QString encryptionModePreferred("preferred");
        const QString encryptionModeRequired("required");

        QString encryptionModeString(ServerSettings::EncryptionMode mode)
        {
            switch (mode) {
            case ServerSettings::AllowedEncryption:
                return encryptionModeAllowed;
            case ServerSettings::PreferredEncryption:
                return encryptionModePreferred;
            case ServerSettings::RequiredEncryption:
                return encryptionModeRequired;
            }
            return QString();
        }

        const QString utpEnabledKey("utp-enabled");
        const QString pexEnabledKey("pex-enabled");
        const QString dhtEnabledKey("dht-enabled");
        const QString lpdEnabledKey("lpd-enabled");
        const QString maximumPeersPerTorrentKey("peer-limit-per-torrent");
        const QString maximumPeersGloballyKey("peer-limit-global");
    }

    ServerSettings::ServerSettings(Rpc* rpc, QObject *parent)
        : QObject(parent),
          mRpc(rpc)
    {

    }

    int ServerSettings::rpcVersion() const
    {
        return mRpcVersion;
    }

    int ServerSettings::minimumRpcVersion() const
    {
        return mMinimumRpcVersion;
    }

    const QString& ServerSettings::downloadDirectory() const
    {
        return mDownloadDirectory;
    }

    void ServerSettings::setDownloadDirectory(const QString& directory, bool saveImmediately)
    {
        if (directory != mDownloadDirectory) {
            mDownloadDirectory = directory;
            if (saveImmediately) {
                mRpc->setSessionProperty(downloadDirectoryKey, mDownloadDirectory);
            }
        }
    }

    bool ServerSettings::startAddedTorrents() const
    {
        return mStartAddedTorrents;
    }

    void ServerSettings::setStartAddedTorrents(bool start, bool saveImmediately)
    {
        mStartAddedTorrents = start;
        if (saveImmediately) {
            mRpc->setSessionProperty(startAddedTorrentsKey, mStartAddedTorrents);
        }
    }

    bool ServerSettings::trashTorrentFiles() const
    {
        return mTrashTorrentFiles;
    }

    void ServerSettings::setTrashTorrentFiles(bool trash, bool saveImmediately)
    {
        mTrashTorrentFiles = trash;
        if (saveImmediately) {
            mRpc->setSessionProperty(trashTorrentFilesKey, mTrashTorrentFiles);
        }
    }

    bool ServerSettings::renameIncompleteFiles() const
    {
        return mRenameIncompleteFiles;
    }

    void ServerSettings::setRenameIncompleteFiles(bool rename, bool saveImmediately)
    {
        mRenameIncompleteFiles = rename;
        if (saveImmediately) {
            mRpc->setSessionProperty(renameIncompleteFilesKey, mRenameIncompleteFiles);
        }
    }

    bool ServerSettings::isIncompleteDirectoryEnabled() const
    {
        return mIncompleteDirectoryEnabled;
    }

    void ServerSettings::setIncompleteDirectoryEnabled(bool enabled, bool saveImmediately)
    {
        mIncompleteDirectoryEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(incompleteDirectoryEnabledKey, mIncompleteDirectoryEnabled);
        }
    }

    const QString& ServerSettings::incompleteDirectory() const
    {
        return mIncompleteDirectory;
    }

    void ServerSettings::setIncompleteDirectory(const QString& directory, bool saveImmediately)
    {
        if (directory != mIncompleteDirectory) {
            mIncompleteDirectory = directory;
            qDebug() << mIncompleteDirectory;
            if (saveImmediately) {
                mRpc->setSessionProperty(incompleteDirectoryKey, mIncompleteDirectory);
            }
        }
    }

    bool ServerSettings::isRatioLimited() const
    {
        return mRatioLimited;
    }

    void ServerSettings::setRatioLimited(bool limited, bool saveImmediately)
    {
        mRatioLimited = limited;
        if (saveImmediately) {
            mRpc->setSessionProperty(ratioLimitedKey, mRatioLimited);
        }
    }

    float ServerSettings::ratioLimit() const
    {
        return mRatioLimit;
    }

    void ServerSettings::setRatioLimit(float limit, bool saveImmediately)
    {
        mRatioLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(ratioLimitKey, mRatioLimit);
        }
    }

    bool ServerSettings::isIdleSeedingLimited() const
    {
        return mIdleSeedingLimited;
    }

    void ServerSettings::setIdleSeedingLimited(bool limited, bool saveImmediately)
    {
        mIdleSeedingLimited = limited;
        if (saveImmediately) {
            mRpc->setSessionProperty(idleSeedingLimitedKey, mIdleSeedingLimited);
        }
    }

    int ServerSettings::idleSeedingLimit() const
    {
        return mIdleSeedingLimit;
    }

    void ServerSettings::setIdleSeedingLimit(int limit, bool saveImmediately)
    {
        mIdleSeedingLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(idleSeedingLimitKey, mIdleSeedingLimit);
        }
    }

    bool ServerSettings::isDownloadQueueEnabled() const
    {
        return mDownloadQueueEnabled;
    }

    void ServerSettings::setDownloadQueueEnabled(bool enabled, bool saveImmediately)
    {
        mDownloadQueueEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(downloadQueueEnabledKey, mDownloadQueueEnabled);
        }
    }

    int ServerSettings::downloadQueueSize() const
    {
        return mDownloadQueueSize;
    }

    void ServerSettings::setDownloadQueueSize(int size, bool saveImmediately)
    {
        mDownloadQueueSize = size;
        if (saveImmediately) {
            mRpc->setSessionProperty(downloadQueueSizeKey, mDownloadQueueSize);
        }
    }

    bool ServerSettings::isSeedQueueEnabled() const
    {
        return mSeedQueueEnabled;
    }

    void ServerSettings::setSeedQueueEnabled(bool enabled, bool saveImmediately)
    {
        mSeedQueueEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(seedQueueEnabledKey, mSeedQueueEnabled);
        }
    }

    int ServerSettings::seedQueueSize() const
    {
        return mSeedQueueSize;
    }

    void ServerSettings::setSeedQueueSize(int size, bool saveImmediately)
    {
        mSeedQueueSize = size;
        if (saveImmediately) {
            mRpc->setSessionProperty(seedQueueSizeKey, mSeedQueueSize);
        }
    }

    bool ServerSettings::isIdleQueueLimited() const
    {
        return mIdleQueueLimited;
    }

    void ServerSettings::setIdleQueueLimited(bool limited, bool saveImmediately)
    {
        mIdleQueueLimited = limited;
        if (saveImmediately) {
            mRpc->setSessionProperty(idleQueueLimitedKey, mIdleQueueLimited);
        }
    }

    int ServerSettings::idleQueueLimit() const
    {
        return mIdleQueueLimit;
    }

    void ServerSettings::setIdleQueueLimit(int limit, bool saveImmediately)
    {
        mIdleQueueLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(idleQueueLimitKey, mIdleQueueLimit);
        }
    }

    bool ServerSettings::isDownloadSpeedLimited() const
    {
        return mDownloadSpeedLimited;
    }

    void ServerSettings::setDownloadSpeedLimited(bool limited, bool saveImmediately)
    {
        mDownloadSpeedLimited = limited;
        if (saveImmediately) {
            mRpc->setSessionProperty(downloadSpeedLimitedKey, mDownloadSpeedLimited);
        }
    }

    int ServerSettings::downloadSpeedLimit() const
    {
        return mDownloadSpeedLimit;
    }

    void ServerSettings::setDownloadSpeedLimit(int limit, bool saveImmediately)
    {
        mDownloadSpeedLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(downloadSpeedLimitKey, mDownloadSpeedLimit);
        }
    }

    bool ServerSettings::isUploadSpeedLimited() const
    {
        return mUploadSpeedLimited;
    }

    void ServerSettings::setUploadSpeedLimited(bool limited, bool saveImmediately)
    {
        mUploadSpeedLimited = limited;
        if (saveImmediately) {
            mRpc->setSessionProperty(uploadSpeedLimitedKey, mUploadSpeedLimited);
        }
    }

    int ServerSettings::uploadSpeedLimit() const
    {
        return mUploadSpeedLimit;
    }

    void ServerSettings::setUploadSpeedLimit(int limit, bool saveImmediately)
    {
        mUploadSpeedLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(uploadSpeedLimitKey, mUploadSpeedLimit);
        }
    }

    bool ServerSettings::isAlternativeSpeedLimitsEnabled() const
    {
        return mAlternativeSpeedLimitsEnabled;
    }

    void ServerSettings::setAlternativeSpeedLimitsEnabled(bool enabled, bool saveImmediately)
    {
        mAlternativeSpeedLimitsEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(alternativeSpeedLimitsEnabledKey, mAlternativeSpeedLimitsEnabled);
        }
    }

    int ServerSettings::alternativeDownloadSpeedLimit() const
    {
        return mAlternativeDownloadSpeedLimit;
    }

    void ServerSettings::setAlternativeDownloadSpeedLimit(int limit, bool saveImmediately)
    {
        mAlternativeDownloadSpeedLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(alternativeDownloadSpeedLimitKey, mAlternativeDownloadSpeedLimit);
        }
    }

    int ServerSettings::alternativeUploadSpeedLimit() const
    {
        return mAlternativeUploadSpeedLimit;
    }

    void ServerSettings::setAlternativeUploadSpeedLimit(int limit, bool saveImmediately)
    {
        mAlternativeUploadSpeedLimit = limit;
        if (saveImmediately) {
            mRpc->setSessionProperty(alternativeUploadSpeedLimitKey, mAlternativeUploadSpeedLimit);
        }
    }

    bool ServerSettings::isAlternativeSpeedLimitsScheduled() const
    {
        return mAlternativeSpeedLimitsScheduled;
    }

    void ServerSettings::setAlternativeSpeedLimitsScheduled(bool scheduled, bool saveImmediately)
    {
        mAlternativeSpeedLimitsScheduled = scheduled;
        if (saveImmediately) {
            mRpc->setSessionProperty(alternativeSpeedLimitsScheduledKey, mAlternativeSpeedLimitsScheduled);
        }
    }

    const QTime& ServerSettings::alternativeSpeedLimitsBeginTime() const
    {
        return mAlternativeSpeedLimitsBeginTime;
    }

    void ServerSettings::setAlternativeSpeedLimitsBeginTime(const QTime& time, bool saveImmediately)
    {
        mAlternativeSpeedLimitsBeginTime = time;
        if (saveImmediately) {
            mRpc->setSessionProperty(alternativeSpeedLimitsBeginTimeKey, mAlternativeSpeedLimitsBeginTime.msecsSinceStartOfDay() / 60000);
        }
    }

    const QTime& ServerSettings::alternativeSpeedLimitsEndTime() const
    {
        return mAlternativeSpeedLimitsEndTime;
    }

    void ServerSettings::setAlternativeSpeedLimitsEndTime(const QTime& time, bool saveImmediately)
    {
        mAlternativeSpeedLimitsEndTime = time;
        if (saveImmediately) {
            mRpc->setSessionProperty(alternativeSpeedLimitsEndTimeKey, mAlternativeSpeedLimitsEndTime.msecsSinceStartOfDay() / 60000);
        }
    }

    ServerSettings::AlternativeSpeedLimitsDays ServerSettings::alternativeSpeedLimitsDays() const
    {
        return mAlternativeSpeedLimitsDays;
    }

    void ServerSettings::setAlternativeSpeedLimitsDays(ServerSettings::AlternativeSpeedLimitsDays days, bool saveImmediately)
    {
        if (days != mAlternativeSpeedLimitsDays) {
            mAlternativeSpeedLimitsDays = days;
            if (saveImmediately) {
                mRpc->setSessionProperty(alternativeSpeedLimitsDaysKey, mAlternativeSpeedLimitsDays);
            }
        }
    }

    int ServerSettings::peerPort() const
    {
        return mPeerPort;
    }

    void ServerSettings::setPeerPort(int port, bool saveImmediately)
    {
        mPeerPort = port;
        if (saveImmediately) {
            mRpc->setSessionProperty(peerPortKey, mPeerPort);
        }
    }

    bool ServerSettings::isRandomPortEnabled() const
    {
        return mRandomPortEnabled;
    }

    void ServerSettings::setRandomPortEnabled(bool enabled, bool saveImmediately)
    {
        mRandomPortEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(randomPortEnabledKey, mRandomPortEnabled);
        }
    }

    bool ServerSettings::isPortForwardingEnabled() const
    {
        return mPortForwardingEnabled;
    }

    void ServerSettings::setPortForwardingEnabled(bool enabled, bool saveImmediately)
    {
        mPortForwardingEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(portForwardingEnabledKey, mPortForwardingEnabled);
        }
    }

    ServerSettings::EncryptionMode ServerSettings::encryptionMode() const
    {
        return mEncryptionMode;
    }

    void ServerSettings::setEncryptionMode(ServerSettings::EncryptionMode mode, bool saveImmediately)
    {
        mEncryptionMode = mode;
        if (saveImmediately) {
            mRpc->setSessionProperty(encryptionModeKey, encryptionModeString(mode));
        }
    }

    bool ServerSettings::isUtpEnabled() const
    {
        return mUtpEnabled;
    }

    void ServerSettings::setUtpEnabled(bool enabled, bool saveImmediately)
    {
        mUtpEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(utpEnabledKey, mUtpEnabled);
        }
    }

    bool ServerSettings::isPexEnabled() const
    {
        return mPexEnabled;
    }

    void ServerSettings::setPexEnabled(bool enabled, bool saveImmediately)
    {
        mPexEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(pexEnabledKey, mPexEnabled);
        }
    }

    bool ServerSettings::isDhtEnabled() const
    {
        return mDhtEnabled;
    }

    void ServerSettings::setDhtEnabled(bool enabled, bool saveImmediately)
    {
        mDhtEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(dhtEnabledKey, mDhtEnabled);
        }
    }

    bool ServerSettings::isLpdEnabled() const
    {
        return mLpdEnabled;
    }

    void ServerSettings::setLpdEnabled(bool enabled, bool saveImmediately)
    {
        mLpdEnabled = enabled;
        if (saveImmediately) {
            mRpc->setSessionProperty(lpdEnabledKey, mLpdEnabled);
        }
    }

    int ServerSettings::maximumPeersPerTorrent() const
    {
        return mMaximumPeersPerTorrent;
    }

    void ServerSettings::setMaximumPeersPerTorrent(int peers, bool saveImmediately)
    {
        mMaximumPeersPerTorrent = peers;
        if (saveImmediately) {
            mRpc->setSessionProperty(maximumPeersPerTorrentKey, mMaximumPeersPerTorrent);
        }
    }

    int ServerSettings::maximumPeersGlobally() const
    {
        return mMaximumPeersGlobally;
    }

    void ServerSettings::setMaximumPeersGlobally(int peers, bool saveImmediately)
    {
        mMaximumPeersGlobally = peers;
        if (saveImmediately) {
            qDebug() << mMaximumPeersGlobally;
            mRpc->setSessionProperty(maximumPeersGloballyKey, mMaximumPeersGlobally);
        }
    }

    int ServerSettings::kibiBytes(int kiloBytesOrKibiBytes) const
    {
        if (mUsingDecimalUnits) {
            return kiloBytesOrKibiBytes * 1000 / 1024;
        }
        return kiloBytesOrKibiBytes;
    }

    int ServerSettings::kiloBytesOrKibiBytes(int kibiBytes) const
    {
        if (mUsingDecimalUnits) {
            return kibiBytes * 1024 / 1000;
        }
        return kibiBytes;
    }

    void ServerSettings::update(const QVariantMap& serverSettings)
    {
        mRpcVersion = serverSettings.value("rpc-version").toInt();
        mMinimumRpcVersion = serverSettings.value("rpc-version-minimum").toInt();

        mUsingDecimalUnits = (serverSettings.value("units").toMap().value("speed-bytes").toInt() == 1000);

        mDownloadDirectory = serverSettings.value(downloadDirectoryKey).toString();
        mTrashTorrentFiles = serverSettings.value(trashTorrentFilesKey).toBool();
        mStartAddedTorrents = serverSettings.value(startAddedTorrentsKey).toBool();
        mRenameIncompleteFiles = serverSettings.value(renameIncompleteFilesKey).toBool();
        mIncompleteDirectoryEnabled = serverSettings.value(incompleteDirectoryEnabledKey).toBool();
        mIncompleteDirectory = serverSettings.value(incompleteDirectoryKey).toString();

        mRatioLimited = serverSettings.value(ratioLimitedKey).toBool();
        mRatioLimit = serverSettings.value(ratioLimitKey).toFloat();
        mIdleSeedingLimited = serverSettings.value(idleSeedingLimitedKey).toBool();
        mIdleSeedingLimit = serverSettings.value(idleSeedingLimitKey).toInt();

        mDownloadQueueEnabled = serverSettings.value(downloadQueueEnabledKey).toBool();
        mDownloadQueueSize = serverSettings.value(downloadQueueSizeKey).toInt();
        mSeedQueueEnabled = serverSettings.value(seedQueueEnabledKey).toBool();
        mSeedQueueSize = serverSettings.value(seedQueueSizeKey).toInt();
        mIdleQueueLimited = serverSettings.value(idleQueueLimitedKey).toBool();
        mIdleQueueLimit = serverSettings.value(idleQueueLimitKey).toInt();

        mDownloadSpeedLimited = serverSettings.value(downloadSpeedLimitedKey).toBool();
        mDownloadSpeedLimit = kibiBytes(serverSettings.value(downloadSpeedLimitKey).toInt());
        mUploadSpeedLimited = serverSettings.value(uploadSpeedLimitedKey).toBool();
        mUploadSpeedLimit = kibiBytes(serverSettings.value(uploadSpeedLimitKey).toInt());
        mAlternativeSpeedLimitsEnabled = serverSettings.value(alternativeSpeedLimitsEnabledKey).toBool();
        mAlternativeDownloadSpeedLimit = kibiBytes(serverSettings.value(alternativeDownloadSpeedLimitKey).toInt());
        mAlternativeUploadSpeedLimit = kibiBytes(serverSettings.value(alternativeUploadSpeedLimitKey).toInt());
        mAlternativeSpeedLimitsScheduled = serverSettings.value(alternativeSpeedLimitsScheduledKey).toBool();
        mAlternativeSpeedLimitsBeginTime = QTime::fromMSecsSinceStartOfDay(serverSettings.value(alternativeSpeedLimitsBeginTimeKey).toInt() * 60000);
        mAlternativeSpeedLimitsEndTime = QTime::fromMSecsSinceStartOfDay(serverSettings.value(alternativeSpeedLimitsEndTimeKey).toInt() * 60000);
        mAlternativeSpeedLimitsDays = static_cast<AlternativeSpeedLimitsDays>(serverSettings.value(alternativeSpeedLimitsDaysKey).toInt());

        mPeerPort = serverSettings.value(peerPortKey).toInt();
        mRandomPortEnabled = serverSettings.value(randomPortEnabledKey).toBool();
        mPortForwardingEnabled = serverSettings.value(portForwardingEnabledKey).toBool();

        const QString encryption(serverSettings.value(encryptionModeKey).toString());
        if (encryption == encryptionModeAllowed) {
            mEncryptionMode = AllowedEncryption;
        } else if (encryption == encryptionModePreferred) {
            mEncryptionMode = PreferredEncryption;
        } else {
            mEncryptionMode = RequiredEncryption;
        }

        mUtpEnabled = serverSettings.value(utpEnabledKey).toBool();
        mPexEnabled = serverSettings.value(pexEnabledKey).toBool();
        mDhtEnabled = serverSettings.value(dhtEnabledKey).toBool();
        mLpdEnabled = serverSettings.value(lpdEnabledKey).toBool();
        mMaximumPeersPerTorrent = serverSettings.value(maximumPeersPerTorrentKey).toInt();
        mMaximumPeersGlobally = serverSettings.value(maximumPeersGloballyKey).toInt();
    }

    void ServerSettings::save() const
    {
        mRpc->setSessionProperties({{downloadDirectoryKey, mDownloadDirectory},
                                    {trashTorrentFilesKey, mTrashTorrentFiles},
                                    {startAddedTorrentsKey, mStartAddedTorrents},
                                    {renameIncompleteFilesKey, mRenameIncompleteFiles},
                                    {incompleteDirectoryEnabledKey, mIncompleteDirectoryEnabled},
                                    {incompleteDirectoryKey, mIncompleteDirectory},

                                    {ratioLimitedKey, mRatioLimited},
                                    {ratioLimitKey, mRatioLimit},
                                    {idleSeedingLimitedKey, mIdleSeedingLimit},
                                    {idleSeedingLimitKey, mIdleSeedingLimit},

                                    {downloadQueueEnabledKey, mDownloadQueueEnabled},
                                    {downloadQueueSizeKey, mDownloadQueueSize},
                                    {seedQueueEnabledKey, mSeedQueueEnabled},
                                    {seedQueueSizeKey, mSeedQueueSize},
                                    {idleQueueLimitedKey, mIdleQueueLimited},
                                    {idleQueueLimitKey, mIdleQueueLimit},

                                    {downloadSpeedLimitedKey, mDownloadSpeedLimited},
                                    {downloadSpeedLimitKey, mDownloadSpeedLimit},
                                    {uploadSpeedLimitedKey, mUploadSpeedLimited},
                                    {uploadSpeedLimitKey, mUploadSpeedLimit},
                                    {alternativeSpeedLimitsEnabledKey, mAlternativeSpeedLimitsEnabled},
                                    {alternativeDownloadSpeedLimitKey, mAlternativeDownloadSpeedLimit},
                                    {alternativeUploadSpeedLimitKey, mAlternativeUploadSpeedLimit},
                                    {alternativeSpeedLimitsScheduledKey, mAlternativeSpeedLimitsScheduled},
                                    {alternativeSpeedLimitsBeginTimeKey, mAlternativeSpeedLimitsBeginTime.msecsSinceStartOfDay() / 60000},
                                    {alternativeSpeedLimitsEndTimeKey, mAlternativeSpeedLimitsEndTime.msecsSinceStartOfDay() / 60000},
                                    {alternativeSpeedLimitsDaysKey, mAlternativeSpeedLimitsDays},

                                    {peerPortKey, mPeerPort},
                                    {randomPortEnabledKey, mRandomPortEnabled},
                                    {portForwardingEnabledKey, mPortForwardingEnabled},
                                    {encryptionModeKey, encryptionModeString(mEncryptionMode)},
                                    {utpEnabledKey, mUtpEnabled},
                                    {pexEnabledKey, mPexEnabled},
                                    {dhtEnabledKey, mDhtEnabled},
                                    {lpdEnabledKey, mLpdEnabled},
                                    {maximumPeersPerTorrentKey, mMaximumPeersPerTorrent},
                                    {maximumPeersGloballyKey, mMaximumPeersGlobally}});
    }
}
