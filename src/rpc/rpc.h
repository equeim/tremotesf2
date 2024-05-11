// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_RPC_H
#define TREMOTESF_RPC_RPC_H

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <QByteArray>
#include <QObject>

#include "coroutines/scope.h"
#include "log/formatters.h"
#include "serversettings.h"
#include "serverstats.h"
#include "torrent.h"

namespace tremotesf {
    Q_NAMESPACE

    namespace impl {
        class RequestRouter;
    }

    struct ConnectionConfiguration {
        Q_GADGET
    public:
        enum class ProxyType { Default, Http, Socks5 };
        Q_ENUM(ProxyType)

        QString address{};
        int port{};
        QString apiPath{};

        ProxyType proxyType{ProxyType::Default};
        QString proxyHostname{};
        int proxyPort{};
        QString proxyUser{};
        QString proxyPassword{};

        bool https{};
        bool selfSignedCertificateEnabled{};
        QByteArray selfSignedCertificate{};
        bool clientCertificateEnabled{};
        QByteArray clientCertificate{};

        bool authentication{};
        QString username{};
        QString password{};

        int updateInterval{};
        int timeout{};

        bool autoReconnectEnabled{};
        int autoReconnectInterval{};

        bool operator==(const ConnectionConfiguration& rhs) const = default;
    };

    enum class RpcConnectionState { Disconnected, Connecting, Connected };
    Q_ENUM_NS(RpcConnectionState)

    enum class RpcError {
        NoError,
        TimedOut,
        ConnectionError,
        AuthenticationError,
        ParseError,
        ServerIsTooNew,
        ServerIsTooOld
    };
    Q_ENUM_NS(RpcError)

    class Rpc : public QObject {
        Q_OBJECT
    public:
        using ConnectionState = RpcConnectionState;
        using Error = RpcError;

        explicit Rpc(QObject* parent = nullptr);
        ~Rpc() override;
        Q_DISABLE_COPY_MOVE(Rpc)

        ServerSettings* serverSettings() const;
        ServerStats* serverStats() const;

        const std::vector<std::unique_ptr<Torrent>>& torrents() const;
        Torrent* torrentByHash(const QString& hash) const;
        Torrent* torrentById(int id) const;

        struct Status {
            ConnectionState connectionState{ConnectionState::Disconnected};
            Error error{Error::NoError};
            QString errorMessage{};
            QString detailedErrorMessage{};

            bool operator==(const Status& other) const = default;

            QString toString() const;
        };

        bool isConnected() const;
        const Status& status() const;
        ConnectionState connectionState() const;
        Error error() const;
        const QString& errorMessage() const;
        const QString& detailedErrorMessage() const;
        bool isLocal() const;

        int torrentsCount() const;

        void setConnectionConfiguration(const ConnectionConfiguration& configuration);
        void resetConnectionConfiguration();

        void connect();
        void disconnect();

        void addTorrentFile(
            QString filePath,
            QString downloadDirectory,
            std::vector<int> unwantedFiles,
            std::vector<int> highPriorityFiles,
            std::vector<int> lowPriorityFiles,
            std::map<QString, QString> renamedFiles,
            TorrentData::Priority bandwidthPriority,
            bool start
        );

        void
        addTorrentLink(QString link, QString downloadDirectory, TorrentData::Priority bandwidthPriority, bool start);

        void startTorrents(std::span<const int> ids);
        void startTorrentsNow(std::span<const int> ids);
        void pauseTorrents(std::span<const int> ids);
        void removeTorrents(std::span<const int> ids, bool deleteFiles);
        void checkTorrents(std::span<const int> ids);
        void moveTorrentsToTop(std::span<const int> ids);
        void moveTorrentsUp(std::span<const int> ids);
        void moveTorrentsDown(std::span<const int> ids);
        void moveTorrentsToBottom(std::span<const int> ids);

        void reannounceTorrents(std::span<const int> ids);

        void setSessionProperty(QString property, QJsonValue value);
        void setSessionProperties(QJsonObject properties);
        void setTorrentProperty(int id, QString property, QJsonValue value, bool updateIfSuccessful = false);
        void setTorrentsLocation(std::span<const int> ids, QString location, bool moveFiles);
        void getTorrentFiles(int torrentId);
        void getTorrentPeers(int torrentId);

