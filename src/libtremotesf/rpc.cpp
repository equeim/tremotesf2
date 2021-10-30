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

#include "rpc.h"

#include <QAuthenticator>
#include <QCoreApplication>
#include <QDebug>
#include <QFutureWatcher>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QTimer>
#include <QSslCertificate>
#include <QSslKey>
#include <QStandardPaths>
#include <QtConcurrentRun>

#ifdef TREMOTESF_SAILFISHOS
#include <QQmlEngine>
#endif

#include "itemlistupdater.h"
#include "serversettings.h"
#include "serverstats.h"
#include "stdutils.h"
#include "torrent.h"

namespace libtremotesf
{
    namespace
    {
        // Transmission 2.40+
        const int minimumRpcVersion = 14;

        const int maxRetryAttempts = 3;

        const QByteArray sessionIdHeader(QByteArrayLiteral("X-Transmission-Session-Id"));
        const auto torrentsKey(QJsonKeyStringInit("torrents"));
        const QLatin1String torrentDuplicateKey("torrent-duplicate");

#ifdef Q_OS_WIN
        constexpr auto sessionIdFileLocation = QStandardPaths::GenericDataLocation;
        const QLatin1String sessionIdFilePrefix("Transmission/tr_session_id_");
#else
        constexpr auto sessionIdFileLocation = QStandardPaths::TempLocation;
        const QLatin1String sessionIdFilePrefix("tr_session_id_");
#endif

        inline QByteArray makeRequestData(const QString& method, const QVariantMap& arguments)
        {
            return QJsonDocument::fromVariant(QVariantMap{{QStringLiteral("method"), method},
                                                          {QStringLiteral("arguments"), arguments}})
                .toJson(QJsonDocument::Compact);
        }

        inline QJsonObject getReplyArguments(const QJsonObject& parseResult)
        {
            return parseResult.value(QJsonKeyStringInit("arguments")).toObject();
        }

        inline bool isResultSuccessful(const QJsonObject& parseResult)
        {
            return (parseResult.value(QJsonKeyStringInit("result")).toString() == QLatin1String("success"));
        }

        bool isAddressLocal(const QString& address)
        {
            if (address == QHostInfo::localHostName()) {
                return true;
            }

            const QHostAddress ipAddress(address);

            if (ipAddress.isNull()) {
                return address == QHostInfo::fromName(QHostAddress(QHostAddress::LocalHost).toString()).hostName() ||
                       address == QHostInfo::fromName(QHostAddress(QHostAddress::LocalHostIPv6).toString()).hostName();
            }

            return ipAddress.isLoopback() || QNetworkInterface::allAddresses().contains(ipAddress);
        }

        QString readFileAsBase64String(QFile& file)
        {
            if (!file.isOpen() && !file.open(QIODevice::ReadOnly)) {
                qWarning("Failed to open file");
                return QString();
            }

            static constexpr qint64 bufferSize = 1024 * 1024 - 1; // 1 MiB minus 1 byte (dividable by 3)
            QString string;
            string.reserve(static_cast<int>(((4 * file.size() / 3) + 3) & ~3));

            QByteArray buffer;
            buffer.resize(static_cast<int>(std::min(bufferSize, file.size())));

            qint64 offset = 0;

            while (true) {
                const qint64 n = file.read(buffer.data() + offset, buffer.size() - offset);
                if (n <= 0) {
                    if (offset > 0) {
                        buffer.resize(static_cast<int>(offset));
                        string.append(QLatin1String(buffer.toBase64()));
                    }
                    break;
                }
                offset += n;
                if (offset == buffer.size()) {
                    string.append(QLatin1String(buffer.toBase64()));
                    offset = 0;
                }
            }

            return string;
        }
    }

    Rpc::Rpc(QObject* parent)
        : QObject(parent),
          mNetwork(new QNetworkAccessManager(this)),
          mAuthenticationRequested(false),
          mUpdateDisabled(false),
          mUpdating(false),
          mAuthentication(false),
          mTimeout(0),
          mAutoReconnectEnabled(false),
          mLocal(false),
          mRpcVersionChecked(false),
          mServerSettingsUpdated(false),
          mTorrentsUpdated(false),
          mServerStatsUpdated(false),
          mUpdateTimer(new QTimer(this)),
          mAutoReconnectTimer(new QTimer(this)),
          mServerSettings(new ServerSettings(this, this)),
          mServerStats(new ServerStats(this))
    {
        QObject::connect(mNetwork, &QNetworkAccessManager::authenticationRequired, this, &Rpc::onAuthenticationRequired);

        mAutoReconnectTimer->setSingleShot(true);
        QObject::connect(mAutoReconnectTimer, &QTimer::timeout, this, [=] {
            qInfo("Auto reconnection");
            connect();
        });

        mUpdateTimer->setSingleShot(true);
        QObject::connect(mUpdateTimer, &QTimer::timeout, this, &Rpc::updateData);

        QObject::connect(mNetwork, &QNetworkAccessManager::sslErrors, this, [=](auto, const auto& errors) {
            for (const auto& error : errors) {
                if (!mExpectedSslErrors.contains(error)) {
                    qWarning() << error;
                }
            }
        });
    }

