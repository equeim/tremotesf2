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

#ifndef LIBTREMOTESF_RPC_H
#define LIBTREMOTESF_RPC_H

#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <QByteArray>
#include <QFile>
#include <QNetworkRequest>
#include <QObject>
#include <QSslConfiguration>
#include <QUrl>
#include <QVariantList>

#include "serversettings.h"
#include "serverstats.h"
#include "qtsupport.h"

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace libtremotesf
{
    class Torrent;

    struct Server
    {
        Q_GADGET
    public:
        enum class ProxyType {
            Default,
            Http,
            Socks5
        };
        Q_ENUM(ProxyType)

        QString name;

        QString address;
        int port;
        QString apiPath;

        ProxyType proxyType;
        QString proxyHostname;
        int proxyPort;
        QString proxyUser;
        QString proxyPassword;

        bool https;
        bool selfSignedCertificateEnabled;
        QByteArray selfSignedCertificate;
        bool clientCertificateEnabled;
        QByteArray clientCertificate;

        bool authentication;
        QString username;
        QString password;

        int updateInterval;
        int backgroundUpdateInterval;
        int timeout;
    };

    DEFINE_Q_ENUM_NS(RpcConnectionState,

                     Disconnected,
                     Connecting,
                     Connected)

    DEFINE_Q_ENUM_NS(RpcError,

                     NoError,
                     TimedOut,
                     ConnectionError,
                     AuthenticationError,
                     ParseError,
                     ServerIsTooNew,
                     ServerIsTooOld)

    class Rpc : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(libtremotesf::ServerSettings* serverSettings READ serverSettings CONSTANT)
        Q_PROPERTY(libtremotesf::ServerStats* serverStats READ serverStats CONSTANT)
        Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
        Q_PROPERTY(libtremotesf::RpcConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
        Q_PROPERTY(libtremotesf::RpcError error READ error NOTIFY errorChanged)
        Q_PROPERTY(bool local READ isLocal NOTIFY connectedChanged)
        Q_PROPERTY(int torrentsCount READ torrentsCount NOTIFY torrentsUpdated)
        Q_PROPERTY(bool backgroundUpdate READ backgroundUpdate WRITE setBackgroundUpdate NOTIFY backgroundUpdateChanged)
        Q_PROPERTY(bool updateDisabled READ isUpdateDisabled WRITE setUpdateDisabled NOTIFY updateDisabledChanged)
    public:
        using ConnectionState = RpcConnectionState;
        using Error = RpcError;

        explicit Rpc(QObject* parent = nullptr);

        ServerSettings* serverSettings() const;
        ServerStats* serverStats() const;

        const std::vector<std::shared_ptr<Torrent>>& torrents() const;
        Q_INVOKABLE libtremotesf::Torrent* torrentByHash(const QString& hash) const;
        Torrent* torrentById(int id) const;

        Q_INVOKABLE void setAutoReconnect(bool enabled);
        bool isConnected() const;
        ConnectionState connectionState() const;
        Error error() const;
        const QString& errorMessage() const;
        bool isLocal() const;

        int torrentsCount() const;

        bool backgroundUpdate() const;
        Q_INVOKABLE void setBackgroundUpdate(bool background);

        bool isUpdateDisabled() const;
        Q_INVOKABLE void setUpdateDisabled(bool disabled);

        Q_INVOKABLE void setServer(const libtremotesf::Server& server);
        Q_INVOKABLE void resetServer();

        Q_INVOKABLE void connect();
        Q_INVOKABLE void disconnect();

        Q_INVOKABLE void addTorrentFile(const QString& filePath,
                                        const QString& downloadDirectory,
                                        const QVariantList& unwantedFiles,
                                        const QVariantList& highPriorityFiles,
                                        const QVariantList& lowPriorityFiles,
                                        const QVariantMap& renamedFiles,
                                        int bandwidthPriority,
                                        bool start);

        Q_INVOKABLE void addTorrentFile(std::shared_ptr<QFile> file,
                                        const QString& downloadDirectory,
                                        const QVariantList& unwantedFiles,
                                        const QVariantList& highPriorityFiles,
                                        const QVariantList& lowPriorityFiles,
                                        const QVariantMap& renamedFiles,
                                        int bandwidthPriority,
                                        bool start);

        Q_INVOKABLE void addTorrentLink(const QString& link,
                                        const QString& downloadDirectory,
                                        int bandwidthPriority,
                                        bool start);

        Q_INVOKABLE void startTorrents(const QVariantList& ids);
        Q_INVOKABLE void startTorrentsNow(const QVariantList& ids);
        Q_INVOKABLE void pauseTorrents(const QVariantList& ids);
        Q_INVOKABLE void removeTorrents(const QVariantList& ids, bool deleteFiles);
        Q_INVOKABLE void checkTorrents(const QVariantList& ids);
        Q_INVOKABLE void moveTorrentsToTop(const QVariantList& ids);
        Q_INVOKABLE void moveTorrentsUp(const QVariantList& ids);
        Q_INVOKABLE void moveTorrentsDown(const QVariantList& ids);
        Q_INVOKABLE void moveTorrentsToBottom(const QVariantList& ids);

        Q_INVOKABLE void reannounceTorrents(const QVariantList& ids);

        void setSessionProperty(const QString& property, const QVariant& value);
        void setSessionProperties(const QVariantMap& properties);
        void setTorrentProperty(int id, const QString& property, const QVariant& value, bool updateIfSuccessful = false);
        Q_INVOKABLE void setTorrentsLocation(const QVariantList& ids, const QString& location, bool moveFiles);
        void getTorrentsFiles(const QVariantList& ids, bool scheduled);
        void getTorrentsPeers(const QVariantList& ids, bool scheduled);

        Q_INVOKABLE void renameTorrentFile(int torrentId,
                                           const QString& filePath,
                                           const QString& newName);

        Q_INVOKABLE void getDownloadDirFreeSpace();
        Q_INVOKABLE void getFreeSpaceForPath(const QString& path);

        Q_INVOKABLE void updateData();

        Q_INVOKABLE void shutdownServer();

    private:
        struct Request
        {
            QLatin1String method;
            QNetworkRequest request;
            QByteArray data;
            std::function<void(const QJsonObject&, bool)> callOnSuccessParse;

            void setSessionId(const QByteArray& sessionId);
        };

        void setConnectionState(ConnectionState state);
        void setError(Error error, const QString& errorMessage = QString());

        void getServerSettings();
        void getTorrents();
        void checkTorrentsSingleFile(const QVariantList& torrentIds);
        void getServerStats();

        void checkIfTorrentsUpdated();
        void startUpdateTimer();

        void onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator);

        QNetworkReply* postRequest(Request&& request);

        bool retryRequest(Request&& request,
                          QNetworkReply* previousAttempt);

        void postRequest(QLatin1String method,
                         const QByteArray& data,
                         const std::function<void(const QJsonObject&, bool)>& callOnSuccessParse = {});

        void postRequest(QLatin1String method,
                         const QVariantMap& arguments,
                         const std::function<void(const QJsonObject&, bool)>& callOnSuccessParse = {});

        bool isSessionIdFileExists() const;

        QNetworkAccessManager* mNetwork;
        std::unordered_set<QNetworkReply*> mActiveNetworkRequests;
        std::unordered_map<QNetworkReply*, int> mRetryingNetworkRequests;

        bool mAuthenticationRequested;
        QByteArray mSessionId;

        bool mBackgroundUpdate;
        bool mUpdateDisabled;
        bool mUpdating;

        QUrl mServerUrl;
        bool mAuthentication;
        QSslConfiguration mSslConfiguration;
        QList<QSslError> mExpectedSslErrors;
        QString mUsername;
        QString mPassword;
        int mUpdateInterval;
        int mBackgroundUpdateInterval;
        int mTimeout;
        bool mLocal;

        bool mRpcVersionChecked;
        bool mServerSettingsUpdated;
        bool mTorrentsUpdated;
        bool mServerStatsUpdated;
        QTimer* mUpdateTimer;

        bool mAutoReconnectEnabled;
        QTimer* mAutoReconnectTimer;

        ServerSettings* mServerSettings;
        std::vector<std::shared_ptr<Torrent>> mTorrents;
        ServerStats* mServerStats;

        ConnectionState mConnectionState;
        Error mError;
        QString mErrorMessage;

    signals:
        void aboutToDisconnect();
        void connectedChanged();
        void connectionStateChanged();
        void errorChanged();

        void torrentsUpdated(const std::vector<int>& removed, const std::vector<int>& changed, int added);

        void torrentFilesUpdated(const libtremotesf::Torrent* torrent, const std::vector<int>& changed);
        void torrentPeersUpdated(const libtremotesf::Torrent* torrent,
                                 const std::vector<int>& removed,
                                 const std::vector<int>& changed,
                                 int added);

        void torrentFileRenamed(int torrentId, const QString& filePath, const QString& newName);

        void torrentAdded(libtremotesf::Torrent* torrent);
        void torrentFinished(libtremotesf::Torrent* torrent);

        void torrentAddDuplicate();
        void torrentAddError();

        void gotDownloadDirFreeSpace(long long bytes);
        void gotFreeSpaceForPath(const QString& path, bool success, long long bytes);

        void backgroundUpdateChanged();
        void updateDisabledChanged();
    };
}

#endif // LIBTREMOTESF_RPC_H
