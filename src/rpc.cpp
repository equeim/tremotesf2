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

#include "rpc.h"

#include <QCoreApplication>
#include <QDebug>
#include <QAuthenticator>
#include <QFile>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QTimer>
#include <QSslCertificate>
#include <QSslKey>

#ifdef TREMOTESF_SAILFISHOS
#include <QQmlEngine>
#endif

#include "accounts.h"
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

        QByteArray makeRequestData(const QByteArray& method, const QVariantMap& arguments)
        {
            return QJsonDocument::fromVariant(QVariantMap {
                                                  {
                                                      "method",
                                                      method
                                                  },
                                                  {
                                                      "arguments",
                                                      arguments
                                                  }
                                              }).toJson();
        }

        QVariantMap getReplyArguments(const QVariant& result)
        {
            return result.toMap().value("arguments").toMap();
        }
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

        QObject::connect(Accounts::instance(), &Accounts::currentAccountChanged, this, &Rpc::updateAccount);
        updateAccount();
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
            return qApp->translate("tremotesf", "Connecting");
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

    void Rpc::addTorrentFile(const QString& filePath,
                             const QString& downloadDirectory,
                             const QVariantList& wantedFiles,
                             const QVariantList& unwantedFiles,
                             const QVariantList& highPriorityFiles,
                             const QVariantList& normalPriorityFiles,
                             const QVariantList& lowPriorityFiles,
                             int bandwidthPriority,
                             bool start)
    {
        if (!isConnected()) {
            return;
        }

        QFile file(filePath);
        if (file.open(QFile::ReadOnly)) {
            postRequest(makeRequestData("torrent-add", {{"metainfo", file.readAll().toBase64()},
                                                        {"download-dir", downloadDirectory},
                                                        {"files-wanted", wantedFiles},
                                                        {"files-unwanted", unwantedFiles},
                                                        {"priority-high", highPriorityFiles},
                                                        {"priority-normal", normalPriorityFiles},
                                                        {"priority-low", lowPriorityFiles},
                                                        {"bandwidthPriority", bandwidthPriority},
                                                        {"paused", !start}}),
                        [=]() {
                            addTorrentFile(filePath,
                                           downloadDirectory,
                                           wantedFiles,
                                           unwantedFiles,
                                           highPriorityFiles,
                                           normalPriorityFiles,
                                           lowPriorityFiles,
                                           bandwidthPriority,
                                           start);
                        });
        } else {
            qDebug() << "Error reading torrent file:" << file.errorString();
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

        postRequest(makeRequestData("torrent-add", {{"filename", link},
                                                    {"download-dir", downloadDirectory},
                                                    {"bandwidthPriority", bandwidthPriority},
                                                    {"paused", !start}}),
                    [=]() {
                        addTorrentLink(link,
                                       downloadDirectory,
                                       bandwidthPriority,
                                       start);
                    });
    }


    void Rpc::startTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("torrent-start", {{"ids", ids}}), [=]() {
                startTorrents(ids);
            });
        }
    }

    void Rpc::startTorrentsNow(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("torrent-start-now", {{"ids", ids}}), [=]() {
                startTorrentsNow(ids);
            });
        }
    }

    void Rpc::pauseTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("torrent-stop", {{"ids", ids}}), [=]() {
                pauseTorrents(ids);
            });
        }
    }

    void Rpc::removeTorrents(const QVariantList& ids, bool deleteFiles)
    {
        if (isConnected()) {
            postRequest(makeRequestData("torrent-remove", {{"ids", ids},
                                                           {"delete-local-data", deleteFiles}}),
                        [=]() {
                removeTorrents(ids, deleteFiles);
            });
        }
    }

    void Rpc::checkTorrents(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("torrent-verify", {{"ids", ids}}), [=]() {
                checkTorrents(ids);
            });
        }
    }

    void Rpc::moveTorrentsToTop(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("queue-move-top", {{"ids", ids}}), [=]() {
                moveTorrentsToTop(ids);
            });
        }
    }

    void Rpc::moveTorrentsUp(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("queue-move-up", {{"ids", ids}}), [=]() {
                moveTorrentsUp(ids);
            });
        }
    }

    void Rpc::moveTorrentsDown(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("queue-move-down", {{"ids", ids}}), [=]() {
                moveTorrentsDown(ids);
            });
        }
    }

    void Rpc::moveTorrentsToBottom(const QVariantList& ids)
    {
        if (isConnected()) {
            postRequest(makeRequestData("queue-move-bottom", {{"ids", ids}}), [=]() {
                moveTorrentsToBottom(ids);
            });
        }
    }

    void Rpc::setSessionProperty(const QString& property, const QVariant& value)
    {
        if (isConnected()) {
            postRequest(makeRequestData("session-set", {{property, value}}), [=]() {
                setSessionProperty(property, value);
            });
        }
    }

    void Rpc::setSessionProperties(const QVariantMap& properties)
    {
        if (isConnected()) {
            postRequest(makeRequestData("session-set", properties), [=]() {
                setSessionProperties(properties);
            });
        }
    }

    void Rpc::setTorrentProperty(int id, const QString& property, const QVariant& value)
    {
        if (isConnected()) {
            postRequest(makeRequestData("torrent-set", {{"ids", QVariantList {id}},
                                                        {property, value}}),
                        [=]() {
                            setTorrentProperty(id, property, value);
                        });
        }
    }

    void Rpc::getTorrentFiles(int id)
    {
        QNetworkReply* reply = postRequest(QString("{"
                                                   "    \"arguments\": {"
                                                   "        \"fields\": ["
                                                   "            \"files\","
                                                   "            \"fileStats\""
                                                   "        ],"
                                                   "        \"ids\": [%1]"
                                                   "    },"
                                                   "    \"method\": \"torrent-get\""
                                                   "}").arg(id).toLatin1());
        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Status::Disconnected) {
                if (!checkSessionId(reply)) {
                    getTorrentFiles(id);
                } else if (checkReplyError(reply)) {
                    const QVariant parseResult(QJsonDocument::fromJson(reply->readAll()).toVariant());
                    if (checkParseResult(parseResult)) {
                        const QVariantList torrentsVariants(getReplyArguments(parseResult).value("torrents").toList());
                        const std::shared_ptr<Torrent> torrent(torrentById(id));
                        if (!torrentsVariants.isEmpty() && torrent) {
                            torrent->updateFiles(torrentsVariants.first().toMap());
                            checkIfTorrentsUpdated();
                            startUpdateTimer();
                        }
                    }
                }
            }
            reply->deleteLater();
        });
    }

    void Rpc::getTorrentPeers(int id)
    {
        QNetworkReply* reply = postRequest(QString("{"
                                                   "    \"arguments\": {"
                                                   "        \"fields\": [\"peers\"],"
                                                   "        \"ids\": [%1]"
                                                   "    },"
                                                   "    \"method\": \"torrent-get\""
                                                   "}").arg(id).toLatin1());
        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Status::Disconnected) {
                if (!checkSessionId(reply)) {
                    getTorrentPeers(id);
                } else if (checkReplyError(reply)) {
                    const QVariant parseResult(QJsonDocument::fromJson(reply->readAll()).toVariant());
                    if (checkParseResult(parseResult)) {
                        const QVariantList torrentsVariants(getReplyArguments(parseResult).value("torrents").toList());
                        const std::shared_ptr<Torrent> torrent(torrentById(id));
                        if (!torrentsVariants.isEmpty() && torrent) {
                            torrent->updatePeers(torrentsVariants.first().toMap());
                            checkIfTorrentsUpdated();
                            startUpdateTimer();
                        }
                    }
                }
            }
            reply->deleteLater();
        });
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
        } else if (wasConnected) {
            mNetwork->clearAccessCache();
            mAuthenticationRequested = false;
            mRpcVersionChecked = false;
            mFirstUpdate = true;
            mTorrents.clear();
            emit torrentsUpdated();
            emit connectedChanged();
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

    void Rpc::updateAccount()
    {
        if (!Accounts::instance()->hasAccounts()) {
            disconnect();
            return;
        }

        const bool wasConnected = (mStatus != Disconnected);

        disconnect();

        const Account account(Accounts::instance()->currentAccount());

        mServerUrl.setHost(account.address);
        mServerUrl.setPort(account.port);
        mServerUrl.setPath(account.apiPath);
        if (account.https)
            mServerUrl.setScheme("https");
        else
            mServerUrl.setScheme("http");

        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        mSslConfiguration.setPeerVerifyMode(QSslSocket::QueryPeer);
        const QByteArray localCertificate(account.localCertificate);
        mSslConfiguration.setLocalCertificate(QSslCertificate(localCertificate));
        mSslConfiguration.setPrivateKey(QSslKey(localCertificate, QSsl::Rsa));

        mAuthentication = account.authentication;
        mUsername = account.username;
        mPassword = account.password;
        mTimeout = account.timeout * 1000; // msecs
        mUpdateTimer->setInterval(account.updateInterval * 1000); // msecs

        if (wasConnected) {
            connect();
        }
    }

    void Rpc::getServerSettings()
    {
        QNetworkReply* reply = postRequest("{\"method\": \"session-get\"}");

        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Status::Disconnected) {
                if (!checkSessionId(reply)) {
                    getServerSettings();
                } else if (checkReplyError(reply)) {
                    const QVariant parseResult(QJsonDocument::fromJson(reply->readAll()).toVariant());
                    if (checkParseResult(parseResult)) {
                        mServerSettings->update(getReplyArguments(parseResult));
                        if (!mRpcVersionChecked) {
                            mRpcVersionChecked = true;
                            if (mServerSettings->minimumRpcVersion() > minimumRpcVersion) {
                                setError(ServerIsTooNew);
                                setStatus(Disconnected);
                            } else if (mServerSettings->rpcVersion() < minimumRpcVersion) {
                                setError(ServerIsTooOld);
                                setStatus(Disconnected);
                            } else {
                                setStatus(Connected);
                                getTorrents();
                                getServerStats();
                            }
                        }
                        mServerSettingsUpdated = true;
                        startUpdateTimer();
                    }
                }
            }
            reply->deleteLater();
        });
    }

    void Rpc::getTorrents()
    {
        QNetworkReply* reply = postRequest("{"
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
                                           "}");
        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Status::Disconnected) {
                if (!checkSessionId(reply)) {
                    getTorrents();
                } else if (checkReplyError(reply)) {
                    const QVariant parseResult(QJsonDocument::fromJson(reply->readAll()).toVariant());
                    if (checkParseResult(parseResult)) {
                        const QVariantList torrentsVariants(getReplyArguments(parseResult).value("torrents").toList());

                        QList<std::shared_ptr<Torrent>> torrents;
                        for (const QVariant& torrentVariant : torrentsVariants) {
                            const QVariantMap torrentMap(torrentVariant.toMap());
                            const int id = torrentMap.value("id").toInt();

                            std::shared_ptr<Torrent> torrent(torrentById(id));
                            if (torrent) {
                                const bool wasFinished = (torrent->percentDone() == 1);
                                torrent->update(torrentMap);
                                const bool finished = (torrent->percentDone() == 1);

                                if (finished && !wasFinished) {
                                    emit torrentFinished(torrent->name());
                                }

                                if (torrent->isFilesEnabled()) {
                                    getTorrentFiles(id);
                                }
                                if (torrent->isPeersEnabled()) {
                                    getTorrentPeers(id);
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
                    }
                }
            }
            reply->deleteLater();
        });
    }

    void Rpc::getServerStats()
    {
        QNetworkReply* reply = postRequest("{\"method\": \"session-stats\"}");
        QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
            if (mStatus != Status::Disconnected) {
                if (!checkSessionId(reply)) {
                    getServerStats();
                } else if (checkReplyError(reply)) {
                    const QVariant parseResult(QJsonDocument::fromJson(reply->readAll()).toVariant());
                    if (checkParseResult(parseResult)) {
                        mServerStats->update(getReplyArguments(parseResult));
                        mServerStatsUpdated = true;
                        startUpdateTimer();
                    }
                }
            }
            reply->deleteLater();
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
        emit torrentsUpdated();
    }

    void Rpc::startUpdateTimer()
    {
        if (mServerSettingsUpdated && mTorrentsUpdated && mServerStatsUpdated) {
            mUpdateTimer->start();
        }
    }

    void Rpc::updateData()
    {
        mServerSettingsUpdated = false;
        mTorrentsUpdated = false;
        mServerStatsUpdated = false;

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

    QNetworkReply* Rpc::postRequest(const QByteArray& data, const std::function<void()>& callAfterNewId)
    {
        QNetworkRequest request(mServerUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader(sessionIdHeader, mSessionId);
        request.setSslConfiguration(mSslConfiguration);

        QNetworkReply* reply = mNetwork->post(request, data);

        if (callAfterNewId) {
            QObject::connect(reply, &QNetworkReply::finished, this, [=]() {
                if (mStatus != Status::Disconnected) {
                    if (!checkSessionId(reply)) {
                        callAfterNewId();
                    } else {
                        checkReplyError(reply);
                    }
                }
                reply->deleteLater();
            });
        }

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

        return reply;
    }

    bool Rpc::checkSessionId(const QNetworkReply* reply)
    {
        // 409 Conflict
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409 &&
                reply->hasRawHeader(sessionIdHeader)) {
            mSessionId = reply->rawHeader(sessionIdHeader);
            return false;
        }
        return true;
    }

    bool Rpc::checkReplyError(const QNetworkReply* reply)
    {
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
            if (reply->error() == QNetworkReply::OperationCanceledError) {
                setError(TimedOut);
            } else {
                qDebug() << reply->errorString();
                setError(ConnectionError);
            }
            setStatus(Status::Disconnected);
            return false;
        }
        return true;
    }

    bool Rpc::checkParseResult(const QVariant& result)
    {
        if (result.isValid()) {
            return true;
        }
        setError(ParseError);
        setStatus(Disconnected);
        return false;
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
