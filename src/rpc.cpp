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

#include "rpc.h"

#include <QAuthenticator>
#include <QCoreApplication>
#include <QDebug>
#include <QFutureWatcher>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QTimer>
#include <QSslCertificate>
#include <QSslKey>
#include <QtConcurrentRun>

#ifdef TREMOTESF_SAILFISHOS
#include <QQmlEngine>
#endif

#include "servers.h"
#include "serversettings.h"
#include "serverstats.h"
#include "settings.h"
#include "torrent.h"
#include "torrentsproxymodel.h"

namespace tremotesf
{
    namespace
    {
        // Transmission 2.40+
        const int minimumRpcVersion = 14;

        const QByteArray sessionIdHeader("X-Transmission-Session-Id");

        QByteArray makeRequestData(const QString& method, const QVariantMap& arguments)
        {
            return QJsonDocument::fromVariant(QVariantMap{{QLatin1String("method"), method},
                                                          {QLatin1String("arguments"), arguments}})
                .toJson();
        }

        QVariantMap getReplyArguments(const QVariantMap& parseResult)
        {
            return parseResult.value(QLatin1String("arguments")).toMap();
        }

        bool isResultSuccessful(const QVariantMap& parseResult)
        {
            return (parseResult.value(QLatin1String("result")).toString() == "success");
        }
    }

    Rpc::Rpc(QObject* parent)
        : QObject(parent),
          mNetwork(new QNetworkAccessManager(this)),
          mAuthenticationRequested(false),
          mAuthentication(false),
          mSelfSignedCertificate(false),
          mTimeout(0),
          mRpcVersionChecked(false),
          mServerSettingsUpdated(false),
          mTorrentsUpdated(false),
          mFirstUpdate(true),
          mServerStatsUpdated(false),
          mUpdateTimer(new QTimer(this)),
          mServerSettings(new ServerSettings(this, this)),
          mServerStats(new ServerStats(this)),
          mStatus(Disconnected),
          mError(NoError)
    {
        QObject::connect(mNetwork, &QNetworkAccessManager::authenticationRequired, this, &Rpc::onAuthenticationRequired);

        mUpdateTimer->setSingleShot(true);
        QObject::connect(mUpdateTimer, &QTimer::timeout, this, &Rpc::updateData);

        QObject::connect(mNetwork, &QNetworkAccessManager::sslErrors, this, [=](QNetworkReply* reply, const QList<QSslError>& errors) {
            /*if (mSelfSignedCertificate) {
                if (errors.length() == 1 && errors.first().error() == QSslError::HostNameMismatch) {
                    reply->ignoreSslErrors(errors);
                    return;
                }
            }*/
            qWarning() << errors;
        });

        QObject::connect(Servers::instance(), &Servers::currentServerChanged, this, &Rpc::updateServer);
        updateServer();
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

    bool Rpc::isConnected() const
    {
        return (mStatus == Connected);
    }

    Rpc::Status Rpc::status() const
    {
        return mStatus;
    }

    QString Rpc::statusString() const
    {
        switch (mStatus) {
        case Disconnected:
            switch (mError) {
            case NoError:
                return qApp->translate("tremotesf", "Disconnected");
            case TimedOut:
                return qApp->translate("tremotesf", "Timed out");
            case ConnectionError:
                return qApp->translate("tremotesf", "Connection error");
            case AuthenticationError:
                return qApp->translate("tremotesf", "Authentication error");
            case ParseError:
                return qApp->translate("tremotesf", "Parse error");
            case ServerIsTooNew:
                return qApp->translate("tremotesf", "Server is too new");
            case ServerIsTooOld:
                return qApp->translate("tremotesf", "Server is too old");
            }
            break;
        case Connecting:
            return qApp->translate("tremotesf", "Connecting...");
        case Connected:
            return qApp->translate("tremotesf", "Connected");
        }

        return QString();
    }

    Rpc::Error Rpc::error() const
    {
        return mError;
    }

    bool Rpc::isLocal() const
    {
        const QString hostName = mServerUrl.host();
        if (hostName == QLatin1String("localhost") ||
            hostName == QHostInfo::localHostName()) {
            return true;
        }

        const QHostAddress ipAddress(hostName);
        if (!ipAddress.isNull() &&
            (ipAddress.isLoopback() || QNetworkInterface::allAddresses().contains(ipAddress))) {
            return true;
        }

        return false;
    }

    int Rpc::torrentsCount() const
    {
        return mTorrents.size();
    }

    void Rpc::connect()
    {
        setError(NoError);
        setStatus(Connecting);
        getServerSettings();
    }

    void Rpc::disconnect()
    {
        setError(NoError);
        setStatus(Disconnected);
    }

    void Rpc::addTorrentFile(const QByteArray& fileData,
                             const QString& downloadDirectory,
                             const QVariantList& wantedFiles,
                             const QVariantList& unwantedFiles,
                             const QVariantList& highPriorityFiles,
                             const QVariantList& normalPriorityFiles,
                             const QVariantList& lowPriorityFiles,
                             int bandwidthPriority,
                             bool start)
    {
        if (isConnected()) {
            const auto future = QtConcurrent::run([=]() {
                return makeRequestData(QLatin1String("torrent-add"),
                                       {{QLatin1String("metainfo"), fileData.toBase64()},
                                        {QLatin1String("download-dir"), downloadDirectory},
                                        {QLatin1String("files-wanted"), wantedFiles},
                                        {QLatin1String("files-unwanted"), unwantedFiles},
                                        {QLatin1String("priority-high"), highPriorityFiles},
                                        {QLatin1String("priority-normal"), normalPriorityFiles},
                                        {QLatin1String("priority-low"), lowPriorityFiles},
                                        {QLatin1String("bandwidthPriority"), bandwidthPriority},
                                        {QLatin1String("paused"), !start}});
            });
            auto watcher = new QFutureWatcher<QByteArray>(this);
            QObject::connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [=]() {
                if (isConnected()) {
                    postRequest(watcher->result(), [=](const QVariantMap& parseResult) {
                        if (getReplyArguments(parseResult).contains(QLatin1String("torrent-added"))) {
                            updateData();
                        }
                    });
                    watcher->deleteLater();
                }
            });
            watcher->setFuture(future);
        }
    }