        void renameTorrentFile(int torrentId, QString filePath, QString newName);

        void getDownloadDirFreeSpace();
        void getFreeSpaceForPath(QString path);

        void shutdownServer();

    private:
        void setStatus(Status&& status);
        void resetStateOnConnectionStateChanged(ConnectionState oldConnectionState, size_t& removedTorrentsCount);
        void emitSignalsOnConnectionStateChanged(ConnectionState oldConnectionState, size_t removedTorrentsCount);

        Coroutine<> postRequest(QLatin1String method, QJsonObject arguments, bool updateIfSuccessful = true);

        Coroutine<> addTorrentFileImpl(
            QString filePath,
            QString downloadDirectory,
            std::vector<int> unwantedFiles,
            std::vector<int> highPriorityFiles,
            std::vector<int> lowPriorityFiles,
            std::map<QString, QString> renamedFiles,
            TorrentData::Priority bandwidthPriority,
            bool start
        );
        Coroutine<> addTorrentLinkImpl(
            QString link, QString downloadDirectory, TorrentData::Priority bandwidthPriority, bool start
        );
        Coroutine<> getTorrentsFiles(QJsonArray ids);
        Coroutine<> getTorrentsPeers(QJsonArray ids);
        Coroutine<> renameTorrentFileImpl(int torrentId, QString filePath, QString newName);
        Coroutine<> getDownloadDirFreeSpaceImpl();
        Coroutine<> getFreeSpaceForPathImpl(QString path);
        Coroutine<> shutdownServerImpl();

        Coroutine<> getServerSettings();
        Coroutine<> getTorrents();
        Coroutine<> checkTorrentsSingleFile(std::vector<int> torrentIds);
        Coroutine<> getServerStats();

        Coroutine<> connectAndPerformDataUpdates();
        Coroutine<> updateData();

        Coroutine<> checkIfServerIsLocal();

        void onRequestFailed(RpcError error, const QString& errorMessage, const QString& detailedErrorMessage);
        Coroutine<> autoReconnect();

        impl::RequestRouter* mRequestRouter{};
        CoroutineScope mBackgroundRequestsCoroutineScope{};
        CoroutineScope mAutoReconnectCoroutineScope{};

        bool mAutoReconnectEnabled{};

        std::optional<bool> mServerIsLocal{};

        std::chrono::seconds mUpdateInterval{};
        std::chrono::seconds mAutoReconnectInterval{};

        ServerSettings* mServerSettings{};
        std::vector<std::unique_ptr<Torrent>> mTorrents{};
        ServerStats* mServerStats{};

        Status mStatus{};

    signals:
        void aboutToDisconnect();
        void statusChanged();
        void connectedChanged();
        void connectionStateChanged();
        void errorChanged();

        void onAboutToRemoveTorrents(size_t first, size_t last);
        void onRemovedTorrents(size_t first, size_t last);
        void onChangedTorrents(size_t first, size_t last);
        void onAboutToAddTorrents(size_t count);
        void onAddedTorrents(size_t count);

        void torrentsUpdated(
            const std::vector<std::pair<int, int>>& removedIndexRanges,
            const std::vector<std::pair<int, int>>& changedIndexRanges,
            int addedCount
        );

        void torrentFilesUpdated(const tremotesf::Torrent* torrent, const std::vector<int>& changedIndexes);
        void torrentPeersUpdated(
            const tremotesf::Torrent* torrent,
            const std::vector<std::pair<int, int>>& removedIndexRanges,
            const std::vector<std::pair<int, int>>& changedIndexRanges,
            int addedCount
        );

        void torrentFileRenamed(int torrentId, const QString& filePath, const QString& newName);

        void torrentAdded(tremotesf::Torrent* torrent);
        void torrentFinished(tremotesf::Torrent* torrent);

        void torrentAddDuplicate();
        void torrentAddError();

        void gotDownloadDirFreeSpace(qint64 bytes);
        void gotFreeSpaceForPath(const QString& path, bool success, qint64 bytes);
    };
}

SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::RpcConnectionState)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::RpcError)

#endif // TREMOTESF_RPC_RPC_H
