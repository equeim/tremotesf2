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
#include <unordered_set>

#include <QByteArray>
#include <QObject>
#include <QSslConfiguration>
#include <QUrl>
#include <QVariantList>

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace libtremotesf
{
    class ServerSettings;
    class ServerStats;
    class Torrent;

    struct Server
    {
        QString name;
        QString address;
        int port;
        QString apiPath;
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

    class Rpc : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(libtremotesf::ServerSettings* serverSettings READ serverSettings CONSTANT)
        Q_PROPERTY(libtremotesf::ServerStats* serverStats READ serverStats CONSTANT)
        Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
        Q_PROPERTY(Status status READ status NOTIFY statusChanged)
        Q_PROPERTY(Error error READ error NOTIFY errorChanged)
        Q_PROPERTY(bool local READ isLocal NOTIFY connectedChanged)
        Q_PROPERTY(int torrentsCount READ torrentsCount NOTIFY torrentsUpdated)
        Q_PROPERTY(bool backgroundUpdate READ backgroundUpdate WRITE setBackgroundUpdate NOTIFY backgroundUpdateChanged)
        Q_PROPERTY(bool updateDisabled READ isUpdateDisabled WRITE setUpdateDisabled NOTIFY updateDisabledChanged)
    public:
        enum Status
        {
            Disconnected,
            Connecting,
            Connected
        };
        Q_ENUM(Status)

        enum Error
        {
            NoError,
            TimedOut,
            ConnectionError,
            AuthenticationError,
            ParseError,
            ServerIsTooNew,
            ServerIsTooOld
        };
        Q_ENUM(Error)

        explicit Rpc(bool createServerSettings = true, QObject* parent = nullptr);

        ServerSettings* serverSettings() const;
        void setServerSettings(ServerSettings* settings);
        ServerStats* serverStats() const;

        const std::vector<std::shared_ptr<Torrent>>& torrents() const;
        Q_INVOKABLE libtremotesf::Torrent* torrentByHash(const QString& hash) const;
        std::shared_ptr<Torrent> torrentById(int id) const;

        bool isConnected() const;
        Status status() const;
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

        Q_INVOKABLE void addTorrentFile(const QByteArray& fileData,
                                        const QString& downloadDirectory,
                                        const QVariantList& wantedFiles,
                                        const QVariantList& unwantedFiles,
                                        const QVariantList& highPriorityFiles,
                                        const QVariantList& normalPriorityFiles,
                                        const QVariantList& lowPriorityFiles,
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
        Q_INVOKABLE void getTorrentFiles(int id, bool scheduled);
        Q_INVOKABLE void getTorrentPeers(int id, bool scheduled);

        Q_INVOKABLE void renameTorrentFile(int torrentId,
                                           const QString& filePath,
                                           const QString& newName);

        Q_INVOKABLE void getDownloadDirFreeSpace();
        Q_INVOKABLE void getFreeSpaceForPath(const QString& path);

    private:
        void setStatus(Status status);
        void setError(Error error, const QString& errorMessage = QString());

        void getServerSettings();
        void getTorrents();
        void getServerStats();

        void checkIfTorrentsUpdated();
        void startUpdateTimer();
        void updateData();

        void onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator);

        void postRequest(const QByteArray& data,
                         const std::function<void(const QJsonObject&)>& callOnSuccessParse,
                         const std::function<void()>& callOnSuccess);

        void postRequest(const QByteArray& data,
                         const std::function<void(const QJsonObject&)>& callOnSuccess);

        void postRequest(const QByteArray& data,
                         const std::function<void()>& callOnSuccess = nullptr);

        QNetworkAccessManager* mNetwork;
        std::unordered_set<QNetworkReply*> mNetworkRequests;

        bool mAuthenticationRequested;
        QByteArray mSessionId;

        bool mBackgroundUpdate;
        bool mUpdateDisabled;

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

        ServerSettings* mServerSettings;
        std::vector<std::shared_ptr<Torrent>> mTorrents;
        ServerStats* mServerStats;

        Status mStatus;
        Error mError;
        QString mErrorMessage;

    signals:
        void aboutToDisconnect();
        void connectedChanged();
        void statusChanged();
        void errorChanged();

        void torrentsUpdated();

        void torrentAdded(libtremotesf::Torrent* torrent);
        void torrentFinished(libtremotesf::Torrent* torrent);

        void torrentAddDuplicate();
        void torrentAddError();
        
        void gotTorrentFiles(int torrentId);
        void torrentFileRenamed(int torrentId, const QString& filePath, const QString& newName);
        void gotTorrentPeers(int torrentId);

        void gotDownloadDirFreeSpace(long long bytes);
        void gotFreeSpaceForPath(const QString& path, bool success, long long bytes);

        void backgroundUpdateChanged();
        void updateDisabledChanged();
    };
}

#endif // LIBTREMOTESF_RPC_H
