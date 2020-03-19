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

#include "serversettings.h"

#include <QJsonObject>

#include "rpc.h"
#include "stdutils.h"

namespace libtremotesf
{
    namespace
    {
        const auto downloadDirectoryKey(QJsonKeyStringInit("download-dir"));
        const auto trashTorrentFilesKey(QJsonKeyStringInit("trash-original-torrent-files"));
        const auto startAddedTorrentsKey(QJsonKeyStringInit("start-added-torrents"));
        const auto renameIncompleteFilesKey(QJsonKeyStringInit("rename-partial-files"));
        const auto incompleteDirectoryEnabledKey(QJsonKeyStringInit("incomplete-dir-enabled"));
        const auto incompleteDirectoryKey(QJsonKeyStringInit("incomplete-dir"));

        const auto ratioLimitedKey(QJsonKeyStringInit("seedRatioLimited"));
        const auto ratioLimitKey(QJsonKeyStringInit("seedRatioLimit"));
        const auto idleSeedingLimitedKey(QJsonKeyStringInit("idle-seeding-limit-enabled"));
        const auto idleSeedingLimitKey(QJsonKeyStringInit("idle-seeding-limit"));

        const auto downloadQueueEnabledKey(QJsonKeyStringInit("download-queue-enabled"));
        const auto downloadQueueSizeKey(QJsonKeyStringInit("download-queue-size"));
        const auto seedQueueEnabledKey(QJsonKeyStringInit("seed-queue-enabled"));
        const auto seedQueueSizeKey(QJsonKeyStringInit("seed-queue-size"));
        const auto idleQueueLimitedKey(QJsonKeyStringInit("queue-stalled-enabled"));
        const auto idleQueueLimitKey(QJsonKeyStringInit("queue-stalled-minutes"));

        const auto downloadSpeedLimitedKey(QJsonKeyStringInit("speed-limit-down-enabled"));
        const auto downloadSpeedLimitKey(QJsonKeyStringInit("speed-limit-down"));
        const auto uploadSpeedLimitedKey(QJsonKeyStringInit("speed-limit-up-enabled"));
        const auto uploadSpeedLimitKey(QJsonKeyStringInit("speed-limit-up"));
        const auto alternativeSpeedLimitsEnabledKey(QJsonKeyStringInit("alt-speed-enabled"));
        const auto alternativeDownloadSpeedLimitKey(QJsonKeyStringInit("alt-speed-down"));
        const auto alternativeUploadSpeedLimitKey(QJsonKeyStringInit("alt-speed-up"));
        const auto alternativeSpeedLimitsScheduledKey(QJsonKeyStringInit("alt-speed-time-enabled"));
        const auto alternativeSpeedLimitsBeginTimeKey(QJsonKeyStringInit("alt-speed-time-begin"));
        const auto alternativeSpeedLimitsEndTimeKey(QJsonKeyStringInit("alt-speed-time-end"));
        const auto alternativeSpeedLimitsDaysKey(QJsonKeyStringInit("alt-speed-time-day"));

        inline ServerSettingsData::AlternativeSpeedLimitsDays daysFromInt(int days)
        {
            switch (days) {
            case ServerSettingsData::Sunday:
            case ServerSettingsData::Monday:
            case ServerSettingsData::Tuesday:
            case ServerSettingsData::Wednesday:
            case ServerSettingsData::Thursday:
            case ServerSettingsData::Friday:
            case ServerSettingsData::Saturday:
            case ServerSettingsData::Weekdays:
            case ServerSettingsData::Weekends:
            case ServerSettingsData::All:
                return static_cast<ServerSettingsData::AlternativeSpeedLimitsDays>(days);
            }
            return ServerSettingsData::All;
        }

        const auto peerPortKey(QJsonKeyStringInit("peer-port"));
        const auto randomPortEnabledKey(QJsonKeyStringInit("peer-port-random-on-start"));
        const auto portForwardingEnabledKey(QJsonKeyStringInit("port-forwarding-enabled"));