    ServerSettings* Rpc::serverSettings() const
    {
        return mServerSettings;
    }

    ServerStats* Rpc::serverStats() const
    {
        return mServerStats;
    }

    const std::vector<std::shared_ptr<Torrent>>& Rpc::torrents() const
    {
        return mTorrents;
    }

    Torrent* Rpc::torrentByHash(const QString& hash) const
    {
        for (const std::shared_ptr<Torrent>& torrent : mTorrents) {
            if (torrent->hashString() == hash) {
                return torrent.get();
            }
        }
        return nullptr;
    }

    Torrent* Rpc::torrentById(int id) const
    {
        const auto end(mTorrents.end());
        const auto found(std::find_if(mTorrents.begin(), mTorrents.end(), [id](const auto& torrent) { return torrent->id() == id; }));
        return (found == end) ? nullptr : found->get();
    }

    bool Rpc::isConnected() const
    {
        return (mStatus.connectionState == ConnectionState::Connected);
    }

    const Rpc::Status& Rpc::status() const
    {
        return mStatus;
    }

    Rpc::ConnectionState Rpc::connectionState() const
    {
        return mStatus.connectionState;
    }

    Rpc::Error Rpc::error() const
    {
        return mStatus.error;
    }

    const QString& Rpc::errorMessage() const
    {
        return mStatus.errorMessage;
    }

    bool Rpc::isLocal() const
    {
        return mLocal;
    }

    int Rpc::torrentsCount() const
    {
        return static_cast<int>(mTorrents.size());
    }

    bool Rpc::isUpdateDisabled() const
    {
        return mUpdateDisabled;
    }

    void Rpc::setUpdateDisabled(bool disabled)
    {
        if (disabled != mUpdateDisabled) {
            mUpdateDisabled = disabled;
            if (isConnected()) {
                if (disabled) {
                    mUpdateTimer->stop();
                } else {
                    updateData();
                }
            }
            if (disabled) {
                mAutoReconnectTimer->stop();
            }
            emit updateDisabledChanged();
        }
    }

