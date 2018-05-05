/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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
        const QString downloadDirectoryKey(QLatin1String("download-dir"));
        const QString trashTorrentFilesKey(QLatin1String("trash-original-torrent-files"));
        const QString startAddedTorrentsKey(QLatin1String("start-added-torrents"));
        const QString renameIncompleteFilesKey(QLatin1String("rename-partial-files"));
        const QString incompleteDirectoryEnabledKey(QLatin1String("incomplete-dir-enabled"));
        const QString incompleteDirectoryKey(QLatin1String("incomplete-dir"));

        const QString ratioLimitedKey(QLatin1String("seedRatioLimited"));
        const QString ratioLimitKey(QLatin1String("seedRatioLimit"));
        const QString idleSeedingLimitedKey(QLatin1String("idle-seeding-limit-enabled"));
        const QString idleSeedingLimitKey(QLatin1String("idle-seeding-limit"));

        const QString downloadQueueEnabledKey(QLatin1String("download-queue-enabled"));
        const QString downloadQueueSizeKey(QLatin1String("download-queue-size"));
        const QString seedQueueEnabledKey(QLatin1String("seed-queue-enabled"));
        const QString seedQueueSizeKey(QLatin1String("seed-queue-size"));
        const QString idleQueueLimitedKey(QLatin1String("queue-stalled-enabled"));
        const QString idleQueueLimitKey(QLatin1String("queue-stalled-minutes"));

        const QString downloadSpeedLimitedKey(QLatin1String("speed-limit-down-enabled"));
        const QString downloadSpeedLimitKey(QLatin1String("speed-limit-down"));
        const QString uploadSpeedLimitedKey(QLatin1String("speed-limit-up-enabled"));
        const QString uploadSpeedLimitKey(QLatin1String("speed-limit-up"));
        const QString alternativeSpeedLimitsEnabledKey(QLatin1String("alt-speed-enabled"));
        const QString alternativeDownloadSpeedLimitKey(QLatin1String("alt-speed-down"));
        const QString alternativeUploadSpeedLimitKey(QLatin1String("alt-speed-up"));
        const QString alternativeSpeedLimitsScheduledKey(QLatin1String("alt-speed-time-enabled"));
        const QString alternativeSpeedLimitsBeginTimeKey(QLatin1String("alt-speed-time-begin"));
        const QString alternativeSpeedLimitsEndTimeKey(QLatin1String("alt-speed-time-end"));
        const QString alternativeSpeedLimitsDaysKey(QLatin1String("alt-speed-time-day"));

        const QString peerPortKey(QLatin1String("peer-port"));
        const QString randomPortEnabledKey(QLatin1String("peer-port-random-on-start"));
        const QString portForwardingEnabledKey(QLatin1String("port-forwarding-enabled"));

        const QString encryptionModeKey(QLatin1String("encryption"));
        const QString encryptionModeAllowed(QLatin1String("tolerated"));
        const QString encryptionModePreferred(QLatin1String("preferred"));
        const QString encryptionModeRequired(QLatin1String("required"));

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

        const QString utpEnabledKey(QLatin1String("utp-enabled"));
        const QString pexEnabledKey(QLatin1String("pex-enabled"));
        const QString dhtEnabledKey(QLatin1String("dht-enabled"));
        const QString lpdEnabledKey(QLatin1String("lpd-enabled"));
        const QString maximumPeersPerTorrentKey(QLatin1String("peer-limit-per-torrent"));
        const QString maximumPeersGloballyKey(QLatin1String("peer-limit-global"));
    }

    ServerSettings::ServerSettings(Rpc* rpc, QObject* parent)
        : QObject(parent),
          mRpc(rpc),
          mSaveOnSet(true)
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

    bool ServerSettings::canRenameFiles() const
    {
        return (mRpcVersion >= 15);
    }

    bool ServerSettings::canShowFreeSpaceForPath() const
    {
        return (mRpcVersion >= 15);
    }

    const QString& ServerSettings::downloadDirectory() const
    {
        return mDownloadDirectory;
    }

    void ServerSettings::setDownloadDirectory(const QString& directory)
    {
        if (directory != mDownloadDirectory) {
            mDownloadDirectory = directory;
            if (mSaveOnSet) {
                mRpc->setSessionProperty(downloadDirectoryKey, mDownloadDirectory);
            }
        }
    }

    bool ServerSettings::startAddedTorrents() const
    {
        return mStartAddedTorrents;
    }

    void ServerSettings::setStartAddedTorrents(bool start)
    {
        mStartAddedTorrents = start;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(startAddedTorrentsKey, mStartAddedTorrents);
        }
    }

    bool ServerSettings::trashTorrentFiles() const
    {
        return mTrashTorrentFiles;
    }

    void ServerSettings::setTrashTorrentFiles(bool trash)
    {
        mTrashTorrentFiles = trash;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(trashTorrentFilesKey, mTrashTorrentFiles);
        }
    }

    bool ServerSettings::renameIncompleteFiles() const
    {
        return mRenameIncompleteFiles;
    }

    void ServerSettings::setRenameIncompleteFiles(bool rename)
    {
        mRenameIncompleteFiles = rename;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(renameIncompleteFilesKey, mRenameIncompleteFiles);
        }
    }

    bool ServerSettings::isIncompleteDirectoryEnabled() const
    {
        return mIncompleteDirectoryEnabled;
    }

    void ServerSettings::setIncompleteDirectoryEnabled(bool enabled)
    {
        mIncompleteDirectoryEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(incompleteDirectoryEnabledKey, mIncompleteDirectoryEnabled);
        }
    }

    const QString& ServerSettings::incompleteDirectory() const
    {
        return mIncompleteDirectory;
    }

    void ServerSettings::setIncompleteDirectory(const QString& directory)
    {
        if (directory != mIncompleteDirectory) {
            mIncompleteDirectory = directory;
            if (mSaveOnSet) {
                mRpc->setSessionProperty(incompleteDirectoryKey, mIncompleteDirectory);
            }
        }
    }

    bool ServerSettings::isRatioLimited() const
    {
        return mRatioLimited;
    }

    void ServerSettings::setRatioLimited(bool limited)
    {
        mRatioLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(ratioLimitedKey, mRatioLimited);
        }
    }

    float ServerSettings::ratioLimit() const
    {
        return mRatioLimit;
    }

    void ServerSettings::setRatioLimit(float limit)
    {
        mRatioLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(ratioLimitKey, mRatioLimit);
        }
    }

    bool ServerSettings::isIdleSeedingLimited() const
    {
        return mIdleSeedingLimited;
    }

    void ServerSettings::setIdleSeedingLimited(bool limited)
    {
        mIdleSeedingLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleSeedingLimitedKey, mIdleSeedingLimited);
        }
    }

    int ServerSettings::idleSeedingLimit() const
    {
        return mIdleSeedingLimit;
    }

    void ServerSettings::setIdleSeedingLimit(int limit)
    {
        mIdleSeedingLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleSeedingLimitKey, mIdleSeedingLimit);
        }
    }

    bool ServerSettings::isDownloadQueueEnabled() const
    {
        return mDownloadQueueEnabled;
    }

    void ServerSettings::setDownloadQueueEnabled(bool enabled)
    {
        mDownloadQueueEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadQueueEnabledKey, mDownloadQueueEnabled);
        }
    }

    int ServerSettings::downloadQueueSize() const
    {
        return mDownloadQueueSize;
    }

    void ServerSettings::setDownloadQueueSize(int size)
    {
        mDownloadQueueSize = size;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadQueueSizeKey, mDownloadQueueSize);
        }
    }

    bool ServerSettings::isSeedQueueEnabled() const
    {
        return mSeedQueueEnabled;
    }

    void ServerSettings::setSeedQueueEnabled(bool enabled)
    {
        mSeedQueueEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(seedQueueEnabledKey, mSeedQueueEnabled);
        }
    }

    int ServerSettings::seedQueueSize() const
    {
        return mSeedQueueSize;
    }

    void ServerSettings::setSeedQueueSize(int size)
    {
        mSeedQueueSize = size;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(seedQueueSizeKey, mSeedQueueSize);
        }
    }

    bool ServerSettings::isIdleQueueLimited() const
    {
        return mIdleQueueLimited;
    }

    void ServerSettings::setIdleQueueLimited(bool limited)
    {
        mIdleQueueLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleQueueLimitedKey, mIdleQueueLimited);
        }
    }

    int ServerSettings::idleQueueLimit() const
    {
        return mIdleQueueLimit;
    }

    void ServerSettings::setIdleQueueLimit(int limit)
    {
        mIdleQueueLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleQueueLimitKey, mIdleQueueLimit);
        }
    }

    bool ServerSettings::isDownloadSpeedLimited() const
    {
        return mDownloadSpeedLimited;
    }

    void ServerSettings::setDownloadSpeedLimited(bool limited)
    {
        mDownloadSpeedLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadSpeedLimitedKey, mDownloadSpeedLimited);
        }
    }

    int ServerSettings::downloadSpeedLimit() const
    {
        return mDownloadSpeedLimit;
    }

    void ServerSettings::setDownloadSpeedLimit(int limit)
    {
        mDownloadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadSpeedLimitKey, mDownloadSpeedLimit);
        }
    }

    bool ServerSettings::isUploadSpeedLimited() const
    {
        return mUploadSpeedLimited;
    }

    void ServerSettings::setUploadSpeedLimited(bool limited)
    {
        mUploadSpeedLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(uploadSpeedLimitedKey, mUploadSpeedLimited);
        }
    }

    int ServerSettings::uploadSpeedLimit() const
    {
        return mUploadSpeedLimit;
    }

    void ServerSettings::setUploadSpeedLimit(int limit)
    {
        mUploadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(uploadSpeedLimitKey, mUploadSpeedLimit);
        }
    }

    bool ServerSettings::isAlternativeSpeedLimitsEnabled() const
    {
        return mAlternativeSpeedLimitsEnabled;
    }

    void ServerSettings::setAlternativeSpeedLimitsEnabled(bool enabled)
    {
        mAlternativeSpeedLimitsEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsEnabledKey, mAlternativeSpeedLimitsEnabled);
        }
    }

    int ServerSettings::alternativeDownloadSpeedLimit() const
    {
        return mAlternativeDownloadSpeedLimit;
    }

    void ServerSettings::setAlternativeDownloadSpeedLimit(int limit)
    {
        mAlternativeDownloadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeDownloadSpeedLimitKey, mAlternativeDownloadSpeedLimit);
        }
    }

    int ServerSettings::alternativeUploadSpeedLimit() const
    {
        return mAlternativeUploadSpeedLimit;
    }

    void ServerSettings::setAlternativeUploadSpeedLimit(int limit)
    {
        mAlternativeUploadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeUploadSpeedLimitKey, mAlternativeUploadSpeedLimit);
        }
    }

    bool ServerSettings::isAlternativeSpeedLimitsScheduled() const
    {
        return mAlternativeSpeedLimitsScheduled;
    }

    void ServerSettings::setAlternativeSpeedLimitsScheduled(bool scheduled)
    {
        mAlternativeSpeedLimitsScheduled = scheduled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsScheduledKey, mAlternativeSpeedLimitsScheduled);
        }
    }

    const QTime& ServerSettings::alternativeSpeedLimitsBeginTime() const
    {
        return mAlternativeSpeedLimitsBeginTime;
    }

    void ServerSettings::setAlternativeSpeedLimitsBeginTime(const QTime& time)
    {
        mAlternativeSpeedLimitsBeginTime = time;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsBeginTimeKey, mAlternativeSpeedLimitsBeginTime.msecsSinceStartOfDay() / 60000);
        }
    }

    const QTime& ServerSettings::alternativeSpeedLimitsEndTime() const
    {
        return mAlternativeSpeedLimitsEndTime;
    }

    void ServerSettings::setAlternativeSpeedLimitsEndTime(const QTime& time)
    {
        mAlternativeSpeedLimitsEndTime = time;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsEndTimeKey, mAlternativeSpeedLimitsEndTime.msecsSinceStartOfDay() / 60000);
        }
    }

    ServerSettings::AlternativeSpeedLimitsDays ServerSettings::alternativeSpeedLimitsDays() const
    {
        return mAlternativeSpeedLimitsDays;
    }

    void ServerSettings::setAlternativeSpeedLimitsDays(ServerSettings::AlternativeSpeedLimitsDays days)
    {
        if (days != mAlternativeSpeedLimitsDays) {
            mAlternativeSpeedLimitsDays = days;
            if (mSaveOnSet) {
                mRpc->setSessionProperty(alternativeSpeedLimitsDaysKey, mAlternativeSpeedLimitsDays);
            }
        }
    }

    int ServerSettings::peerPort() const
    {
        return mPeerPort;
    }

    void ServerSettings::setPeerPort(int port)
    {
        mPeerPort = port;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(peerPortKey, mPeerPort);
        }
    }

    bool ServerSettings::isRandomPortEnabled() const
    {
        return mRandomPortEnabled;
    }

    void ServerSettings::setRandomPortEnabled(bool enabled)
    {
        mRandomPortEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(randomPortEnabledKey, mRandomPortEnabled);
        }
    }

    bool ServerSettings::isPortForwardingEnabled() const
    {
        return mPortForwardingEnabled;
    }

    void ServerSettings::setPortForwardingEnabled(bool enabled)
    {
        mPortForwardingEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(portForwardingEnabledKey, mPortForwardingEnabled);
        }
    }

    ServerSettings::EncryptionMode ServerSettings::encryptionMode() const
    {
        return mEncryptionMode;
    }

    void ServerSettings::setEncryptionMode(ServerSettings::EncryptionMode mode)
    {
        mEncryptionMode = mode;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(encryptionModeKey, encryptionModeString(mode));
        }
    }

    bool ServerSettings::isUtpEnabled() const
    {
        return mUtpEnabled;
    }

    void ServerSettings::setUtpEnabled(bool enabled)
    {
        mUtpEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(utpEnabledKey, mUtpEnabled);
        }
    }

    bool ServerSettings::isPexEnabled() const
    {
        return mPexEnabled;
    }

    void ServerSettings::setPexEnabled(bool enabled)
    {
        mPexEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(pexEnabledKey, mPexEnabled);
        }
    }

    bool ServerSettings::isDhtEnabled() const
    {
        return mDhtEnabled;
    }

    void ServerSettings::setDhtEnabled(bool enabled)
    {
        mDhtEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(dhtEnabledKey, mDhtEnabled);
        }
    }

    bool ServerSettings::isLpdEnabled() const
    {
        return mLpdEnabled;
    }

    void ServerSettings::setLpdEnabled(bool enabled)
    {
        mLpdEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(lpdEnabledKey, mLpdEnabled);
        }
    }

    int ServerSettings::maximumPeersPerTorrent() const
    {
        return mMaximumPeersPerTorrent;
    }

    void ServerSettings::setMaximumPeersPerTorrent(int peers)
    {
        mMaximumPeersPerTorrent = peers;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(maximumPeersPerTorrentKey, mMaximumPeersPerTorrent);
        }
    }

    int ServerSettings::maximumPeersGlobally() const
    {
        return mMaximumPeersGlobally;
    }

    bool ServerSettings::saveOnSet() const
    {
        return mSaveOnSet;
    }

    void ServerSettings::setSaveOnSet(bool save)
    {
        mSaveOnSet = save;
    }

    void ServerSettings::setMaximumPeersGlobally(int peers)
    {
        mMaximumPeersGlobally = peers;
        if (mSaveOnSet) {
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
        mRpcVersion = serverSettings.value(QLatin1String("rpc-version")).toInt();
        mMinimumRpcVersion = serverSettings.value(QLatin1String("rpc-version-minimum")).toInt();

        mUsingDecimalUnits = (serverSettings
                                  .value(QLatin1String("units"))
                                  .toMap()
                                  .value(QLatin1String("speed-bytes"))
                                  .toInt() == 1000);

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