        const auto encryptionModeKey(QJsonKeyStringInit("encryption"));
        const QLatin1String encryptionModeAllowed("tolerated");
        const QLatin1String encryptionModePreferred("preferred");
        const QLatin1String encryptionModeRequired("required");

        inline QString encryptionModeString(ServerSettingsData::EncryptionMode mode)
        {
            switch (mode) {
            case ServerSettingsData::AllowedEncryption:
                return encryptionModeAllowed;
            case ServerSettingsData::PreferredEncryption:
                return encryptionModePreferred;
            case ServerSettingsData::RequiredEncryption:
                return encryptionModeRequired;
            }
            return QString();
        }

        const auto utpEnabledKey(QJsonKeyStringInit("utp-enabled"));
        const auto pexEnabledKey(QJsonKeyStringInit("pex-enabled"));
        const auto dhtEnabledKey(QJsonKeyStringInit("dht-enabled"));
        const auto lpdEnabledKey(QJsonKeyStringInit("lpd-enabled"));
        const auto maximumPeersPerTorrentKey(QJsonKeyStringInit("peer-limit-per-torrent"));
        const auto maximumPeersGloballyKey(QJsonKeyStringInit("peer-limit-global"));
    }

    bool ServerSettingsData::canRenameFiles() const
    {
        return (rpcVersion >= 15);
    }

    bool ServerSettingsData::canShowFreeSpaceForPath() const
    {
        return (rpcVersion >= 15);
    }

    ServerSettings::ServerSettings(Rpc* rpc, QObject* parent)
        : QObject(parent),
          mRpc(rpc),
          mSaveOnSet(true)
    {

    }

    int ServerSettings::rpcVersion() const
    {
        return mData.rpcVersion;
    }

    int ServerSettings::minimumRpcVersion() const
    {
        return mData.minimumRpcVersion;
    }

    bool ServerSettings::canRenameFiles() const
    {
        return mData.canRenameFiles();
    }

    bool ServerSettings::canShowFreeSpaceForPath() const
    {
        return mData.canShowFreeSpaceForPath();
    }

    const QString& ServerSettings::downloadDirectory() const
    {
        return mData.downloadDirectory;
    }

    void ServerSettings::setDownloadDirectory(const QString& directory)
    {
        if (directory != mData.downloadDirectory) {
            mData.downloadDirectory = directory;
            if (mSaveOnSet) {
                mRpc->setSessionProperty(downloadDirectoryKey, mData.downloadDirectory);
            }
        }
    }

    bool ServerSettings::startAddedTorrents() const
    {
        return mData.startAddedTorrents;
    }

