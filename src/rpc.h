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

#ifndef TREMOTESF_RPC_H
#define TREMOTESF_RPC_H

#include <functional>
#include <memory>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSslConfiguration>
#include <QUrl>

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace tremotesf
{
    class MainWindow;
    class ServerSettings;
    class ServerStats;
    class Torrent;

    typedef void (*tess) (void);

    class Rpc : public QObject
    {
        Q_OBJECT
        Q_ENUMS(Status)
        Q_ENUMS(Error)
        Q_PROPERTY(tremotesf::ServerSettings* serverSettings READ serverSettings CONSTANT)
        Q_PROPERTY(tremotesf::ServerStats* serverStats READ serverStats CONSTANT)
        Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
        Q_PROPERTY(Status status READ status NOTIFY statusChanged)
        Q_PROPERTY(QString statusString READ statusString NOTIFY statusStringChanged)
        Q_PROPERTY(Error error READ error NOTIFY errorChanged)
        Q_PROPERTY(bool local READ isLocal NOTIFY connectedChanged)
        Q_PROPERTY(int torrentsCount READ torrentsCount NOTIFY torrentsUpdated)
    public:
        enum Status
        {
            Disconnected,
            Connecting,
            Connected
        };

        enum Error
        {
            NoError,
            TimedOut,
            ConnectionError,
            ParseError,
            ServerIsTooNew,
            ServerIsTooOld,
        };

        explicit Rpc(QObject* parent = nullptr);

        ServerSettings* serverSettings() const;
        ServerStats* serverStats() const;

        const QList<std::shared_ptr<Torrent>>& torrents() const;

        bool isConnected() const;
        Status status() const;
        QString statusString() const;
        Error error() const;
        bool isLocal() const;

        int torrentsCount() const;

        Q_INVOKABLE void connect();
        Q_INVOKABLE void disconnect();

        Q_INVOKABLE void addTorrentFile(const QString& filePath,
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
        void moveTorrentsToTop(const QVariantList& ids);
        void moveTorrentsUp(const QVariantList& ids);
        void moveTorrentsDown(const QVariantList& ids);
        void moveTorrentsToBottom(const QVariantList& ids);

        void setSessionProperty(const QString& property, const QVariant& value);
        void setSessionProperties(const QVariantMap& properties);
        void setTorrentProperty(int id, const QString& property, const QVariant& value);
        void getTorrentFiles(int id);
        void getTorrentPeers(int id);

    private:
        void setStatus(Status status);
        void setError(Error error);

        void updateAccount();

        void getServerSettings();
        void getTorrents();
        void getServerStats();

        void checkIfTorrentsUpdated();
        void startUpdateTimer();
        void updateData();

        void onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator);
        QNetworkReply* postRequest(const QByteArray& data, const std::function<void()>& callAfterNewId = nullptr);
        bool checkSessionId(const QNetworkReply* reply);
        bool checkReplyError(const QNetworkReply* reply);
        bool checkParseResult(const QVariant& result);

        std::shared_ptr<Torrent> torrentById(int id) const;


        QNetworkAccessManager* mNetwork;
        bool mAuthenticationRequested;
        QByteArray mSessionId;

        QUrl mServerUrl;
        bool mAuthentication;
        QSslConfiguration mSslConfiguration;
        QString mUsername;
        QString mPassword;
        int mTimeout;

        bool mRpcVersionChecked;
        bool mServerSettingsUpdated;
        bool mTorrentsUpdated;
        bool mFirstUpdate;
        bool mServerStatsUpdated;
        QTimer* mUpdateTimer;

        ServerSettings* mServerSettings;
        QList<std::shared_ptr<Torrent>> mTorrents;
        ServerStats* mServerStats;

        Status mStatus;
        Error mError;

    signals:
        void connectedChanged();
        void statusChanged();
        void statusStringChanged();
        void errorChanged();

        void torrentsUpdated();

        void torrentAdded(const QString& torrent);
        void torrentFinished(const QString& torrent);
    };
}

#endif // TREMOTESF_RPC_H
