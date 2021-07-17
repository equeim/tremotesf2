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
          mBackgroundUpdate(false),
          mUpdateDisabled(false),
          mUpdating(false),
          mAuthentication(false),
          mUpdateInterval(0),
          mBackgroundUpdateInterval(0),
          mTimeout(0),
          mLocal(false),
          mRpcVersionChecked(false),
          mServerSettingsUpdated(false),
          mTorrentsUpdated(false),
          mServerStatsUpdated(false),
          mUpdateTimer(new QTimer(this)),
          mServerSettings(new ServerSettings(this, this)),
          mServerStats(new ServerStats(this)),
          mConnectionState(ConnectionState::Disconnected),
          mError(Error::NoError)
    {
        QObject::connect(mNetwork, &QNetworkAccessManager::authenticationRequired, this, &Rpc::onAuthenticationRequired);

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
        return (mConnectionState == ConnectionState::Connected);
    }

    Rpc::ConnectionState Rpc::connectionState() const
    {
        return mConnectionState;
    }

    Rpc::Error Rpc::error() const
    {
        return mError;
    }

    const QString& Rpc::errorMessage() const
    {
        return mErrorMessage;
    }

    bool Rpc::isLocal() const
    {
        return mLocal;
    }

    int Rpc::torrentsCount() const
    {
        return static_cast<int>(mTorrents.size());
    }

    bool Rpc::backgroundUpdate() const
    {
        return mBackgroundUpdate;
    }

    void Rpc::setBackgroundUpdate(bool background)
    {
        if (background != mBackgroundUpdate) {
            mBackgroundUpdate = background;
            const int interval = background ? mBackgroundUpdateInterval : mUpdateInterval;
            if (mUpdateTimer->isActive()) {
                mUpdateTimer->stop();
                mUpdateTimer->setInterval(interval);
                startUpdateTimer();
            } else if (!mServerUrl.isEmpty()) {
                mUpdateTimer->setInterval(interval);
            }
            emit backgroundUpdateChanged();
        }
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
        mUpdateInterval = server.updateInterval * 1000; // msecs
        mBackgroundUpdateInterval = server.backgroundUpdateInterval * 1000; // msecs
        mUpdateTimer->setInterval(mUpdateInterval);
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
        mUpdateInterval = 0;
        mBackgroundUpdateInterval = 0;
        mTimeout = 0;
        mLocal = false;
    }

    void Rpc::connect()
    {
        if (mConnectionState == ConnectionState::Disconnected && !mServerUrl.isEmpty()) {
            setError(Error::NoError);
            setConnectionState(ConnectionState::Connecting);
            getServerSettings();
        }
    }

    void Rpc::disconnect()
    {
        setError(Error::NoError);
        setConnectionState(ConnectionState::Disconnected);
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
                            for (const QJsonValue& torrentValue : torrents) {
                                const QJsonObject torrentMap(torrentValue.toObject());
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
                            for (const QJsonValue& torrentValue : torrents) {
                                const QJsonObject torrentMap(torrentValue.toObject());
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

    void Rpc::setConnectionState(ConnectionState state)
    {
        if (state == mConnectionState) {
            return;
        }

        const bool wasConnected = isConnected();

        if (wasConnected && (state == ConnectionState::Disconnected)) {
            emit aboutToDisconnect();
        }

        mConnectionState = state;

        switch (mConnectionState) {
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

            emit connectionStateChanged();

            if (wasConnected) {
                emit connectedChanged();
            }

            if (!mTorrents.empty()) {
                std::vector<int> removed;
                removed.reserve(mTorrents.size());
                for (int i = static_cast<int>(mTorrents.size()) - 1; i >= 0; --i) {
                    removed.push_back(i);
                }
                mTorrents.clear();
                emit torrentsUpdated(removed, {}, 0);
            }

            break;
        }
        case ConnectionState::Connecting:
            qInfo("Connecting");
            mUpdating = true;
            emit connectionStateChanged();
            break;
        case ConnectionState::Connected:
        {
            qInfo("Connected");
            emit connectionStateChanged();
            emit connectedChanged();
            break;
        }
        }
    }

    void Rpc::setError(Error error, const QString& errorMessage)
    {
        if (error != mError) {
            mError = error;
            mErrorMessage = errorMessage;
            emit errorChanged();
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
                                    setError(Error::ServerIsTooNew);
                                    setConnectionState(ConnectionState::Disconnected);
                                } else if (mServerSettings->rpcVersion() < minimumRpcVersion) {
                                    setError(Error::ServerIsTooOld);
                                    setConnectionState(ConnectionState::Disconnected);
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
                                                  "\"uploadRatio\""
                                              "]"
                                          "},"
                                          "\"method\":\"torrent-get\""
                                      "}"),
                    [=](const auto& parseResult, bool success) {
                        if (!success) {
                            return;
                        }

                        std::vector<std::tuple<QJsonObject, int, bool>> newTorrents;
                        {
                            const QJsonArray torrentsJsons(getReplyArguments(parseResult)
                                                            .value(QJsonKeyStringInit("torrents"))
                                                            .toArray());
                            newTorrents.reserve(static_cast<size_t>(torrentsJsons.size()));
                            for (const QJsonValue& torrentValue : torrentsJsons) {
                                QJsonObject torrentJson(torrentValue.toObject());
                                const int id = torrentJson.value(Torrent::idKey).toInt();
                                newTorrents.emplace_back(std::move(torrentJson), id, false);
                            }
                        }

                        std::vector<int> removed;
                        if (newTorrents.size() < mTorrents.size()) {
                            removed.reserve(mTorrents.size() - newTorrents.size());
                        }
                        std::vector<int> changed;

                        QVariantList checkSingleFileIds;
                        if (newTorrents.size() > mTorrents.size()) {
                            checkSingleFileIds.reserve(static_cast<int>(newTorrents.size() - mTorrents.size()));
                        }

                        QVariantList getFilesIds;
                        QVariantList getPeersIds;

                        {
                            const auto newTorrentsEnd(newTorrents.end());
                            VectorBatchRemover<std::shared_ptr<Torrent>> remover(mTorrents, &removed, &changed);
                            for (int i = static_cast<int>(mTorrents.size()) - 1; i >= 0; --i) {
                                const auto& torrent = mTorrents[static_cast<size_t>(i)];
                                const int id = torrent->id();
                                const auto found(std::find_if(newTorrents.begin(), newTorrents.end(), [id](const auto& t) {
                                    const auto& [json, new_id, existing] = t;
                                    return new_id == id;
                                }));
                                if (found == newTorrentsEnd) {
                                    remover.remove(i);
                                } else {
                                    auto& [json, id, existing] = *found;
                                    existing = true;

                                    const bool wasFinished = torrent->isFinished();
                                    const bool wasPaused = (torrent->status() == Torrent::Status::Paused);
                                    const auto oldSizeWhenDone = torrent->sizeWhenDone();
                                    const bool metadataWasComplete = torrent->isMetadataComplete();
                                    if (torrent->update(json)) {
                                        changed.push_back(i);
                                        if (!wasFinished
                                                && torrent->isFinished()
                                                && !wasPaused
                                                // Don't emit torrentFinished() if torrent's size became smaller
                                                // since there is high chance that it happened because user unselected some files
                                                // and torrent immediately became finished. We don't want notification in that case
                                                && torrent->sizeWhenDone() >= oldSizeWhenDone) {
                                            emit torrentFinished(torrent.get());
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
                                }
                            }
                            remover.doRemove();
                        }
                        std::reverse(changed.begin(), changed.end());

                        int added = 0;
                        if (newTorrents.size() > mTorrents.size()) {
                            mTorrents.reserve(newTorrents.size());
                            for (const auto& t : newTorrents) {
                                const auto& [torrentJson, id, existing] = t;
                                if (!existing) {
                                    mTorrents.emplace_back(std::make_shared<Torrent>(id, torrentJson, this));
                                    ++added;
                                    Torrent* torrent = mTorrents.back().get();
#ifdef TREMOTESF_SAILFISHOS
                                    // prevent automatic destroying on QML side
                                    QQmlEngine::setObjectOwnership(torrent, QQmlEngine::CppOwnership);
#endif
                                    if (isConnected()) {
                                        emit torrentAdded(torrent);
                                    }

                                    if (torrent->isMetadataComplete()) {
                                        checkSingleFileIds.push_back(id);
                                    }
                                }
                            }
                        }

                        checkIfTorrentsUpdated();
                        startUpdateTimer();

                        emit torrentsUpdated(removed, changed, added);

                        if (!checkSingleFileIds.isEmpty()) {
                            checkTorrentsSingleFile(checkSingleFileIds);
                        }
                        if (!getFilesIds.isEmpty()) {
                            getTorrentsFiles(getFilesIds, true);
                        }
                        if (!getPeersIds.isEmpty()) {
                            getTorrentsPeers(getPeersIds, true);
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
                            for (const QJsonValue& torrentValue : torrents) {
                                const QJsonObject torrentMap(torrentValue.toObject());
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
            if (mConnectionState == ConnectionState::Connecting) {
                setConnectionState(ConnectionState::Connected);
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

            if (mConnectionState != ConnectionState::Disconnected) {
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
                        if (mConnectionState != ConnectionState::Disconnected) {
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
                                setError(Error::ParseError);
                                setConnectionState(ConnectionState::Disconnected);
                            }
                        }
                        watcher->deleteLater();
                    });
                    watcher->setFuture(future);
                    break;
                }
                case QNetworkReply::AuthenticationRequiredError:
                    qWarning("Authentication error");
                    setError(Error::AuthenticationError);
                    setConnectionState(ConnectionState::Disconnected);
                    break;
                case QNetworkReply::OperationCanceledError:
                case QNetworkReply::TimeoutError:
                    qWarning("Timed out");
                    if (!retryRequest(std::move(request), reply)) {
                        setError(Error::TimedOut);
                        setConnectionState(ConnectionState::Disconnected);
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
                        setError(Error::ConnectionError, reply->errorString());
                        setConnectionState(ConnectionState::Disconnected);
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

    void Rpc::postRequest(const QLatin1String& method, const QByteArray& data, const std::function<void(const QJsonObject&, bool)>& callOnSuccessParse)
    {
        Request request{method, QNetworkRequest(mServerUrl), data, callOnSuccessParse};
        request.setSessionId(mSessionId);
        request.request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
        request.request.setSslConfiguration(mSslConfiguration);
        postRequest(std::move(request));
    }

    void Rpc::postRequest(const QLatin1String& method, const QVariantMap& arguments, const std::function<void (const QJsonObject&, bool)>& callOnSuccessParse)
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