    void ServerSettings::setStartAddedTorrents(bool start)
    {
        mData.startAddedTorrents = start;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(startAddedTorrentsKey, mData.startAddedTorrents);
        }
    }

    bool ServerSettings::trashTorrentFiles() const
    {
        return mData.trashTorrentFiles;
    }

    void ServerSettings::setTrashTorrentFiles(bool trash)
    {
        mData.trashTorrentFiles = trash;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(trashTorrentFilesKey, mData.trashTorrentFiles);
        }
    }

    bool ServerSettings::renameIncompleteFiles() const
    {
        return mData.renameIncompleteFiles;
    }

    void ServerSettings::setRenameIncompleteFiles(bool rename)
    {
        mData.renameIncompleteFiles = rename;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(renameIncompleteFilesKey, mData.renameIncompleteFiles);
        }
    }

    bool ServerSettings::isIncompleteDirectoryEnabled() const
    {
        return mData.incompleteDirectoryEnabled;
    }

    void ServerSettings::setIncompleteDirectoryEnabled(bool enabled)
    {
        mData.incompleteDirectoryEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(incompleteDirectoryEnabledKey, mData.incompleteDirectoryEnabled);
        }
    }

    const QString& ServerSettings::incompleteDirectory() const
    {
        return mData.incompleteDirectory;
    }

    void ServerSettings::setIncompleteDirectory(const QString& directory)
    {
        if (directory != mData.incompleteDirectory) {
            mData.incompleteDirectory = directory;
            if (mSaveOnSet) {
                mRpc->setSessionProperty(incompleteDirectoryKey, mData.incompleteDirectory);
            }
        }
    }

    bool ServerSettings::isRatioLimited() const
    {
        return mData.ratioLimited;
    }

    void ServerSettings::setRatioLimited(bool limited)
    {
        mData.ratioLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(ratioLimitedKey, mData.ratioLimited);
        }
    }

    double ServerSettings::ratioLimit() const
    {
        return mData.ratioLimit;
    }

    void ServerSettings::setRatioLimit(double limit)
    {
        mData.ratioLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(ratioLimitKey, mData.ratioLimit);
        }
    }

    bool ServerSettings::isIdleSeedingLimited() const
    {
        return mData.idleSeedingLimited;
    }

    void ServerSettings::setIdleSeedingLimited(bool limited)
    {
        mData.idleSeedingLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleSeedingLimitedKey, mData.idleSeedingLimited);
        }
    }

    int ServerSettings::idleSeedingLimit() const
    {
        return mData.idleSeedingLimit;
    }

    void ServerSettings::setIdleSeedingLimit(int limit)
    {
        mData.idleSeedingLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleSeedingLimitKey, mData.idleSeedingLimit);
        }
    }

    bool ServerSettings::isDownloadQueueEnabled() const
    {
        return mData.downloadQueueEnabled;
    }

    void ServerSettings::setDownloadQueueEnabled(bool enabled)
    {
        mData.downloadQueueEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadQueueEnabledKey, mData.downloadQueueEnabled);
        }
    }

    int ServerSettings::downloadQueueSize() const
    {
        return mData.downloadQueueSize;
    }

    void ServerSettings::setDownloadQueueSize(int size)
    {
        mData.downloadQueueSize = size;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadQueueSizeKey, mData.downloadQueueSize);
        }
    }

    bool ServerSettings::isSeedQueueEnabled() const
    {
        return mData.seedQueueEnabled;
    }

    void ServerSettings::setSeedQueueEnabled(bool enabled)
    {
        mData.seedQueueEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(seedQueueEnabledKey, mData.seedQueueEnabled);
        }
    }

    int ServerSettings::seedQueueSize() const
    {
        return mData.seedQueueSize;
    }

    void ServerSettings::setSeedQueueSize(int size)
    {
        mData.seedQueueSize = size;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(seedQueueSizeKey, mData.seedQueueSize);
        }
    }

    bool ServerSettings::isIdleQueueLimited() const
    {
        return mData.idleQueueLimited;
    }

    void ServerSettings::setIdleQueueLimited(bool limited)
    {
        mData.idleQueueLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleQueueLimitedKey, mData.idleQueueLimited);
        }
    }

    int ServerSettings::idleQueueLimit() const
    {
        return mData.idleQueueLimit;
    }

    void ServerSettings::setIdleQueueLimit(int limit)
    {
        mData.idleQueueLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(idleQueueLimitKey, mData.idleQueueLimit);
        }
    }

    bool ServerSettings::isDownloadSpeedLimited() const
    {
        return mData.downloadSpeedLimited;
    }

    void ServerSettings::setDownloadSpeedLimited(bool limited)
    {
        mData.downloadSpeedLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadSpeedLimitedKey, mData.downloadSpeedLimited);
        }
    }

    int ServerSettings::downloadSpeedLimit() const
    {
        return mData.downloadSpeedLimit;
    }

    void ServerSettings::setDownloadSpeedLimit(int limit)
    {
        mData.downloadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(downloadSpeedLimitKey, mData.downloadSpeedLimit);
        }
    }

    bool ServerSettings::isUploadSpeedLimited() const
    {
        return mData.uploadSpeedLimited;
    }

    void ServerSettings::setUploadSpeedLimited(bool limited)
    {
        mData.uploadSpeedLimited = limited;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(uploadSpeedLimitedKey, mData.uploadSpeedLimited);
        }
    }

    int ServerSettings::uploadSpeedLimit() const
    {
        return mData.uploadSpeedLimit;
    }

    void ServerSettings::setUploadSpeedLimit(int limit)
    {
        mData.uploadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(uploadSpeedLimitKey, mData.uploadSpeedLimit);
        }
    }

    bool ServerSettings::isAlternativeSpeedLimitsEnabled() const
    {
        return mData.alternativeSpeedLimitsEnabled;
    }

    void ServerSettings::setAlternativeSpeedLimitsEnabled(bool enabled)
    {
        mData.alternativeSpeedLimitsEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsEnabledKey, mData.alternativeSpeedLimitsEnabled);
        }
    }

    int ServerSettings::alternativeDownloadSpeedLimit() const
    {
        return mData.alternativeDownloadSpeedLimit;
    }

    void ServerSettings::setAlternativeDownloadSpeedLimit(int limit)
    {
        mData.alternativeDownloadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeDownloadSpeedLimitKey, mData.alternativeDownloadSpeedLimit);
        }
    }

    int ServerSettings::alternativeUploadSpeedLimit() const
    {
        return mData.alternativeUploadSpeedLimit;
    }

    void ServerSettings::setAlternativeUploadSpeedLimit(int limit)
    {
        mData.alternativeUploadSpeedLimit = limit;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeUploadSpeedLimitKey, mData.alternativeUploadSpeedLimit);
        }
    }

    bool ServerSettings::isAlternativeSpeedLimitsScheduled() const
    {
        return mData.alternativeSpeedLimitsScheduled;
    }

    void ServerSettings::setAlternativeSpeedLimitsScheduled(bool scheduled)
    {
        mData.alternativeSpeedLimitsScheduled = scheduled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsScheduledKey, mData.alternativeSpeedLimitsScheduled);
        }
    }

    QTime ServerSettings::alternativeSpeedLimitsBeginTime() const
    {
        return mData.alternativeSpeedLimitsBeginTime;
    }

    void ServerSettings::setAlternativeSpeedLimitsBeginTime(QTime time)
    {
        mData.alternativeSpeedLimitsBeginTime = time;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsBeginTimeKey, mData.alternativeSpeedLimitsBeginTime.msecsSinceStartOfDay() / 60000);
        }
    }

    QTime ServerSettings::alternativeSpeedLimitsEndTime() const
    {
        return mData.alternativeSpeedLimitsEndTime;
    }

    void ServerSettings::setAlternativeSpeedLimitsEndTime(QTime time)
    {
        mData.alternativeSpeedLimitsEndTime = time;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(alternativeSpeedLimitsEndTimeKey, mData.alternativeSpeedLimitsEndTime.msecsSinceStartOfDay() / 60000);
        }
    }

    ServerSettingsData::AlternativeSpeedLimitsDays ServerSettings::alternativeSpeedLimitsDays() const
    {
        return mData.alternativeSpeedLimitsDays;
    }

    void ServerSettings::setAlternativeSpeedLimitsDays(ServerSettingsData::AlternativeSpeedLimitsDays days)
    {
        if (days != mData.alternativeSpeedLimitsDays) {
            mData.alternativeSpeedLimitsDays = days;
            if (mSaveOnSet) {
                mRpc->setSessionProperty(alternativeSpeedLimitsDaysKey, mData.alternativeSpeedLimitsDays);
            }
        }
    }

    int ServerSettings::peerPort() const
    {
        return mData.peerPort;
    }

    void ServerSettings::setPeerPort(int port)
    {
        mData.peerPort = port;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(peerPortKey, mData.peerPort);
        }
    }

    bool ServerSettings::isRandomPortEnabled() const
    {
        return mData.randomPortEnabled;
    }

    void ServerSettings::setRandomPortEnabled(bool enabled)
    {
        mData.randomPortEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(randomPortEnabledKey, mData.randomPortEnabled);
        }
    }

    bool ServerSettings::isPortForwardingEnabled() const
    {
        return mData.portForwardingEnabled;
    }

    void ServerSettings::setPortForwardingEnabled(bool enabled)
    {
        mData.portForwardingEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(portForwardingEnabledKey, mData.portForwardingEnabled);
        }
    }

    ServerSettingsData::EncryptionMode ServerSettings::encryptionMode() const
    {
        return mData.encryptionMode;
    }

    void ServerSettings::setEncryptionMode(ServerSettingsData::EncryptionMode mode)
    {
        mData.encryptionMode = mode;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(encryptionModeKey, encryptionModeString(mode));
        }
    }

    bool ServerSettings::isUtpEnabled() const
    {
        return mData.utpEnabled;
    }

    void ServerSettings::setUtpEnabled(bool enabled)
    {
        mData.utpEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(utpEnabledKey, mData.utpEnabled);
        }
    }

    bool ServerSettings::isPexEnabled() const
    {
        return mData.pexEnabled;
    }

    void ServerSettings::setPexEnabled(bool enabled)
    {
        mData.pexEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(pexEnabledKey, mData.pexEnabled);
        }
    }

    bool ServerSettings::isDhtEnabled() const
    {
        return mData.dhtEnabled;
    }

    void ServerSettings::setDhtEnabled(bool enabled)
    {
        mData.dhtEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(dhtEnabledKey, mData.dhtEnabled);
        }
    }

    bool ServerSettings::isLpdEnabled() const
    {
        return mData.lpdEnabled;
    }

    void ServerSettings::setLpdEnabled(bool enabled)
    {
        mData.lpdEnabled = enabled;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(lpdEnabledKey, mData.lpdEnabled);
        }
    }

    int ServerSettings::maximumPeersPerTorrent() const
    {
        return mData.maximumPeersPerTorrent;
    }

    void ServerSettings::setMaximumPeersPerTorrent(int peers)
    {
        mData.maximumPeersPerTorrent = peers;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(maximumPeersPerTorrentKey, mData.maximumPeersPerTorrent);
        }
    }

    int ServerSettings::maximumPeersGlobally() const
    {
        return mData.maximumPeersGlobally;
    }

    void ServerSettings::setMaximumPeersGlobally(int peers)
    {
        mData.maximumPeersGlobally = peers;
        if (mSaveOnSet) {
            mRpc->setSessionProperty(maximumPeersGloballyKey, mData.maximumPeersGlobally);
        }
    }

    bool ServerSettings::saveOnSet() const
    {
        return mSaveOnSet;
    }

    void ServerSettings::setSaveOnSet(bool save)
    {
        mSaveOnSet = save;
    }

    int ServerSettings::toKibiBytes(int kiloBytesOrKibiBytes) const
    {
        if (mUsingDecimalUnits) {
            return kiloBytesOrKibiBytes * 1000 / 1024;
        }
        return kiloBytesOrKibiBytes;
    }

    int ServerSettings::fromKibiBytes(int kibiBytes) const
    {
        if (mUsingDecimalUnits) {
            return kibiBytes * 1024 / 1000;
        }
        return kibiBytes;
    }

    void ServerSettings::update(const QJsonObject& serverSettings)
    {
        bool changed = false;

        mData.rpcVersion = serverSettings.value(QJsonKeyStringInit("rpc-version")).toInt();
        mData.minimumRpcVersion = serverSettings.value(QJsonKeyStringInit("rpc-version-minimum")).toInt();


        const bool usingDecimalUnits = (serverSettings
                                            .value(QJsonKeyStringInit("units"))
                                            .toObject()
                                            .value(QJsonKeyStringInit("speed-bytes"))
                                            .toInt() == 1000);
        setChanged(mUsingDecimalUnits, usingDecimalUnits, changed);

        setChanged(mData.downloadDirectory, serverSettings.value(downloadDirectoryKey).toString(), changed);
        setChanged(mData.trashTorrentFiles, serverSettings.value(trashTorrentFilesKey).toBool(), changed);
        setChanged(mData.startAddedTorrents, serverSettings.value(startAddedTorrentsKey).toBool(), changed);
        setChanged(mData.renameIncompleteFiles, serverSettings.value(renameIncompleteFilesKey).toBool(), changed);
        setChanged(mData.incompleteDirectoryEnabled, serverSettings.value(incompleteDirectoryEnabledKey).toBool(), changed);
        setChanged(mData.incompleteDirectory, serverSettings.value(incompleteDirectoryKey).toString(), changed);

        setChanged(mData.ratioLimited, serverSettings.value(ratioLimitedKey).toBool(), changed);
        setChanged(mData.ratioLimit, serverSettings.value(ratioLimitKey).toDouble(), changed);
        setChanged(mData.idleSeedingLimited, serverSettings.value(idleSeedingLimitedKey).toBool(), changed);
        setChanged(mData.idleSeedingLimit, serverSettings.value(idleSeedingLimitKey).toInt(), changed);

        setChanged(mData.downloadQueueEnabled, serverSettings.value(downloadQueueEnabledKey).toBool(), changed);
        setChanged(mData.downloadQueueSize, serverSettings.value(downloadQueueSizeKey).toInt(), changed);
        setChanged(mData.seedQueueEnabled, serverSettings.value(seedQueueEnabledKey).toBool(), changed);
        setChanged(mData.seedQueueSize, serverSettings.value(seedQueueSizeKey).toInt(), changed);
        setChanged(mData.idleQueueLimited, serverSettings.value(idleQueueLimitedKey).toBool(), changed);
        setChanged(mData.idleQueueLimit, serverSettings.value(idleQueueLimitKey).toInt(), changed);

        setChanged(mData.downloadSpeedLimited, serverSettings.value(downloadSpeedLimitedKey).toBool(), changed);
        setChanged(mData.downloadSpeedLimit, toKibiBytes(serverSettings.value(downloadSpeedLimitKey).toInt()), changed);
        setChanged(mData.uploadSpeedLimited, serverSettings.value(uploadSpeedLimitedKey).toBool(), changed);
        setChanged(mData.uploadSpeedLimit, toKibiBytes(serverSettings.value(uploadSpeedLimitKey).toInt()), changed);
        setChanged(mData.alternativeSpeedLimitsEnabled, serverSettings.value(alternativeSpeedLimitsEnabledKey).toBool(), changed);
        setChanged(mData.alternativeDownloadSpeedLimit, toKibiBytes(serverSettings.value(alternativeDownloadSpeedLimitKey).toInt()), changed);
        setChanged(mData.alternativeUploadSpeedLimit, toKibiBytes(serverSettings.value(alternativeUploadSpeedLimitKey).toInt()), changed);
        setChanged(mData.alternativeSpeedLimitsScheduled, serverSettings.value(alternativeSpeedLimitsScheduledKey).toBool(), changed);
        setChanged(mData.alternativeSpeedLimitsBeginTime, QTime::fromMSecsSinceStartOfDay(serverSettings.value(alternativeSpeedLimitsBeginTimeKey).toInt() * 60000), changed);
        setChanged(mData.alternativeSpeedLimitsEndTime, QTime::fromMSecsSinceStartOfDay(serverSettings.value(alternativeSpeedLimitsEndTimeKey).toInt() * 60000), changed);
        setChanged(mData.alternativeSpeedLimitsDays, daysFromInt(serverSettings.value(alternativeSpeedLimitsDaysKey).toInt()), changed);

        setChanged(mData.peerPort, serverSettings.value(peerPortKey).toInt(), changed);
        setChanged(mData.randomPortEnabled, serverSettings.value(randomPortEnabledKey).toBool(), changed);
        setChanged(mData.portForwardingEnabled, serverSettings.value(portForwardingEnabledKey).toBool(), changed);

        const QString encryption(serverSettings.value(encryptionModeKey).toString());
        if (encryption == encryptionModeAllowed) {
            setChanged(mData.encryptionMode, ServerSettingsData::AllowedEncryption, changed);
        } else if (encryption == encryptionModePreferred) {
            setChanged(mData.encryptionMode, ServerSettingsData::PreferredEncryption, changed);
        } else {
            setChanged(mData.encryptionMode, ServerSettingsData::RequiredEncryption, changed);
        }

        setChanged(mData.utpEnabled, serverSettings.value(utpEnabledKey).toBool(), changed);
        setChanged(mData.pexEnabled, serverSettings.value(pexEnabledKey).toBool(), changed);
        setChanged(mData.dhtEnabled, serverSettings.value(dhtEnabledKey).toBool(), changed);
        setChanged(mData.lpdEnabled, serverSettings.value(lpdEnabledKey).toBool(), changed);
        setChanged(mData.maximumPeersPerTorrent, serverSettings.value(maximumPeersPerTorrentKey).toInt(), changed);
        setChanged(mData.maximumPeersGlobally, serverSettings.value(maximumPeersGloballyKey).toInt(), changed);

        if (changed) {
            emit this->changed();
        }
    }

    void ServerSettings::save() const
    {
        mRpc->setSessionProperties({{downloadDirectoryKey, mData.downloadDirectory},
                                    {trashTorrentFilesKey, mData.trashTorrentFiles},
                                    {startAddedTorrentsKey, mData.startAddedTorrents},
                                    {renameIncompleteFilesKey, mData.renameIncompleteFiles},
                                    {incompleteDirectoryEnabledKey, mData.incompleteDirectoryEnabled},
                                    {incompleteDirectoryKey, mData.incompleteDirectory},

                                    {ratioLimitedKey, mData.ratioLimited},
                                    {ratioLimitKey, mData.ratioLimit},
                                    {idleSeedingLimitedKey, mData.idleSeedingLimit},
                                    {idleSeedingLimitKey, mData.idleSeedingLimit},

                                    {downloadQueueEnabledKey, mData.downloadQueueEnabled},
                                    {downloadQueueSizeKey, mData.downloadQueueSize},
                                    {seedQueueEnabledKey, mData.seedQueueEnabled},
                                    {seedQueueSizeKey, mData.seedQueueSize},
                                    {idleQueueLimitedKey, mData.idleQueueLimited},
                                    {idleQueueLimitKey, mData.idleQueueLimit},

                                    {downloadSpeedLimitedKey, mData.downloadSpeedLimited},
                                    {downloadSpeedLimitKey, mData.downloadSpeedLimit},
                                    {uploadSpeedLimitedKey, mData.uploadSpeedLimited},
                                    {uploadSpeedLimitKey, mData.uploadSpeedLimit},
                                    {alternativeSpeedLimitsEnabledKey, mData.alternativeSpeedLimitsEnabled},
                                    {alternativeDownloadSpeedLimitKey, mData.alternativeDownloadSpeedLimit},
                                    {alternativeUploadSpeedLimitKey, mData.alternativeUploadSpeedLimit},
                                    {alternativeSpeedLimitsScheduledKey, mData.alternativeSpeedLimitsScheduled},
                                    {alternativeSpeedLimitsBeginTimeKey, mData.alternativeSpeedLimitsBeginTime.msecsSinceStartOfDay() / 60000},
                                    {alternativeSpeedLimitsEndTimeKey, mData.alternativeSpeedLimitsEndTime.msecsSinceStartOfDay() / 60000},
                                    {alternativeSpeedLimitsDaysKey, mData.alternativeSpeedLimitsDays},

                                    {peerPortKey, mData.peerPort},
                                    {randomPortEnabledKey, mData.randomPortEnabled},
                                    {portForwardingEnabledKey, mData.portForwardingEnabled},
                                    {encryptionModeKey, encryptionModeString(mData.encryptionMode)},
                                    {utpEnabledKey, mData.utpEnabled},
                                    {pexEnabledKey, mData.pexEnabled},
                                    {dhtEnabledKey, mData.dhtEnabled},
                                    {lpdEnabledKey, mData.lpdEnabled},
                                    {maximumPeersPerTorrentKey, mData.maximumPeersPerTorrent},
                                    {maximumPeersGloballyKey, mData.maximumPeersGlobally}});
    }

    const ServerSettingsData& ServerSettings::data() const
    {
        return mData;
    }
}
