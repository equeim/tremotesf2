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
#include <QNetworkReply>
#include <QTimer>
#include <QSslCertificate>
#include <QSslKey>
#include <QtConcurrentRun>

#ifdef TREMOTESF_SAILFISHOS
#include <QQmlEngine>
#endif

#include "serversettings.h"
#include "serverstats.h"
#include "torrent.h"

namespace libtremotesf
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

        QJsonObject getReplyArguments(const QJsonObject& parseResult)
        {
            return parseResult.value(QLatin1String("arguments")).toObject();
        }

        bool isResultSuccessful(const QJsonObject& parseResult)
        {
            return (parseResult.value(QLatin1String("result")).toString() == QLatin1String("success"));
        }
    }

    Rpc::Rpc(ServerSettings* serverSettings, QObject* parent)
        : QObject(parent),
          mNetwork(new QNetworkAccessManager(this)),
          mAuthenticationRequested(false),
          mBackgroundUpdate(false),
          mUpdateDisabled(false),
          mAuthentication(false),
          mUpdateInterval(0),
          mBackgroundUpdateInterval(0),
          mTimeout(0),
          mRpcVersionChecked(false),
          mServerSettingsUpdated(false),
          mTorrentsUpdated(false),
          mServerStatsUpdated(false),
          mUpdateTimer(new QTimer(this)),
          mServerSettings(serverSettings ? serverSettings : new ServerSettings(this, this)),
          mServerStats(new ServerStats(this)),
          mStatus(Disconnected),
          mError(NoError)
    {
        QObject::connect(mNetwork, &QNetworkAccessManager::authenticationRequired, this, &Rpc::onAuthenticationRequired);

        mUpdateTimer->setSingleShot(true);
        QObject::connect(mUpdateTimer, &QTimer::timeout, this, &Rpc::updateData);

        QObject::connect(mNetwork, &QNetworkAccessManager::sslErrors, this, [=](QNetworkReply* reply, const QList<QSslError>& errors) {
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

    bool Rpc::isConnected() const
    {
        return (mStatus == Connected);
    }

    Rpc::Status Rpc::status() const
    {
        return mStatus;
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

    bool Rpc::backgroundUpdate() const
    {
        return mBackgroundUpdate;
    }

    void Rpc::setBackgroundUpdate(bool background)
    {
        if (background != mBackgroundUpdate) {
            mBackgroundUpdate = background;
            const int interval = [=]() {
                if (background) {
                    return mBackgroundUpdateInterval;
                } else {
                    return mUpdateInterval;
                }
            }();
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
                    startUpdateTimer();
                }
            }
            emit updateDisabledChanged();
        }
    }

    void Rpc::setServer(const Server& server)
    {
        mNetwork->clearAccessCache();

        const bool wasConnected = (mStatus != Disconnected);

        disconnect();

        mServerUrl.setHost(server.address);
        mServerUrl.setPort(server.port);
        mServerUrl.setPath(server.apiPath);
        if (server.https) {
            mServerUrl.setScheme(QLatin1String("https"));
        } else {
            mServerUrl.setScheme(QLatin1String("http"));
        }

        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        mExpectedSslErrors.clear();

        if (server.selfSignedCertificateEnabled) {
            const QSslCertificate certificate(server.selfSignedCertificate);
            mExpectedSslErrors.reserve(2);
            mExpectedSslErrors.push_back(QSslError(QSslError::HostNameMismatch, certificate));
            mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificate, certificate));
        }

        if (server.clientCertificateEnabled) {
            mSslConfiguration.setLocalCertificate(QSslCertificate(server.clientCertificate));
            mSslConfiguration.setPrivateKey(QSslKey(server.clientCertificate, QSsl::Rsa));
        }

        mAuthentication = server.authentication;
        mUsername = server.username;
        mPassword = server.password;
        mTimeout = server.timeout * 1000; // msecs
        mUpdateInterval = server.updateInterval * 1000; // msecs
        mBackgroundUpdateInterval = server.backgroundUpdateInterval * 1000; // msecs
        mUpdateTimer->setInterval(mUpdateInterval);

        if (wasConnected) {
            connect();
        }
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
    }

    void Rpc::connect()
    {
        if (!mServerUrl.isEmpty()) {
            setError(NoError);
            setStatus(Connecting);
            getServerSettings();
        }
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
            const auto future = QtConcurrent::run([=]() mutable {
                return makeRequestData(QLatin1String("torrent-add"),
                                       {{QLatin1String("metainfo"), fileData.toBase64()},
                                        {QLatin1String("download-dir"), std::move(downloadDirectory)},
                                        {QLatin1String("files-wanted"), std::move(wantedFiles)},
                                        {QLatin1String("files-unwanted"), std::move(unwantedFiles)},
                                        {QLatin1String("priority-high"), std::move(highPriorityFiles)},
                                        {QLatin1String("priority-normal"), std::move(normalPriorityFiles)},
                                        {QLatin1String("priority-low"), std::move(lowPriorityFiles)},
                                        {QLatin1String("bandwidthPriority"), std::move(bandwidthPriority)},
                                        {QLatin1String("paused"), !start}});
            });
            auto watcher = new QFutureWatcher<QByteArray>(this);
            QObject::connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [=]() {
                if (isConnected()) {
                    postRequest(watcher->result(), [=](const QJsonObject& parseResult) {
                        if (isResultSuccessful(parseResult)) {
                            if (getReplyArguments(parseResult).contains(QLatin1String("torrent-duplicate"))) {
                                emit torrentAddDuplicate();
                            } else {
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
                    [=](const QJsonObject& parseResult) {
                        if (isResultSuccessful(parseResult)) {
                            if (getReplyArguments(parseResult).contains(QLatin1String("torrent-duplicate"))) {
                                emit torrentAddDuplicate();
                            } else {
                                updateData();
                            }
                        } else {
                            emit torrentAddError();
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

    void Rpc::reannounceTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-reannounce"),
                                        {{QLatin1String("ids"), ids}}));
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

    void Rpc::setTorrentProperty(int id, const QString& property, const QVariant& value, bool updateIfSuccessful)
    {
        if (isConnected()) {
           QByteArray requestData(makeRequestData(QLatin1String("torrent-set"),
                                                  {{QLatin1String("ids"), QVariantList{id}},
                                                   {property, value}}));

           if (updateIfSuccessful) {
               postRequest(QByteArray(std::move(requestData)), [=](const QJsonObject& parseResult) {
                   if (isResultSuccessful(parseResult)) {
                       updateData();
                   }
               });
           } else {
               postRequest(QByteArray(std::move(requestData)));
           }
        }
    }

    void Rpc::setTorrentsLocation(const QVariantList& ids, const QString& location, bool moveFiles)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("torrent-set-location"),
                                        {{QLatin1String("ids"), ids},
                                         {QLatin1String("location"), location},
                                         {QLatin1String("move"), moveFiles}}),
                        [=](const QJsonObject& parseResult) {
                if (isResultSuccessful(parseResult)) {
                    updateData();
                }
            });
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
                    [=](const QJsonObject& parseResult) {
                        const QJsonArray torrentsVariants(getReplyArguments(parseResult)
                                                                .value(QLatin1String("torrents"))
                                                                .toArray());
                        const std::shared_ptr<Torrent> torrent(torrentById(id));
                        if (!torrentsVariants.isEmpty() && torrent) {
                            torrent->updateFiles(torrentsVariants.first().toObject());
                            emit gotTorrentFiles(id);
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
                    [=](const QJsonObject& parseResult) {
                        const QJsonArray torrentsVariants(getReplyArguments(parseResult)
                                                                .value(QLatin1String("torrents"))
                                                                .toArray());
                        const std::shared_ptr<Torrent> torrent(torrentById(id));
                        if (!torrentsVariants.isEmpty() && torrent) {
                            torrent->updatePeers(torrentsVariants.first().toObject());
                            emit gotTorrentPeers(id);
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
                        [=](const QJsonObject& parseResult) {
                            const std::shared_ptr<Torrent> torrent(torrentById(torrentId));
                            if (torrent) {
                                const QJsonObject arguments(getReplyArguments(parseResult));
                                const QString path(arguments.value(QLatin1String("path")).toString());
                                const QString newName(arguments.value(QLatin1String("name")).toString());
                                emit torrent->fileRenamed(path, newName);
                                emit torrentFileRenamed(torrentId, path, newName);
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
                        [=](const QJsonObject& parseResult) {
                            emit gotDownloadDirFreeSpace(getReplyArguments(parseResult).value(QLatin1String("download-dir-free-space")).toDouble());
                        });
        }
    }

    void Rpc::getFreeSpaceForPath(const QString& path)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QLatin1String("free-space"),
                                        {{QLatin1String("path"), path}}),
                        [=](const QJsonObject& parseResult) {
                            emit gotFreeSpaceForPath(path,
                                                     isResultSuccessful(parseResult),
                                                     getReplyArguments(parseResult).value(QLatin1String("size-bytes")).toDouble());
                        });
        }
    }

    void Rpc::setStatus(Status status)
    {
        if (status == mStatus) {
            return;
        }

        const bool wasConnected = isConnected();

        if (wasConnected && (status == Disconnected)) {
            emit aboutToDisconnect();
        }

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
            mServerSettingsUpdated = false;
            mTorrentsUpdated = false;
            mServerStatsUpdated = false;
            mTorrents.clear();
            emit connectedChanged();
            emit torrentsUpdated();
            mUpdateTimer->stop();
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

    void Rpc::getServerSettings()
    {
        postRequest(QByteArrayLiteral("{\"method\": \"session-get\"}"),
                    [=](const QJsonObject& parseResult) {
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
                                      "            \"priorities\","
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
                    [=](const QJsonObject& parseResult) {
                        const QJsonArray torrentsVariants(getReplyArguments(parseResult)
                                                                .value(QLatin1String("torrents"))
                                                                .toArray());

                        std::vector<std::shared_ptr<Torrent>> torrents;
                        torrents.reserve(torrentsVariants.size());
                        for (const QJsonValue& torrentVariant : torrentsVariants) {
                            const QJsonObject torrentMap(torrentVariant.toObject());
                            const int id = torrentMap.value(Torrent::idKey).toInt();

                            std::shared_ptr<Torrent> torrent(torrentById(id));
                            if (torrent) {
                                const bool wasFinished = (torrent->isFinished());
                                torrent->update(torrentMap);
                                const bool finished = (torrent->isFinished());

                                if (finished && !wasFinished) {
                                    emit torrentFinished(torrent.get());
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

                                if (isConnected()) {
                                    emit torrentAdded(torrent.get());
                                }
                            }
                            torrents.push_back(std::move(torrent));
                        }
                        mTorrents = std::move(torrents);

                        checkIfTorrentsUpdated();
                        startUpdateTimer();
                    });
    }

    void Rpc::getServerStats()
    {
        postRequest(QByteArrayLiteral("{\"method\": \"session-stats\"}"),
                    [=](const QJsonObject& parseResult) {
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
            if (!mUpdateDisabled) {
                mUpdateTimer->start();
            }
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
                          const std::function<void(const QJsonObject&)>& callOnSuccessParse,
                          const std::function<void()>& callOnSuccess)
    {
        QNetworkRequest request(mServerUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
        request.setRawHeader(sessionIdHeader, mSessionId);
        request.setSslConfiguration(mSslConfiguration);

        QNetworkReply* reply = mNetwork->post(request, data);
        reply->ignoreSslErrors(mExpectedSslErrors);

        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Disconnected) {
                switch (reply->error()) {
                case QNetworkReply::NoError:
                    if (callOnSuccessParse) {
                        const QByteArray replyData(reply->readAll());
                        const auto future = QtConcurrent::run([=]() {
                            QJsonParseError error;
                            QJsonObject result(QJsonDocument::fromJson(replyData, &error).object());
                            return std::pair<QJsonObject, bool>(std::move(result), error.error == QJsonParseError::NoError);
                        });
                        auto watcher = new QFutureWatcher<std::pair<QJsonObject, bool>>(this);
                        QObject::connect(watcher, &QFutureWatcher<std::pair<QJsonObject, bool>>::finished, this, [=]() {
                            const auto result = watcher->result();
                            if (mStatus != Disconnected) {
                                if (result.second) {
                                    callOnSuccessParse(result.first);
                                } else {
                                    qWarning() << "parsing error";
                                    setError(ParseError);
                                    setStatus(Disconnected);
                                }
                            }
                            watcher->deleteLater();
                        });
                        watcher->setFuture(future);
                    } else if (callOnSuccess) {
                        callOnSuccess();
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
                        postRequest(data, callOnSuccessParse);
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

    void Rpc::postRequest(const QByteArray& data, const std::function<void(const QJsonObject&)>& callOnSuccess)
    {
        postRequest(data, callOnSuccess, nullptr);
    }

    void Rpc::postRequest(const QByteArray& data, const std::function<void()>& callOnSuccess)
    {
        postRequest(data, nullptr, callOnSuccess);
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
