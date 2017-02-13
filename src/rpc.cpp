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

#include <QCoreApplication>
#include <QDebug>
#include <QAuthenticator>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QTimer>
#include <QThread>
#include <QSslCertificate>
#include <QSslKey>

#ifdef TREMOTESF_SAILFISHOS
#include <QQmlEngine>
#endif

#include "jsonparser.h"
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

        const QByteArray sessionIdHeader(QByteArrayLiteral("X-Transmission-Session-Id"));

        QByteArray makeRequestData(const QString& method, const QVariantMap& arguments)
        {
            return QJsonDocument::fromVariant(QVariantMap{{QStringLiteral("method"), method},
                                                          {QStringLiteral("arguments"), arguments}})
                .toJson();
        }

        QVariantMap getReplyArguments(const QVariantMap& parseResult)
        {
            return parseResult.value(QStringLiteral("arguments")).toMap();
        }

        bool isResultSuccessful(const QVariantMap& parseResult)
        {
            return (parseResult.value(QStringLiteral("result")).toString() == "success");
        }

        class AddTorrentFileWorker : public QObject
        {
            Q_OBJECT
        public:
            void encode(const QByteArray& fileData,
                        const QString& downloadDirectory,
                        const QVariantList& wantedFiles,
                        const QVariantList& unwantedFiles,
                        const QVariantList& highPriorityFiles,
                        const QVariantList& normalPriorityFiles,
                        const QVariantList& lowPriorityFiles,
                        int bandwidthPriority,
                        bool start)
            {
                emit done(makeRequestData(QStringLiteral("torrent-add"),
                                          {{QStringLiteral("metainfo"), fileData.toBase64()},
                                           {QStringLiteral("download-dir"), downloadDirectory},
                                           {QStringLiteral("files-wanted"), wantedFiles},
                                           {QStringLiteral("files-unwanted"), unwantedFiles},
                                           {QStringLiteral("priority-high"), highPriorityFiles},
                                           {QStringLiteral("priority-normal"), normalPriorityFiles},
                                           {QStringLiteral("priority-low"), lowPriorityFiles},
                                           {QStringLiteral("bandwidthPriority"), bandwidthPriority},
                                           {QStringLiteral("paused"), !start}}));
            }
        signals:
            void done(const QByteArray& requestData);
        };

        class AddTorrentFile : public QObject
        {
            Q_OBJECT
        public:
            explicit AddTorrentFile(const QByteArray& fileData,
                                    const QString& downloadDirectory,
                                    const QVariantList& wantedFiles,
                                    const QVariantList& unwantedFiles,
                                    const QVariantList& highPriorityFiles,
                                    const QVariantList& normalPriorityFiles,
                                    const QVariantList& lowPriorityFiles,
                                    int bandwidthPriority,
                                    bool start,
                                    QObject* parent)
                : QObject(parent),
                  mWorkerThread(new QThread(this))
            {
                auto worker = new AddTorrentFileWorker();
                worker->moveToThread(mWorkerThread);
                QObject::connect(mWorkerThread, &QThread::finished, worker, &QObject::deleteLater);
                QObject::connect(this, &AddTorrentFile::requestEncode, worker, &AddTorrentFileWorker::encode);
                QObject::connect(worker, &AddTorrentFileWorker::done, this, &AddTorrentFile::done);
                mWorkerThread->start();
                emit requestEncode(fileData,
                                   downloadDirectory,
                                   wantedFiles,
                                   unwantedFiles,
                                   highPriorityFiles,
                                   normalPriorityFiles,
                                   lowPriorityFiles,
                                   bandwidthPriority,
                                   start);
            }

            ~AddTorrentFile() override
            {
                mWorkerThread->quit();
                mWorkerThread->wait();
            }

        private:
            QThread* mWorkerThread;
        signals:
            void requestEncode(const QByteArray& fileData,
                               const QString& downloadDirectory,
                               const QVariantList& wantedFiles,
                               const QVariantList& unwantedFiles,
                               const QVariantList& highPriorityFiles,
                               const QVariantList& normalPriorityFiles,
                               const QVariantList& lowPriorityFiles,
                               int bandwidthPriority,
                               bool start);
            void done(const QByteArray& requestData);
        };
    }

    Rpc::Rpc(QObject* parent)
        : QObject(parent),
          mNetwork(new QNetworkAccessManager(this)),
          mAuthenticationRequested(false),
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

    const QList<std::shared_ptr<Torrent>>& Rpc::torrents() const
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
        if (hostName == "localhost" ||
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
            auto add = new AddTorrentFile(fileData,
                                          downloadDirectory,
                                          wantedFiles,
                                          unwantedFiles,
                                          highPriorityFiles,
                                          normalPriorityFiles,
                                          lowPriorityFiles,
                                          bandwidthPriority,
                                          start,
                                          this);
            QObject::connect(add, &AddTorrentFile::done, this, [=](const QByteArray& requestData) {
                if (isConnected()) {
                    postRequest(requestData, [=](const QVariantMap& parseResult) {
                        if (getReplyArguments(parseResult).contains(QStringLiteral("torrent-added"))) {
                            updateData();
                        }
                    });
                }
                add->deleteLater();
            });
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

        postRequest(makeRequestData(QStringLiteral("torrent-add"),
                                    {{QStringLiteral("filename"), link},
                                     {QStringLiteral("download-dir"), downloadDirectory},
                                     {QStringLiteral("bandwidthPriority"), bandwidthPriority},
                                     {QStringLiteral("paused"), !start}}),
                    [=](const QVariantMap& parseResult) {
                        if (getReplyArguments(parseResult).contains(QStringLiteral("torrent-added"))) {
                            updateData();
                        }
                    });
    }

    void Rpc::startTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("torrent-start"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::startTorrentsNow(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("torrent-start-now"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::pauseTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("torrent-stop"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::removeTorrents(const QVariantList& ids, bool deleteFiles)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("torrent-remove"),
                                        {{QStringLiteral("ids"), ids},
                                         {QStringLiteral("delete-local-data"), deleteFiles}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::checkTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("torrent-verify"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsToTop(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("queue-move-top"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsUp(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("queue-move-up"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsDown(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("queue-move-down"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::moveTorrentsToBottom(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("queue-move-bottom"),
                                        {{QStringLiteral("ids"), ids}}),
                        [=]() { updateData(); });
        }
    }

    void Rpc::setSessionProperty(const QString& property, const QVariant& value)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("session-set"), {{property, value}}));
        }
    }

    void Rpc::setSessionProperties(const QVariantMap& properties)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("session-set"), properties));
        }
    }

    void Rpc::setTorrentProperty(int id, const QString& property, const QVariant& value)
    {
        if (isConnected()) {
            postRequest(makeRequestData(QStringLiteral("torrent-set"),
                                        {{QStringLiteral("ids"), QVariantList{id}},
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
                                                                .value(QStringLiteral("torrents"))
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
                                                                .value(QStringLiteral("torrents"))
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
            postRequest(makeRequestData(QStringLiteral("torrent-rename-path"),
                                        {{QStringLiteral("ids"), QVariantList{torrentId}},
                                         {QStringLiteral("path"), filePath},
                                         {QStringLiteral("name"), newName}}),
                        [=](const QVariantMap& parseResult) {
                            const std::shared_ptr<Torrent> torrent(torrentById(torrentId));
                            if (torrent) {
                                const QVariantMap arguments(getReplyArguments(parseResult));
                                emit torrent->fileRenamed(arguments.value(QStringLiteral("path")).toString(),
                                                          arguments.value(QStringLiteral("name")).toString());
                                updateData();
                            }
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
            mServerUrl.setScheme(QStringLiteral("https"));
        } else {
            mServerUrl.setScheme(QStringLiteral("http"));
        }

        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        mSslConfiguration.setPeerVerifyMode(QSslSocket::QueryPeer);
        const QByteArray localCertificate(server.localCertificate);
        mSslConfiguration.setLocalCertificate(QSslCertificate(localCertificate));
        mSslConfiguration.setPrivateKey(QSslKey(localCertificate, QSsl::Rsa));

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
                                                                .value(QStringLiteral("torrents"))
                                                                .toList());

                        QList<std::shared_ptr<Torrent>> torrents;
                        for (const QVariant& torrentVariant : torrentsVariants) {
                            const QVariantMap torrentMap(torrentVariant.toMap());
                            const int id = torrentMap.value(QStringLiteral("id")).toInt();

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
                            torrents.append(torrent);
                        }
                        mTorrents = torrents;

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
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setRawHeader(sessionIdHeader, mSessionId);
        request.setSslConfiguration(mSslConfiguration);

        QNetworkReply* reply = mNetwork->post(request, data);

        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Disconnected) {
                if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409 &&
                    reply->hasRawHeader(sessionIdHeader)) {
                    mSessionId = reply->rawHeader(sessionIdHeader);
                    postRequest(data, callOnSuccess);
                } else if (reply->error() == QNetworkReply::NoError) {
                    if (callOnSuccess) {
                        auto parser = new JsonParser(reply->readAll(), this);
                        QObject::connect(parser,
                                         &JsonParser::done,
                                         [=](const QVariantMap& parseResult, bool success) {
                                             if (mStatus != Disconnected) {
                                                 if (success) {
                                                     callOnSuccess(parseResult);
                                                 } else {
                                                     qDebug() << "parsing error";
                                                     setError(ParseError);
                                                     setStatus(Disconnected);
                                                 }
                                             }
                                             parser->deleteLater();
                                         });
                    }
                } else {
                    if (reply->error() == QNetworkReply::OperationCanceledError) {
                        qDebug() << "timed out";
                        setError(TimedOut);
                    } else {
                        qDebug() << reply->error() << reply->errorString();
                        setError(ConnectionError);
                    }
                    setStatus(Disconnected);
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
        for (const std::shared_ptr<Torrent>& torrent : mTorrents) {
            if (torrent->id() == id) {
                return torrent;
            }
        }
        return std::shared_ptr<Torrent>();
    }
}

#include "rpc.moc"