    void Rpc::setServer(const Server& server)
    {
        mNetwork->clearAccessCache();
        disconnect();

        mServerUrl.setHost(server.address);
        mServerUrl.setPort(server.port);
        mServerUrl.setPath(server.apiPath);
        if (server.https) {
            mServerUrl.setScheme(QLatin1String("https"));
        } else {
            mServerUrl.setScheme(QLatin1String("http"));
        }

        switch (server.proxyType) {
        case Server::ProxyType::Default:
            mNetwork->setProxy(QNetworkProxy::applicationProxy());
            break;
        case Server::ProxyType::Http:
            mNetwork->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy,
                                             server.proxyHostname,
                                             static_cast<quint16>(server.proxyPort),
                                             server.proxyUser,
                                             server.proxyPassword));
            break;
        case Server::ProxyType::Socks5:
            mNetwork->setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy,
                                             server.proxyHostname,
                                             static_cast<quint16>(server.proxyPort),
                                             server.proxyUser,
                                             server.proxyPassword));
            break;
        }

        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        mExpectedSslErrors.clear();

        if (server.selfSignedCertificateEnabled) {
            const auto certificates(QSslCertificate::fromData(server.selfSignedCertificate, QSsl::Pem));
            mExpectedSslErrors.reserve(certificates.size() * 3);
            for (const auto& certificate : certificates) {
                mExpectedSslErrors.push_back(QSslError(QSslError::HostNameMismatch, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificate, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificateInChain, certificate));
            }
        }

        if (server.clientCertificateEnabled) {
            mSslConfiguration.setLocalCertificate(QSslCertificate(server.clientCertificate, QSsl::Pem));
            mSslConfiguration.setPrivateKey(QSslKey(server.clientCertificate, QSsl::Rsa));
        }

        mAuthentication = server.authentication;
        mUsername = server.username;
        mPassword = server.password;
        mTimeout = server.timeout * 1000; // msecs
        mUpdateTimer->setInterval(server.updateInterval * 1000); // msecs

        mAutoReconnectEnabled = server.autoReconnectEnabled;
        mAutoReconnectTimer->setInterval(server.autoReconnectInterval * 1000); // msecs
        mAutoReconnectTimer->stop();
    }

    void Rpc::resetServer()
    {
        disconnect();
        mServerUrl.clear();
        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        mExpectedSslErrors.clear();
        mAuthentication = false;
        mUsername.clear();
        mPassword.clear();
        mTimeout = 0;
        mLocal = false;
        mAutoReconnectEnabled = false;
        mAutoReconnectTimer->stop();
    }

    void Rpc::connect()
    {
        if (connectionState() == ConnectionState::Disconnected && !mServerUrl.isEmpty()) {
            setStatus(Status{ConnectionState::Connecting});
            getServerSettings();
        }
    }

    void Rpc::disconnect()
    {
        setStatus(Status{ConnectionState::Disconnected});
        mAutoReconnectTimer->stop();
    }

    void Rpc::addTorrentFile(const QString& filePath,
                             const QString& downloadDirectory,
                             const QVariantList& unwantedFiles,
                             const QVariantList& highPriorityFiles,
                             const QVariantList& lowPriorityFiles,
                             const QVariantMap& renamedFiles,
                             int bandwidthPriority,
                             bool start)
    {
        if (!isConnected()) {
            return;
        }
        if (auto file = std::make_shared<QFile>(filePath); file->open(QIODevice::ReadOnly)) {
            addTorrentFile(std::move(file),
                           downloadDirectory,
                           unwantedFiles,
                           highPriorityFiles,
                           lowPriorityFiles,
                           renamedFiles,
                           bandwidthPriority,
                           start);
        } else {
            qWarning().nospace() << "addTorrentFile: failed to open file, error = " << file->error() << ", error string = " << file->errorString();
            emit torrentAddError();
        }
    }

    void Rpc::addTorrentFile(std::shared_ptr<QFile> file,
                             const QString& downloadDirectory,
                             const QVariantList& unwantedFiles,
                             const QVariantList& highPriorityFiles,
                             const QVariantList& lowPriorityFiles,
                             const QVariantMap& renamedFiles,
                             int bandwidthPriority,
                             bool start)
    {
        if (!isConnected()) {
            return;
        }
        const auto future = QtConcurrent::run([=, file = std::move(file)]() {
            return makeRequestData(QLatin1String("torrent-add"),
                                   {{QLatin1String("metainfo"), readFileAsBase64String(*file)},
                                    {QLatin1String("download-dir"), downloadDirectory},
                                    {QLatin1String("files-unwanted"), unwantedFiles},
                                    {QLatin1String("priority-high"), highPriorityFiles},
                                    {QLatin1String("priority-low"), lowPriorityFiles},
                                    {QLatin1String("bandwidthPriority"), bandwidthPriority},
                                    {QLatin1String("paused"), !start}});
        });
        auto watcher = new QFutureWatcher<QByteArray>(this);
        QObject::connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [=] {
            if (isConnected()) {
                postRequest(QLatin1String("torrent-add"), watcher->result(), [=](const auto& parseResult, bool success) {
                    if (success) {
                        const auto arguments(getReplyArguments(parseResult));
                        if (arguments.contains(torrentDuplicateKey)) {
                            emit torrentAddDuplicate();
                        } else {
                            if (!renamedFiles.isEmpty()) {
                                const QJsonObject torrentJson(arguments.value(QLatin1String("torrent-added")).toObject());
                                if (!torrentJson.isEmpty()) {
                                    const int id = torrentJson.value(Torrent::idKey).toInt();
                                    for (auto i = renamedFiles.begin(), end = renamedFiles.end();
                                         i != end;
                                         ++i) {
                                        renameTorrentFile(id, i.key(), i.value().toString());
                                    }
                                }
                            }
                            updateData();
                        }
                    } else {
                        emit torrentAddError();
                    }
                });
                watcher->deleteLater();
            }
        });
        watcher->setFuture(future);
    }

    void Rpc::addTorrentLink(const QString& link,
                             const QString& downloadDirectory,
                             int bandwidthPriority,
                             bool start)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-add"),
                        {{QLatin1String("filename"), link},
                         {QLatin1String("download-dir"), downloadDirectory},
                         {QLatin1String("bandwidthPriority"), bandwidthPriority},
                         {QLatin1String("paused"), !start}},
                    [=](const auto& parseResult, bool success) {
                        if (success) {
                            if (getReplyArguments(parseResult).contains(torrentDuplicateKey)) {
                                emit torrentAddDuplicate();
                            } else {
                                updateData();
                            }
                        } else {
                            emit torrentAddError();
                        }
                    });
        }
    }

    void Rpc::startTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-start"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::startTorrentsNow(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-start-now"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::pauseTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-stop"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::removeTorrents(const QVariantList& ids, bool deleteFiles)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-remove"),
                        {{QLatin1String("ids"), ids},
                         {QLatin1String("delete-local-data"), deleteFiles}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::checkTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-verify"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::moveTorrentsToTop(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("queue-move-top"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::moveTorrentsUp(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("queue-move-up"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::moveTorrentsDown(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("queue-move-down"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::moveTorrentsToBottom(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("queue-move-bottom"),
                        {{QLatin1String("ids"), ids}},
                        [=](const auto&, bool success) {
                            if (success) {
                                updateData();
                            }
                        });
        }
    }

    void Rpc::reannounceTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-reannounce"),
                        {{QLatin1String("ids"), ids}});
        }
    }

    void Rpc::setSessionProperty(const QString& property, const QVariant& value)
    {
        setSessionProperties({{property, value}});
    }

    void Rpc::setSessionProperties(const QVariantMap& properties)
    {
        if (isConnected()) {
            postRequest(QLatin1String("session-set"), properties);
        }
    }

    void Rpc::setTorrentProperty(int id, const QString& property, const QVariant& value, bool updateIfSuccessful)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-set"),
                        {{QLatin1String("ids"), QVariantList{id}},
                         {property, value}},
                        [=](const auto&, bool success) {
                            if (success && updateIfSuccessful) {
                                updateData();
                            }
            });
        }
    }

    void Rpc::setTorrentsLocation(const QVariantList& ids, const QString& location, bool moveFiles)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-set-location"),
                        {{QLatin1String("ids"), ids},
                         {QLatin1String("location"), location},
                         {QLatin1String("move"), moveFiles}},
                        [=](const auto&, bool success) {
                if (success) {
                    updateData();
                }
            });
        }
    }

    void Rpc::getTorrentsFiles(const QVariantList& ids, bool scheduled)
    {
        postRequest(QLatin1String("torrent-get"), {{QLatin1String("fields"), QStringList{QLatin1String("id"), QLatin1String("files"), QLatin1String("fileStats")}},
                                                   {QLatin1String("ids"), ids}},
                    [=](const auto& parseResult, bool success) {
                        if (success) {
                            const QJsonArray torrents(getReplyArguments(parseResult).value(torrentsKey).toArray());
                            for (const auto& i : torrents) {
                                const QJsonObject torrentMap(i.toObject());
                                const int torrentId = torrentMap.value(Torrent::idKey).toInt();
                                Torrent* torrent = torrentById(torrentId);
                                if (torrent && torrent->isFilesEnabled()) {
                                    torrent->updateFiles(torrentMap);
                                }
                                if (scheduled) {
                                    for (const auto& torrent : mTorrents) {
                                        torrent->checkThatFilesUpdated();
                                    }
                                    checkIfTorrentsUpdated();
                                    startUpdateTimer();
                                }
                            }
                        }
                    });
    }

    void Rpc::getTorrentsPeers(const QVariantList& ids, bool scheduled)
    {
        postRequest(QLatin1String("torrent-get"), {{QLatin1String("fields"), QStringList{QLatin1String("id"), QLatin1String("peers")}},
                                                   {QLatin1String("ids"), ids}},
                    [=](const auto& parseResult, bool success) {
                        if (success) {
                            const QJsonArray torrents(getReplyArguments(parseResult).value(torrentsKey).toArray());
                            for (const auto& i : torrents) {
                                const QJsonObject torrentMap(i.toObject());
                                const int torrentId = torrentMap.value(Torrent::idKey).toInt();
                                Torrent* torrent = torrentById(torrentId);
                                if (torrent && torrent->isPeersEnabled()) {
                                    torrent->updatePeers(torrentMap);
                                }
                                if (scheduled) {
                                    for (const auto& torrent : mTorrents) {
                                        torrent->checkThatPeersUpdated();
                                    }
                                    checkIfTorrentsUpdated();
                                    startUpdateTimer();
                                }
                            }
                        }
                    });
    }

    void Rpc::renameTorrentFile(int torrentId, const QString& filePath, const QString& newName)
    {
        if (isConnected()) {
            postRequest(QLatin1String("torrent-rename-path"),
                        {{QLatin1String("ids"), QVariantList{torrentId}},
                         {QLatin1String("path"), filePath},
                         {QLatin1String("name"), newName}},
                        [=](const auto& parseResult, bool success) {
                            if (success) {
                                Torrent* torrent = torrentById(torrentId);
                                if (torrent) {
                                    const QJsonObject arguments(getReplyArguments(parseResult));
                                    const QString path(arguments.value(QLatin1String("path")).toString());
                                    const QString newName(arguments.value(QLatin1String("name")).toString());
                                    emit torrent->fileRenamed(path, newName);
                                    emit torrentFileRenamed(torrentId, path, newName);
                                    updateData();
                                }
                            }
                        });
        }
    }

    void Rpc::getDownloadDirFreeSpace()
    {
        if (isConnected()) {
            postRequest(QLatin1String("download-dir-free-space"),
                        QByteArrayLiteral(
                        "{"
                            "\"arguments\":{"
                                "\"fields\":["
                                    "\"download-dir-free-space\""
                                "]"
                            "},"
                            "\"method\":\"session-get\""
                        "}"),
                        [=](const auto& parseResult, bool success) {
                            if (success) {
                                emit gotDownloadDirFreeSpace(static_cast<long long>(getReplyArguments(parseResult).value(QJsonKeyStringInit("download-dir-free-space")).toDouble()));
                            }
                        });
        }
    }

    void Rpc::getFreeSpaceForPath(const QString& path)
    {
        if (isConnected()) {
            postRequest(QLatin1String("free-space"),
                        {{QLatin1String("path"), path}},
                        [=](const auto& parseResult, bool success) {
                            emit gotFreeSpaceForPath(path,
                                                     success,
                                                     success ? static_cast<long long>(getReplyArguments(parseResult).value(QJsonKeyStringInit("size-bytes")).toDouble()) : 0);
                        });
        }
    }

    void Rpc::updateData()
    {
        if (isConnected() && !mUpdating) {
            mServerSettingsUpdated = false;
            mTorrentsUpdated = false;
            mServerStatsUpdated = false;

            mUpdateTimer->stop();

            mUpdating = true;

            getServerSettings();
            getTorrents();
            getServerStats();
        }
    }

    void Rpc::shutdownServer()
    {
        if (isConnected()) {
            postRequest(QLatin1String("session-close"), QVariantMap{}, [=](const QJsonObject&, bool success) {
                if (success) {
                    qInfo("Successfully sent shutdown request, disconnecting");
                    disconnect();
                }
            });
        }
    }

    void Rpc::setStatus(Status status)
    {
        if (status == mStatus) {
            return;
        }

        const Status oldStatus = mStatus;

        const bool connectionStateChanged = status.connectionState != oldStatus.connectionState;
        if (connectionStateChanged && oldStatus.connectionState == ConnectionState::Connected) {
            emit aboutToDisconnect();
        }

        mStatus = status;

        std::vector<int> removedTorrentsIndices;
        if (connectionStateChanged) {
            resetStateOnConnectionStateChanged(removedTorrentsIndices);
        }

        emit statusChanged();

        if (connectionStateChanged) {
            emitSignalsOnConnectionStateChanged(oldStatus.connectionState, std::move(removedTorrentsIndices));
        }

        if (status.error != oldStatus.error || status.errorMessage != oldStatus.errorMessage) {
            emit errorChanged();
        }
    }

    void Rpc::resetStateOnConnectionStateChanged(std::vector<int>& removedTorrentsIndices)
    {
        switch (mStatus.connectionState) {
        case ConnectionState::Disconnected:
        {
            qInfo("Disconnected");

            mNetwork->clearAccessCache();

            const auto activeRequests(mActiveNetworkRequests);
            mActiveNetworkRequests.clear();
            mRetryingNetworkRequests.clear();
            for (QNetworkReply* reply : activeRequests) {
                reply->abort();
            }

            mUpdating = false;

            mAuthenticationRequested = false;
            mRpcVersionChecked = false;
            mServerSettingsUpdated = false;
            mTorrentsUpdated = false;
            mServerStatsUpdated = false;
            mUpdateTimer->stop();

            if (!mTorrents.empty()) {
                removedTorrentsIndices.reserve(mTorrents.size());
                for (int i = static_cast<int>(mTorrents.size()) - 1; i >= 0; --i) {
                    removedTorrentsIndices.push_back(i);
                }
                mTorrents.clear();
            }

            break;
        }
        case ConnectionState::Connecting:
            qInfo("Connecting");
            mUpdating = true;
            break;
        case ConnectionState::Connected:
        {
            qInfo("Connected");
            break;
        }
        }
    }

    void Rpc::emitSignalsOnConnectionStateChanged(Rpc::ConnectionState oldConnectionState, std::vector<int>&& removedTorrentsIndices)
    {
        emit connectionStateChanged();

        switch (mStatus.connectionState) {
        case ConnectionState::Disconnected:
        {
            if (oldConnectionState == ConnectionState::Connected) {
                emit connectedChanged();
            }
            if (!removedTorrentsIndices.empty()) {
                emit torrentsUpdated(removedTorrentsIndices, {}, 0);
            }
            break;
        }
        case ConnectionState::Connecting:
            break;
        case ConnectionState::Connected:
        {
            emit connectedChanged();
            break;
        }
        }
    }

    void Rpc::getServerSettings()
    {
        postRequest(QLatin1String("session-get"), QByteArrayLiteral("{\"method\":\"session-get\"}"),
                    [=](const auto& parseResult, bool success) {
                        if (success) {
                            mServerSettings->update(getReplyArguments(parseResult));
                            mServerSettingsUpdated = true;
                            if (mRpcVersionChecked) {
                                startUpdateTimer();
                            } else {
                                mRpcVersionChecked = true;

                                if (mServerSettings->minimumRpcVersion() > minimumRpcVersion) {
                                    setStatus(Status{ConnectionState::Disconnected, Error::ServerIsTooNew});
                                } else if (mServerSettings->rpcVersion() < minimumRpcVersion) {
                                    setStatus(Status{ConnectionState::Disconnected, Error::ServerIsTooOld});
                                } else {
                                    mLocal = isSessionIdFileExists();
                                    if (!mLocal) {
                                        mLocal = isAddressLocal(mServerUrl.host());
                                    }

                                    getTorrents();
                                    getServerStats();
                                }
                            }
                        }
                    });
    }

    using NewTorrent = std::pair<QJsonObject, int>;

    class TorrentsListUpdater : public ItemListUpdater<std::shared_ptr<Torrent>, NewTorrent, std::vector<NewTorrent>> {
    public:
        inline explicit TorrentsListUpdater(Rpc& rpc) : mRpc(rpc) {}

        void update(std::vector<std::shared_ptr<Torrent>>& torrents, std::vector<NewTorrent>&& newTorrents) override {
            if (newTorrents.size() < torrents.size()) {
                removed.reserve(torrents.size() - newTorrents.size());
            } else if (newTorrents.size() > torrents.size()) {
                checkSingleFileIds.reserve(static_cast<int>(newTorrents.size() - torrents.size()));
            }
            ItemListUpdater::update(torrents, std::move(newTorrents));
        }

        std::vector<int> removed;
        std::vector<int> changed;
        int added = 0;
        QVariantList checkSingleFileIds;
        QVariantList getFilesIds;
        QVariantList getPeersIds;

    protected:
        std::vector<NewTorrent>::iterator findNewItemForItem(std::vector<NewTorrent>& newTorrents, const std::shared_ptr<Torrent>& torrent) override {
            const int id = torrent->id();
            return std::find_if(newTorrents.begin(), newTorrents.end(), [id](const auto& t) {
                const auto& [json, newTorrentId] = t;
                return newTorrentId == id;
            });
        }

        void onAboutToRemoveItems(size_t first, size_t last) override {
            emit mRpc.onAboutToRemoveTorrents(first, last);
        };

        void onRemovedItems(size_t first, size_t last) override {
            removed.reserve(removed.size() + (last - first));
            for (size_t i = first; i < last; ++i) {
                removed.push_back(static_cast<int>(i));
            }
            emit mRpc.onRemovedTorrents(first, last);
        }

        bool updateItem(std::shared_ptr<Torrent>& torrent, NewTorrent&& newTorrent) override {
            const auto& [json, id] = newTorrent;

            const bool wasFinished = torrent->isFinished();
            const bool wasPaused = (torrent->status() == Torrent::Status::Paused);
            const auto oldSizeWhenDone = torrent->sizeWhenDone();
            const bool metadataWasComplete = torrent->isMetadataComplete();

            const bool changed = torrent->update(json);
            if (changed) {
                if (!wasFinished
                        && torrent->isFinished()
                        && !wasPaused
                        // Don't emit torrentFinished() if torrent's size became smaller
                        // since there is high chance that it happened because user unselected some files
                        // and torrent immediately became finished. We don't want notification in that case
                        && torrent->sizeWhenDone() >= oldSizeWhenDone) {
                    emit mRpc.torrentFinished(torrent.get());
                }
                if (!metadataWasComplete && torrent->isMetadataComplete()) {
                    checkSingleFileIds.push_back(id);
                }
            }
            if (torrent->isFilesEnabled()) {
                getFilesIds.push_back(id);
            }
            if (torrent->isPeersEnabled()) {
                getPeersIds.push_back(id);
            }

            return changed;
        }

        void onChangedItems(size_t first, size_t last) override {
            changed.reserve(changed.size() + (last - first));
            for (size_t i = first; i < last; ++i) {
                changed.push_back(static_cast<int>(i));
            }
            emit mRpc.onChangedTorrents(first, last);
        }

        std::shared_ptr<Torrent> createItemFromNewItem(NewTorrent&& newTorrent) override {
            const auto& [torrentJson, id] = newTorrent;
            auto torrent = std::make_shared<Torrent>(id, torrentJson, &mRpc);
#ifdef TREMOTESF_SAILFISHOS
            // prevent automatic destroying on QML side
            QQmlEngine::setObjectOwnership(torrent.get(), QQmlEngine::CppOwnership);
#endif
            if (mRpc.isConnected()) {
                emit mRpc.torrentAdded(torrent.get());
            }
            if (torrent->isMetadataComplete()) {
                checkSingleFileIds.push_back(id);
            }
            return torrent;
        }

        void onAboutToAddItems(size_t count) override {
            emit mRpc.onAboutToAddTorrents(count);
        }

        void onAddedItems(size_t count) override {
            added = static_cast<int>(count);
            emit mRpc.onAddedTorrents(count);
        };

    private:
        Rpc& mRpc;
    };

    void Rpc::getTorrents()
    {
        postRequest(QLatin1String("torrent-get"),
                    QByteArrayLiteral("{"
                                          "\"arguments\":{"
                                              "\"fields\":["
                                                  "\"activityDate\","
                                                  "\"addedDate\","
                                                  "\"bandwidthPriority\","
                                                  "\"comment\","
                                                  "\"creator\","
                                                  "\"dateCreated\","
                                                  "\"doneDate\","
                                                  "\"downloadDir\","
                                                  "\"downloadedEver\","
                                                  "\"downloadLimit\","
                                                  "\"downloadLimited\","
                                                  "\"error\","
                                                  "\"errorString\","
                                                  "\"eta\","
                                                  "\"hashString\","
                                                  "\"haveValid\","
                                                  "\"honorsSessionLimits\","
                                                  "\"id\","
                                                  "\"leftUntilDone\","
                                                  "\"magnetLink\","
                                                  "\"metadataPercentComplete\","
                                                  "\"name\","
                                                  "\"peer-limit\","
                                                  "\"peersConnected\","
                                                  "\"peersGettingFromUs\","
                                                  "\"peersSendingToUs\","
                                                  "\"percentDone\","
                                                  "\"queuePosition\","
                                                  "\"rateDownload\","
                                                  "\"rateUpload\","
                                                  "\"recheckProgress\","
                                                  "\"seedIdleLimit\","
                                                  "\"seedIdleMode\","
                                                  "\"seedRatioLimit\","
                                                  "\"seedRatioMode\","
                                                  "\"sizeWhenDone\","
                                                  "\"status\","
                                                  "\"totalSize\","
                                                  "\"trackerStats\","
                                                  "\"uploadedEver\","
                                                  "\"uploadLimit\","
                                                  "\"uploadLimited\","
                                                  "\"uploadRatio\","
                                                  "\"webseedsSendingToUs\""
                                              "]"
                                          "},"
                                          "\"method\":\"torrent-get\""
                                      "}"),
                    [=](const auto& parseResult, bool success) {
                        if (!success) {
                            return;
                        }

                        std::vector<NewTorrent> newTorrents;
                        {
                            const QJsonArray torrentsJsons(getReplyArguments(parseResult)
                                                            .value(QJsonKeyStringInit("torrents"))
                                                            .toArray());
                            newTorrents.reserve(static_cast<size_t>(torrentsJsons.size()));
                            for (const auto& i : torrentsJsons) {
                                QJsonObject torrentJson(i.toObject());
                                const int id = torrentJson.value(Torrent::idKey).toInt();
                                newTorrents.emplace_back(std::move(torrentJson), id);
                            }
                        }

                        TorrentsListUpdater updater(*this);
                        updater.update(mTorrents, std::move(newTorrents));

                        checkIfTorrentsUpdated();
                        startUpdateTimer();

                        emit torrentsUpdated(updater.removed, updater.changed, updater.added);

                        if (!updater.checkSingleFileIds.isEmpty()) {
                            checkTorrentsSingleFile(updater.checkSingleFileIds);
                        }
                        if (!updater.getFilesIds.isEmpty()) {
                            getTorrentsFiles(updater.getFilesIds, true);
                        }
                        if (!updater.getPeersIds.isEmpty()) {
                            getTorrentsPeers(updater.getPeersIds, true);
                        }
        });
    }

    void Rpc::checkTorrentsSingleFile(const QVariantList& torrentIds)
    {
        postRequest(QLatin1String("torrent-get"),
                    {{QLatin1String("fields"), QVariantList{QLatin1String("id"), QLatin1String("priorities")}},
                     {QLatin1String("ids"), torrentIds}},
                    [=](const auto& parseResult, bool success) {
                        if (success) {
                            const QJsonArray torrents(getReplyArguments(parseResult).value(torrentsKey).toArray());
                            for (const auto& i : torrents) {
                                const QJsonObject torrentMap(i.toObject());
                                const int torrentId = torrentMap.value(Torrent::idKey).toInt();
                                Torrent* torrent = torrentById(torrentId);
                                if (torrent) {
                                    torrent->checkSingleFile(torrentMap);
                                }
                            }
                        }
                    });
    }

    void Rpc::getServerStats()
    {
        postRequest(QLatin1String("session-stats"), QByteArrayLiteral("{\"method\":\"session-stats\"}"),
                    [=](const auto& parseResult, bool success) {
                        if (success) {
                            mServerStats->update(getReplyArguments(parseResult));
                            mServerStatsUpdated = true;
                            startUpdateTimer();
                        }
                    });
    }

    void Rpc::checkIfTorrentsUpdated()
    {
        if (mUpdating && !mTorrentsUpdated) {
            for (const std::shared_ptr<Torrent>& torrent : mTorrents) {
                if (!torrent->isUpdated()) {
                    return;
                }
            }
            mTorrentsUpdated = true;
        }
    }

    void Rpc::startUpdateTimer()
    {
        if (mUpdating && mServerSettingsUpdated && mTorrentsUpdated && mServerStatsUpdated) {
            if (connectionState() == ConnectionState::Connecting) {
                setStatus(Status{ConnectionState::Connected});
            }
            if (!mUpdateDisabled) {
                mUpdateTimer->start();
            }
            mUpdating = false;
        }
    }

    void Rpc::onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator)
    {
        if (mAuthentication && !mAuthenticationRequested) {
            authenticator->setUser(mUsername);
            authenticator->setPassword(mPassword);
            mAuthenticationRequested = true;
        }
    }

    QNetworkReply* Rpc::postRequest(Request&& request)
    {
        QNetworkReply* reply = mNetwork->post(request.request, request.data);
        mActiveNetworkRequests.insert(reply);

        reply->ignoreSslErrors(mExpectedSslErrors);

        QObject::connect(reply, &QNetworkReply::finished, this, [=, request = std::move(request)]() mutable {
            if (mActiveNetworkRequests.erase(reply) == 0) {
                reply->deleteLater();
                return;
            }

            if (connectionState() != ConnectionState::Disconnected) {
                switch (reply->error()) {
                case QNetworkReply::NoError:
                {
                    mRetryingNetworkRequests.erase(reply);

                    const QByteArray replyData(reply->readAll());
                    const auto future = QtConcurrent::run([replyData] {
                        QJsonParseError error{};
                        QJsonObject result(QJsonDocument::fromJson(replyData, &error).object());
                        const bool parsedOk = (error.error == QJsonParseError::NoError);
                        return std::pair<QJsonObject, bool>(std::move(result), parsedOk);
                    });
                    auto watcher = new QFutureWatcher<std::pair<QJsonObject, bool>>(this);
                    QObject::connect(watcher, &QFutureWatcher<std::pair<QJsonObject, bool>>::finished, this, [=] {
                        const auto result = watcher->result();
                        if (connectionState() != ConnectionState::Disconnected) {
                            const QJsonObject& parseResult = result.first;
                            const bool parsedOk = result.second;
                            if (parsedOk) {
                                const bool success = isResultSuccessful(parseResult);
                                if (!success) {
                                    qWarning() << "method" << request.method << "failed, response:" << parseResult;
                                }
                                if (request.callOnSuccessParse) {
                                    request.callOnSuccessParse(parseResult, success);
                                }
                            } else {
                                qWarning("Parsing error");
                                setStatus(Status{ConnectionState::Disconnected, Error::ParseError});
                            }
                        }
                        watcher->deleteLater();
                    });
                    watcher->setFuture(future);
                    break;
                }
                case QNetworkReply::AuthenticationRequiredError:
                    qWarning("Authentication error");
                    setStatus(Status{ConnectionState::Disconnected, Error::AuthenticationError});
                    break;
                case QNetworkReply::OperationCanceledError:
                case QNetworkReply::TimeoutError:
                    qWarning("Timed out");
                    if (!retryRequest(std::move(request), reply)) {
                        setStatus(Status{ConnectionState::Disconnected, Error::TimedOut});
                        if (mAutoReconnectEnabled && !mUpdateDisabled) {
                            qInfo("Auto reconnecting in %d seconds", mAutoReconnectTimer->interval() / 1000);
                            mAutoReconnectTimer->start();
                        }
                    }
                    break;
                default:
                {
                    if (reply->error() == QNetworkReply::ContentConflictError && reply->hasRawHeader(sessionIdHeader)) {
                        const auto newSessionId(reply->rawHeader(sessionIdHeader));
                        if (newSessionId != request.request.rawHeader(sessionIdHeader)) {
                            qInfo() << "Session id changed, retrying" << request.method << "request";
                            mSessionId = reply->rawHeader(sessionIdHeader);
                            // Retry without incrementing retryAttempts
                            request.setSessionId(mSessionId);
                            postRequest(std::move(request));
                            return;
                        }
                    }

                    const auto httpStatusCode(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute));
                    qWarning() << request.method << "request error" << reply->error() << reply->errorString();
                    if (httpStatusCode.isValid()) {
                        qWarning("HTTP status code %d", httpStatusCode.toInt());
                    }

                    if (!retryRequest(std::move(request), reply)) {
                        setStatus(Status{ConnectionState::Disconnected, Error::ConnectionError});
                        if (mAutoReconnectEnabled && !mUpdateDisabled) {
                            qInfo("Auto reconnecting in %d seconds", mAutoReconnectTimer->interval() / 1000);
                            mAutoReconnectTimer->start();
                        }
                    }
                }
                }
            }

            reply->deleteLater();
        });

        QObject::connect(this, &Rpc::connectedChanged, reply, [=] {
            if (!isConnected()) {
                reply->abort();
            }
        });

        auto timer = new QTimer(this);
        timer->setInterval(mTimeout);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
        QObject::connect(timer, &QTimer::timeout, timer, &QObject::deleteLater);
        timer->start();

        return reply;
    }

    bool Rpc::retryRequest(Request&& request, QNetworkReply* previousAttempt)
    {
        int retryAttempts;
        if (const auto found = mRetryingNetworkRequests.find(previousAttempt); found != mRetryingNetworkRequests.end()) {
            retryAttempts = found->second;
            mRetryingNetworkRequests.erase(found);
        } else {
            retryAttempts = 0;
        }
        ++retryAttempts;
        if (retryAttempts > maxRetryAttempts) {
            return false;
        }

        request.setSessionId(mSessionId);

        qWarning() << "Retrying" << request.method << "request, retry attempts =" << retryAttempts;
        QNetworkReply* reply = postRequest(std::move(request));
        mRetryingNetworkRequests.emplace(reply, retryAttempts);

        return true;
    }

    void Rpc::postRequest(QLatin1String method, const QByteArray& data, const std::function<void(const QJsonObject&, bool)>& callOnSuccessParse)
    {
        Request request{method, QNetworkRequest(mServerUrl), data, callOnSuccessParse};
        request.setSessionId(mSessionId);
        request.request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
        request.request.setSslConfiguration(mSslConfiguration);
        postRequest(std::move(request));
    }

    void Rpc::postRequest(QLatin1String method, const QVariantMap& arguments, const std::function<void(const QJsonObject&, bool)>& callOnSuccessParse)
    {
        postRequest(method, makeRequestData(method, arguments), callOnSuccessParse);
    }

    bool Rpc::isSessionIdFileExists() const
    {
#ifndef Q_OS_ANDROID
        if (mServerSettings->hasSessionIdFile()) {
            return !QStandardPaths::locate(sessionIdFileLocation, sessionIdFilePrefix + mSessionId).isEmpty();
        }
#endif
        return false;
    }

    void Rpc::Request::setSessionId(const QByteArray& sessionId)
    {
        request.setRawHeader(sessionIdHeader, sessionId);
    }
}