    void Rpc::addTorrentLink(const QString& link,
                             const QString& downloadDirectory,
                             int bandwidthPriority,
                             bool start)
    {
        if (!isConnected()) {
            return;
        }

        postRequest(makeRequestData(QLatin1String("torrent-add"),
                                    {{QLatin1String("filename"), link},
                                     {QLatin1String("download-dir"), downloadDirectory},
                                     {QLatin1String("bandwidthPriority"), bandwidthPriority},
                                     {QLatin1String("paused"), !start}}),
                    [=](const QVariantMap& parseResult) {
                        if (getReplyArguments(parseResult).contains(QLatin1String("torrent-added"))) {
                            updateData();
                        }
                    });
    }

    void Rpc::startTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-start"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::startTorrentsNow(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-start-now"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::pauseTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-stop"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::removeTorrents(const QVariantList& ids, bool deleteFiles)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-remove"),
                                        {{QLatin1String("ids"), ids},
                                         {QLatin1String("delete-local-data"), deleteFiles}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::checkTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-verify"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsToTop(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("queue-move-top"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsUp(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("queue-move-up"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsDown(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("queue-move-down"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsToBottom(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("queue-move-bottom"),
                                        {{QLatin1String("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::setSessionProperty(const QString& property, const QVariant& value)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("session-set"), {{property, value}}));
        }
    }

    void Rpc::setSessionProperties(const QVariantMap& properties)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("session-set"), properties));
        }
    }

    void Rpc::setTorrentProperty(int id, const QString& property, const QVariant& value)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-set"),
                                        {{QLatin1String("ids"), QVariantList{id}},
                                         {property, value}}));
        }
    }

    void Rpc::getTorrentFiles(int id, bool scheduled)
    {
        postRequest(QStringLiteral("{"
                                   "    \"arguments\": {"
                                   "        \"fields\": ["
                                   "            \"files\","
                                   "            \"fileStats\""
                                   "        ],"
                                   "        \"ids\": [%1]"
                                   "    },"
                                   "    \"method\": \"torrent-get\""
                                   "}")
                        .arg(id)
                        .toLatin1(),
                    [=](const QVariantMap& parseResult) {
                        const QVariantList torrentsVariants(getReplyArguments(parseResult)
                                                                .value(QLatin1String("torrents"))
                                                                .toList());
                        const std::shared_ptr<Torrent> torrent(torrentById(id));
                        if (!torrentsVariants.isEmpty() && torrent) {
                            torrent->updateFiles(torrentsVariants.first().toMap());
                            if (scheduled) {
                                checkIfTorrentsUpdated();
                                startUpdateTimer();
                            }
                        }
                    });
    }

    void Rpc::getTorrentPeers(int id, bool scheduled)
    {
        postRequest(QStringLiteral("{"
                                   "    \"arguments\": {"
                                   "        \"fields\": [\"peers\"],"
                                   "        \"ids\": [%1]"
                                   "    },"
                                   "    \"method\": \"torrent-get\""
                                   "}")
                        .arg(id)
                        .toLatin1(),
                    [=](const QVariantMap& parseResult) {
                        const QVariantList torrentsVariants(getReplyArguments(parseResult)
                                                                .value(QLatin1String("torrents"))
                                                                .toList());
                        const std::shared_ptr<Torrent> torrent(torrentById(id));
                        if (!torrentsVariants.isEmpty() && torrent) {
                            torrent->updatePeers(torrentsVariants.first().toMap());
                            if (scheduled) {
                                checkIfTorrentsUpdated();
                                startUpdateTimer();
                            }
                        }
                    });
    }

    void Rpc::renameTorrentFile(int torrentId, const QString& filePath, const QString& newName)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-rename-path"),
                                        {{QLatin1String("ids"), QVariantList{torrentId}},
                                         {QLatin1String("path"), filePath},
                                         {QLatin1String("name"), newName}}),
                        [=](const QVariantMap& parseResult) {
                            const std::shared_ptr<Torrent> torrent(torrentById(torrentId));
                            if (torrent) {
                                const QVariantMap arguments(getReplyArguments(parseResult));
                                emit torrent->fileRenamed(arguments.value(QLatin1String("path")).toString(),
                                                          arguments.value(QLatin1String("name")).toString());
                                updateData();
                            }
                        });
        }
    }

    void Rpc::getDownloadDirFreeSpace()
    {
        if (isConnected()) {
            postRequest("{"
                        "    \"arguments\": {"
                        "        \"fields\": ["
                        "            \"download-dir-free-space\""
                        "        ]"
                        "    },"
                        "    \"method\": \"session-get\""
                        "}",
                        [=](const QVariantMap& parseResult) {
                            emit gotDownloadDirFreeSpace(getReplyArguments(parseResult).value("download-dir-free-space").toLongLong());
                        });
        }
    }

    void Rpc::getFreeSpaceForPath(const QString& path)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("free-space"),
                                        {{QLatin1String("path"), path}}),
                        [=](const QVariantMap& parseResult) {
                            emit gotFreeSpaceForPath(path, isResultSuccessful(parseResult), getReplyArguments(parseResult).value("size-bytes").toLongLong());
                        });
        }
    }

    void Rpc::setStatus(Status status)
    {
        if (status == mStatus) {
            return;
        }

        const bool wasConnected = isConnected();

        mStatus = status;
        emit statusChanged();
        emit statusStringChanged();

        if (isConnected()) {
            emit connectedChanged();
            emit torrentsUpdated();
        } else if (wasConnected) {
            mNetwork->clearAccessCache();
            mAuthenticationRequested = false;
            mRpcVersionChecked = false;
            mFirstUpdate = true;
            mTorrents.clear();
            emit connectedChanged();
            emit torrentsUpdated();
        }

        switch (mStatus) {
        case Disconnected:
            qDebug() << "disconnected";
            break;
        case Connecting:
            qDebug() << "connecting";
            break;
        case Connected:
            qDebug() << "connected";
        }
    }

    void Rpc::setError(Error error)
    {
        if (error != mError) {
            mError = error;
            emit statusStringChanged();
            emit errorChanged();
        }
    }

    void Rpc::updateServer()
    {
        mNetwork->clearAccessCache();

        if (!Servers::instance()->hasServers()) {
            disconnect();
            return;
        }

        const bool wasConnected = (mStatus != Disconnected);

        disconnect();

        const Server server(Servers::instance()->currentServer());

        mServerUrl.setHost(server.address);
        mServerUrl.setPort(server.port);
        mServerUrl.setPath(server.apiPath);
        if (server.https) {
            mServerUrl.setScheme(QLatin1String("https"));
        } else {
            mServerUrl.setScheme(QLatin1String("http"));
        }

        mSslConfiguration = QSslConfiguration::defaultConfiguration();

        mSelfSignedCertificate = server.selfSignedCertificateEnabled;
        if (mSelfSignedCertificate) {
            mSslConfiguration.setCaCertificates({QSslCertificate(server.selfSignedCertificate)});
        }

        if (server.clientCertificateEnabled) {
            mSslConfiguration.setLocalCertificate(QSslCertificate(server.clientCertificate));
            mSslConfiguration.setPrivateKey(QSslKey(server.clientCertificate, QSsl::Rsa));
        }

        mAuthentication = server.authentication;
        mUsername = server.username;
        mPassword = server.password;
        mTimeout = server.timeout * 1000; // msecs
        mUpdateTimer->setInterval(server.updateInterval * 1000); // msecs

        if (wasConnected) {
            connect();
        }
    }

    void Rpc::getServerSettings()
    {
        postRequest(QByteArrayLiteral("{\"method\": \"session-get\"}"),
                    [=](const QVariantMap& parseResult) {
                        mServerSettings->update(getReplyArguments(parseResult));
                        mServerSettingsUpdated = true;
                        if (mRpcVersionChecked) {
                            startUpdateTimer();
                        } else {
                            mRpcVersionChecked = true;
                            if (mServerSettings->minimumRpcVersion() > minimumRpcVersion) {
                                setError(ServerIsTooNew);
                                setStatus(Disconnected);
                            } else if (mServerSettings->rpcVersion() < minimumRpcVersion) {
                                setError(ServerIsTooOld);
                                setStatus(Disconnected);
                            } else {
                                getTorrents();
                                getServerStats();
                            }
                        }
                    });
    }

    void Rpc::getTorrents()
    {
        postRequest(QByteArrayLiteral("{"
                                      "    \"arguments\": {"
                                      "        \"fields\": ["
                                      "            \"activityDate\","
                                      "            \"addedDate\","
                                      "            \"bandwidthPriority\","
                                      "            \"comment\","
                                      "            \"creator\","
                                      "            \"dateCreated\","
                                      "            \"doneDate\","
                                      "            \"downloadDir\","
                                      "            \"downloadedEver\","
                                      "            \"downloadLimit\","
                                      "            \"downloadLimited\","
                                      "            \"error\","
                                      "            \"errorString\","
                                      "            \"eta\","
                                      "            \"hashString\","
                                      "            \"haveValid\","
                                      "            \"honorsSessionLimits\","
                                      "            \"id\","
                                      "            \"leftUntilDone\","
                                      "            \"name\","
                                      "            \"peer-limit\","
                                      "            \"peersConnected\","
                                      "            \"peersGettingFromUs\","
                                      "            \"peersSendingToUs\","
                                      "            \"percentDone\","
                                      "            \"queuePosition\","
                                      "            \"rateDownload\","
                                      "            \"rateUpload\","
                                      "            \"recheckProgress\","
                                      "            \"seedIdleLimit\","
                                      "            \"seedIdleMode\","
                                      "            \"seedRatioLimit\","
                                      "            \"seedRatioMode\","
                                      "            \"sizeWhenDone\","
                                      "            \"status\","
                                      "            \"totalSize\","
                                      "            \"trackerStats\","
                                      "            \"uploadedEver\","
                                      "            \"uploadLimit\","
                                      "            \"uploadLimited\","
                                      "            \"uploadRatio\""
                                      "        ]"
                                      "    },"
                                      "    \"method\": \"torrent-get\""
                                      "}"),
                    [=](const QVariantMap& parseResult) {
                        const QVariantList torrentsVariants(getReplyArguments(parseResult)
                                                                .value(QLatin1String("torrents"))
                                                                .toList());

                        std::vector<std::shared_ptr<Torrent>> torrents;
                        for (const QVariant& torrentVariant : torrentsVariants) {
                            const QVariantMap torrentMap(torrentVariant.toMap());
                            const int id = torrentMap.value(Torrent::idKey).toInt();

                            std::shared_ptr<Torrent> torrent(torrentById(id));
                            if (torrent) {
                                const bool wasFinished = (torrent->percentDone() == 1);
                                torrent->update(torrentMap);
                                const bool finished = (torrent->percentDone() == 1);

                                if (finished && !wasFinished) {
                                    emit torrentFinished(torrent->name());
                                }

                                if (torrent->isFilesEnabled()) {
                                    getTorrentFiles(id, true);
                                }
                                if (torrent->isPeersEnabled()) {
                                    getTorrentPeers(id, true);
                                }
                            } else {
                                torrent = std::make_shared<Torrent>(id, torrentMap, this);
#ifdef TREMOTESF_SAILFISHOS
                                // prevent automatic destroying on QML side
                                QQmlEngine::setObjectOwnership(torrent.get(), QQmlEngine::CppOwnership);
#endif

                                if (!mFirstUpdate) {
                                    emit torrentAdded(torrent->name());
                                }
                            }
                            torrents.push_back(std::move(torrent));
                        }
                        mTorrents = std::move(torrents);

                        mFirstUpdate = false;

                        checkIfTorrentsUpdated();
                        startUpdateTimer();
                    });
    }

    void Rpc::getServerStats()
    {
        postRequest(QByteArrayLiteral("{\"method\": \"session-stats\"}"),
                    [=](const QVariantMap& parseResult) {
                        mServerStats->update(getReplyArguments(parseResult));
                        mServerStatsUpdated = true;
                        startUpdateTimer();
                    });
    }

    void Rpc::checkIfTorrentsUpdated()
    {
        for (const std::shared_ptr<Torrent>& torrent : mTorrents) {
            if (!torrent->isUpdated()) {
                return;
            }
        }
        mTorrentsUpdated = true;
        if (isConnected()) {
            emit torrentsUpdated();
        }
    }

    void Rpc::startUpdateTimer()
    {
        if (mServerSettingsUpdated && mTorrentsUpdated && mServerStatsUpdated) {
            if (mStatus == Connecting) {
                setStatus(Connected);
            }
            mUpdateTimer->start();
        }
    }

    void Rpc::updateData()
    {
        mServerSettingsUpdated = false;
        mTorrentsUpdated = false;
        mServerStatsUpdated = false;

        mUpdateTimer->stop();

        getServerSettings();
        getTorrents();
        getServerStats();
    }

    void Rpc::onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator)
    {
        if (mAuthentication && !mAuthenticationRequested) {
            authenticator->setUser(mUsername);
            authenticator->setPassword(mPassword);
            mAuthenticationRequested = true;
        }
    }

    void Rpc::postRequest(const QByteArray& data,
                          const std::function<void(const QVariantMap&)>& callOnSuccess)
    {
        QNetworkRequest request(mServerUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
        request.setRawHeader(sessionIdHeader, mSessionId);
        request.setSslConfiguration(mSslConfiguration);

        QNetworkReply* reply = mNetwork->post(request, data);

        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Disconnected) {
                switch (reply->error()) {
                case QNetworkReply::NoError:
                    if (callOnSuccess) {
                        const QByteArray replyData(reply->readAll());
                        const auto future = QtConcurrent::run([=]() {
                            QJsonParseError error;
                            const QVariantMap result(QJsonDocument::fromJson(replyData, &error).toVariant().toMap());
                            return std::pair<QVariantMap, bool>(result, error.error == QJsonParseError::NoError);
                        });
                        auto watcher = new QFutureWatcher<std::pair<QVariantMap, bool>>(this);
                        QObject::connect(watcher, &QFutureWatcher<std::pair<QVariantMap, bool>>::finished, this, [=]() {
                            const auto result = watcher->result();
                            if (mStatus != Disconnected) {
                                if (result.second) {
                                    callOnSuccess(result.first);
                                } else {
                                    qWarning() << "parsing error";
                                    setError(ParseError);
                                    setStatus(Disconnected);
                                }
                            }
                            watcher->deleteLater();
                        });
                        watcher->setFuture(future);
                    }
                    break;
                case QNetworkReply::AuthenticationRequiredError:
                    qWarning() << "authentication error";
                    setError(AuthenticationError);
                    setStatus(Disconnected);
                    break;
                case QNetworkReply::OperationCanceledError:
                case QNetworkReply::TimeoutError:
                    qWarning() << "timed out";
                    setError(TimedOut);
                    setStatus(Disconnected);
                    break;
                default:
                    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409 &&
                        reply->hasRawHeader(sessionIdHeader)) {
                        mSessionId = reply->rawHeader(sessionIdHeader);
                        postRequest(data, callOnSuccess);
                    } else {
                        qWarning() << reply->error() << reply->errorString();
                        setError(ConnectionError);
                        setStatus(Disconnected);
                    }
                }
            }
            reply->deleteLater();
        });

        QObject::connect(this, &Rpc::connectedChanged, reply, [=]() {
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
    }

    void Rpc::postRequest(const QByteArray& data, const std::function<void()>& callOnSuccess)
    {
        postRequest(data, [=](const QVariantMap&) { callOnSuccess(); });
    }

    std::shared_ptr<Torrent> Rpc::torrentById(int id) const
    {
        const auto found = std::find_if(mTorrents.cbegin(), mTorrents.cend(), [id](const std::shared_ptr<Torrent>& torrent) {
            return (torrent->id() == id);
        });
        if (found != mTorrents.cend()) {
            return *found;
        }
        return {};
    }
}
