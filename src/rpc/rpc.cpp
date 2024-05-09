// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rpc.h"

#include <QCoreApplication>
#include <QFile>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkProxy>
#include <QTimer>
#include <QSslCertificate>
#include <QSslKey>
#include <QtConcurrentRun>

#include "addressutils.h"
#include "fileutils.h"
#include "jsonutils.h"
#include "itemlistupdater.h"
#include "log/log.h"
#include "requestrouter.h"
#include "serversettings.h"
#include "serverstats.h"
#include "torrent.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QHostAddress)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf {
    using namespace impl;

    namespace {
        // Transmission 2.40+
        constexpr int minimumRpcVersion = 14;

        constexpr auto torrentsKey = "torrents"_l1;
        constexpr auto torrentDuplicateKey = "torrent-duplicate"_l1;
    }

    using namespace impl;

    QString Rpc::Status::toString() const {
        switch (connectionState) {
        case ConnectionState::Disconnected:
            switch (error) {
            case Error::NoError:
                //: Server connection status
                return qApp->translate("tremotesf", "Disconnected");
            case Error::TimedOut:
                //: Server connection status
                return qApp->translate("tremotesf", "Timed out");
            case Error::ConnectionError:
                //: Server connection status
                return qApp->translate("tremotesf", "Connection error");
            case Error::AuthenticationError:
                //: Server connection status
                return qApp->translate("tremotesf", "Authentication error");
            case Error::ParseError:
                //: Server connection status
                return qApp->translate("tremotesf", "Parse error");
            case Error::ServerIsTooNew:
                //: Server connection status
                return qApp->translate("tremotesf", "Server is too new");
            case Error::ServerIsTooOld:
                //: Server connection status
                return qApp->translate("tremotesf", "Server is too old");
            }
            break;
        case ConnectionState::Connecting:
            //: Server connection status
            return qApp->translate("tremotesf", "Connecting...");
        case ConnectionState::Connected:
            //: Server connection status
            return qApp->translate("tremotesf", "Connected");
        }

        return {};
    }

    Rpc::Rpc(QObject* parent)
        : QObject(parent),
          mRequestRouter(new RequestRouter(this)),
          mUpdateTimer(new QTimer(this)),
          mAutoReconnectTimer(new QTimer(this)),
          mServerSettings(new ServerSettings(this, this)),
          mServerStats(new ServerStats(this)) {
        mAutoReconnectTimer->setSingleShot(true);
        QObject::connect(mAutoReconnectTimer, &QTimer::timeout, this, [=, this] {
            info().log("Auto reconnection");
            connect();
        });

        mUpdateTimer->setSingleShot(true);
        QObject::connect(mUpdateTimer, &QTimer::timeout, this, [=, this] { updateData(); });

        QObject::connect(
            mRequestRouter,
            &RequestRouter::requestFailed,
            this,
            [=, this](RpcError error, const QString& errorMessage, const QString& detailedErrorMessage) {
                setStatus({RpcConnectionState::Disconnected, error, errorMessage, detailedErrorMessage});
                if (mAutoReconnectEnabled && !mUpdateDisabled) {
                    info().log("Auto reconnecting in {} seconds", mAutoReconnectTimer->interval() / 1000);
                    mAutoReconnectTimer->start();
                }
            }
        );
    }

    Rpc::~Rpc() = default;

    ServerSettings* Rpc::serverSettings() const { return mServerSettings; }

    ServerStats* Rpc::serverStats() const { return mServerStats; }

    const std::vector<std::unique_ptr<Torrent>>& Rpc::torrents() const { return mTorrents; }

    Torrent* Rpc::torrentByHash(const QString& hash) const {
        for (const std::unique_ptr<Torrent>& torrent : mTorrents) {
            if (torrent->data().hashString == hash) {
                return torrent.get();
            }
        }
        return nullptr;
    }

    Torrent* Rpc::torrentById(int id) const {
        const auto found = std::ranges::find(mTorrents, id, [](const auto& torrent) { return torrent->data().id; });
        return (found != mTorrents.end()) ? found->get() : nullptr;
    }

    bool Rpc::isConnected() const { return (mStatus.connectionState == ConnectionState::Connected); }

    const Rpc::Status& Rpc::status() const { return mStatus; }

    Rpc::ConnectionState Rpc::connectionState() const { return mStatus.connectionState; }

    Rpc::Error Rpc::error() const { return mStatus.error; }

    const QString& Rpc::errorMessage() const { return mStatus.errorMessage; }

    const QString& Rpc::detailedErrorMessage() const { return mStatus.detailedErrorMessage; }

    bool Rpc::isLocal() const { return mServerIsLocal.value_or(false); }

    int Rpc::torrentsCount() const { return static_cast<int>(mTorrents.size()); }

    bool Rpc::isUpdateDisabled() const { return mUpdateDisabled; }

    void Rpc::setUpdateDisabled(bool disabled) {
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

    void Rpc::setConnectionConfiguration(const ConnectionConfiguration& configuration) {
        disconnect();

        RequestRouter::RequestsConfiguration requestsConfig{};
        if (configuration.https) {
            requestsConfig.serverUrl.setScheme("https"_l1);
        } else {
            requestsConfig.serverUrl.setScheme("http"_l1);
        }
        requestsConfig.serverUrl.setHost(configuration.address);
        if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
            warning().log("Error setting URL hostname: {}", error);
        }
        requestsConfig.serverUrl.setPort(configuration.port);
        if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
            warning().log("Error setting URL port: {}", error);
        }
        if (auto i = configuration.apiPath.indexOf('?'); i != -1) {
            requestsConfig.serverUrl.setPath(configuration.apiPath.mid(0, i));
            if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
                warning().log("Error setting URL path: {}", error);
            }
            if ((i + 1) < configuration.apiPath.size()) {
                requestsConfig.serverUrl.setQuery(configuration.apiPath.mid(i + 1));
                if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
                    warning().log("Error setting URL query: {}", error);
                }
            }
        } else {
            requestsConfig.serverUrl.setPath(configuration.apiPath);
            if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
                warning().log("Error setting URL path: {}", error);
            }
        }
        if (!requestsConfig.serverUrl.isValid()) {
            warning().log("URL {} is invalid", requestsConfig.serverUrl);
        }

        switch (configuration.proxyType) {
        case ConnectionConfiguration::ProxyType::Default:
            break;
        case ConnectionConfiguration::ProxyType::Http:
            requestsConfig.proxy = QNetworkProxy(
                QNetworkProxy::HttpProxy,
                configuration.proxyHostname,
                static_cast<quint16>(configuration.proxyPort),
                configuration.proxyUser,
                configuration.proxyPassword
            );
            break;
        case ConnectionConfiguration::ProxyType::Socks5:
            requestsConfig.proxy = QNetworkProxy(
                QNetworkProxy::Socks5Proxy,
                configuration.proxyHostname,
                static_cast<quint16>(configuration.proxyPort),
                configuration.proxyUser,
                configuration.proxyPassword
            );
            break;
        }

        if (configuration.https && configuration.selfSignedCertificateEnabled) {
            requestsConfig.serverCertificateChain =
                QSslCertificate::fromData(configuration.selfSignedCertificate, QSsl::Pem);
        }

        if (configuration.clientCertificateEnabled) {
            requestsConfig.clientCertificate = QSslCertificate(configuration.clientCertificate, QSsl::Pem);
            requestsConfig.clientPrivateKey = QSslKey(configuration.clientCertificate, QSsl::Rsa);
        }

        requestsConfig.authentication = configuration.authentication;
        requestsConfig.username = configuration.username;
        requestsConfig.password = configuration.password;
        requestsConfig.timeout = std::chrono::seconds(configuration.timeout); // server.timeout is in seconds

        mRequestRouter->setConfiguration(std::move(requestsConfig));

        mUpdateTimer->setInterval(configuration.updateInterval * 1000); // msecs

        mAutoReconnectEnabled = configuration.autoReconnectEnabled;
        mAutoReconnectTimer->setInterval(configuration.autoReconnectInterval * 1000); // msecs
        mAutoReconnectTimer->stop();
    }

    void Rpc::resetConnectionConfiguration() {
        disconnect();
        mRequestRouter->resetConfiguration();
        mAutoReconnectEnabled = false;
        mAutoReconnectTimer->stop();
    }

    void Rpc::connect() {
        if (connectionState() == ConnectionState::Disconnected && mRequestRouter->configuration().has_value()) {
            setStatus(Status{.connectionState = ConnectionState::Connecting});
            getServerSettings();
        }
    }

    void Rpc::disconnect() {
        setStatus(Status{.connectionState = ConnectionState::Disconnected});
        mAutoReconnectTimer->stop();
    }

    void Rpc::addTorrentFile(
        const QString& filePath,
        const QString& downloadDirectory,
        const std::vector<int>& unwantedFiles,
        const std::vector<int>& highPriorityFiles,
        const std::vector<int>& lowPriorityFiles,
        const std::map<QString, QString>& renamedFiles,
        TorrentData::Priority bandwidthPriority,
        bool start
    ) {
        if (!isConnected()) {
            return;
        }
        auto file = std::make_shared<QFile>(filePath);
        try {
            openFile(*file, QIODevice::ReadOnly);
        } catch (const QFileError& e) {
            warning().logWithException(e, "addTorrentFile: failed to open torrent file");
            emit torrentAddError();
            return;
        }
        addTorrentFile(
            std::move(file),
            downloadDirectory,
            unwantedFiles,
            highPriorityFiles,
            lowPriorityFiles,
            renamedFiles,
            bandwidthPriority,
            start
        );
    }

    void Rpc::addTorrentFile(
        std::shared_ptr<QFile> file,
        const QString& downloadDirectory,
        const std::vector<int>& unwantedFiles,
        const std::vector<int>& highPriorityFiles,
        const std::vector<int>& lowPriorityFiles,
        const std::map<QString, QString>& renamedFiles,
        TorrentData::Priority bandwidthPriority,
        bool start
    ) {
        if (!isConnected()) {
            return;
        }
        const auto future = QtConcurrent::run([=, this, file = std::move(file)]() -> std::optional<QByteArray> {
            try {
                return RequestRouter::makeRequestData(
                    "torrent-add"_l1,
                    {{"metainfo"_l1, readFileAsBase64String(*file)},
                     {"download-dir"_l1, downloadDirectory},
                     {"files-unwanted"_l1, toJsonArray(unwantedFiles)},
                     {"priority-high"_l1, toJsonArray(highPriorityFiles)},
                     {"priority-low"_l1, toJsonArray(lowPriorityFiles)},
                     {"bandwidthPriority"_l1, TorrentData::priorityToInt(bandwidthPriority)},
                     {"paused"_l1, !start}}
                );
            } catch (const QFileError& e) {
                warning().logWithException(e, "addTorrentFile: failed to read torrent file");
                emit torrentAddError();
                return std::nullopt;
            }
        });
        using Watcher = QFutureWatcher<std::optional<QByteArray>>;
        auto watcher = new Watcher(this);
        QObject::connect(watcher, &Watcher::finished, this, [=, this] {
            std::optional<QByteArray> requestData = watcher->result();
            watcher->deleteLater();
            if (!requestData.has_value()) return;
            if (!isConnected()) return;
            mRequestRouter->postRequest(
                "torrent-add"_l1,
                requestData.value(),
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.arguments.contains(torrentDuplicateKey)) {
                        emit torrentAddDuplicate();
                    } else if (response.success) {
                        if (!renamedFiles.empty()) {
                            const auto torrentJson = response.arguments.value("torrent-added"_l1).toObject();
                            const auto id = Torrent::idFromJson(torrentJson);
                            if (id.has_value()) {
                                for (const auto& [filePath, newName] : renamedFiles) {
                                    renameTorrentFile(*id, filePath, newName);
                                }
                            }
                        }
                        updateData();
                    } else {
                        emit torrentAddError();
                    }
                }
            );
        });
        watcher->setFuture(future);
    }

    void Rpc::addTorrentLink(
        const QString& link, const QString& downloadDirectory, TorrentData::Priority bandwidthPriority, bool start
    ) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-add"_l1,
                {{"filename"_l1, link},
                 {"download-dir"_l1, downloadDirectory},
                 {"bandwidthPriority"_l1, TorrentData::priorityToInt(bandwidthPriority)},
                 {"paused"_l1, !start}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.arguments.contains(torrentDuplicateKey)) {
                        emit torrentAddDuplicate();
                    } else if (response.success) {
                        updateData();
                    } else {
                        emit torrentAddError();
                    }
                }
            );
        }
    }

    void Rpc::startTorrents(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-start"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::startTorrentsNow(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-start-now"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::pauseTorrents(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-stop"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::removeTorrents(std::span<const int> ids, bool deleteFiles) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-remove"_l1,
                {{"ids"_l1, toJsonArray(ids)}, {"delete-local-data"_l1, deleteFiles}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::checkTorrents(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-verify"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::moveTorrentsToTop(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "queue-move-top"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::moveTorrentsUp(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "queue-move-up"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::moveTorrentsDown(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "queue-move-down"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::moveTorrentsToBottom(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "queue-move-bottom"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::reannounceTorrents(std::span<const int> ids) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-reannounce"_l1,
                {{"ids"_l1, toJsonArray(ids)}},
                RequestRouter::RequestType::Independent
            );
        }
    }

    void Rpc::setSessionProperty(const QString& property, const QJsonValue& value) {
        setSessionProperties({{property, value}});
    }

    void Rpc::setSessionProperties(const QJsonObject& properties) {
        if (isConnected()) {
            mRequestRouter->postRequest("session-set"_l1, properties, RequestRouter::RequestType::Independent);
        }
    }

    void Rpc::setTorrentProperty(int id, const QString& property, const QJsonValue& value, bool updateIfSuccessful) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-set"_l1,
                {{"ids"_l1, QJsonArray{id}}, {property, value}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success && updateIfSuccessful) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::setTorrentsLocation(std::span<const int> ids, const QString& location, bool moveFiles) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-set-location"_l1,
                {{"ids"_l1, toJsonArray(ids)}, {"location"_l1, location}, {"move"_l1, moveFiles}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        updateData();
                    }
                }
            );
        }
    }

    void Rpc::getTorrentsFiles(std::span<const int> ids, bool asDataUpdate) {
        mRequestRouter->postRequest(
            "torrent-get"_l1,
            {{"fields"_l1, QJsonArray{"id"_l1, "files"_l1, "fileStats"_l1}}, {"ids"_l1, toJsonArray(ids)}},
            asDataUpdate ? RequestRouter::RequestType::DataUpdate : RequestRouter::RequestType::Independent,
            [=, this](const RequestRouter::Response& response) {
                if (response.success) {
                    const QJsonArray torrents(response.arguments.value(torrentsKey).toArray());
                    for (const auto& torrentJson : torrents) {
                        const auto object = torrentJson.toObject();
                        const auto torrentId = Torrent::idFromJson(object);
                        if (torrentId.has_value()) {
                            Torrent* torrent = torrentById(*torrentId);
                            if (torrent && torrent->isFilesEnabled()) {
                                torrent->updateFiles(object);
                            }
                        }
                    }
                    if (asDataUpdate) {
                        maybeFinishUpdateOrConnection();
                    }
                }
            }
        );
    }

    void Rpc::getTorrentsPeers(std::span<const int> ids, bool asDataUpdate) {
        mRequestRouter->postRequest(
            "torrent-get"_l1,
            {{"fields"_l1, QJsonArray{"id"_l1, "peers"_l1}}, {"ids"_l1, toJsonArray(ids)}},
            asDataUpdate ? RequestRouter::RequestType::DataUpdate : RequestRouter::RequestType::Independent,
            [=, this](const RequestRouter::Response& response) {
                if (response.success) {
                    const QJsonArray torrents(response.arguments.value(torrentsKey).toArray());
                    for (const auto& torrentJson : torrents) {
                        const auto object = torrentJson.toObject();
                        const auto torrentId = Torrent::idFromJson(object);
                        if (torrentId.has_value()) {
                            Torrent* torrent = torrentById(*torrentId);
                            if (torrent && torrent->isPeersEnabled()) {
                                torrent->updatePeers(object);
                            }
                        }
                    }
                    if (asDataUpdate) {
                        maybeFinishUpdateOrConnection();
                    }
                }
            }
        );
    }

    void Rpc::renameTorrentFile(int torrentId, const QString& filePath, const QString& newName) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "torrent-rename-path"_l1,
                {{"ids"_l1, QJsonArray{torrentId}}, {"path"_l1, filePath}, {"name"_l1, newName}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        Torrent* torrent = torrentById(torrentId);
                        if (torrent) {
                            const QString path(response.arguments.value("path"_l1).toString());
                            const QString newName(response.arguments.value("name"_l1).toString());
                            emit torrent->fileRenamed(path, newName);
                            emit torrentFileRenamed(torrentId, path, newName);
                            updateData();
                        }
                    }
                }
            );
        }
    }

    void Rpc::getDownloadDirFreeSpace() {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "download-dir-free-space"_l1,
                QByteArrayLiteral("{"
                                  "\"arguments\":{"
                                  "\"fields\":["
                                  "\"download-dir-free-space\""
                                  "]"
                                  "},"
                                  "\"method\":\"session-get\""
                                  "}"),
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        emit gotDownloadDirFreeSpace(toInt64(response.arguments.value("download-dir-free-space"_l1)));
                    }
                }
            );
        }
    }

    void Rpc::getFreeSpaceForPath(const QString& path) {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "free-space"_l1,
                {{"path"_l1, path}},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    emit gotFreeSpaceForPath(
                        path,
                        response.success,
                        response.success ? toInt64(response.arguments.value("size-bytes"_l1)) : 0
                    );
                }
            );
        }
    }

    void Rpc::updateData() {
        if (connectionState() != ConnectionState::Disconnected && !mUpdating) {
            debug().log("Updating data");
            mUpdateTimer->stop();
            mUpdating = true;
            if (isConnected()) {
                getServerSettings();
            }
            getTorrents();
            getServerStats();
        } else {
            warning().log(
                "updateData: called in incorrect state, connectionState = {}, updating = {}",
                connectionState(),
                mUpdating
            );
        }
    }

    void Rpc::shutdownServer() {
        if (isConnected()) {
            mRequestRouter->postRequest(
                "session-close"_l1,
                QJsonObject{},
                RequestRouter::RequestType::Independent,
                [=, this](const RequestRouter::Response& response) {
                    if (response.success) {
                        info().log("Successfully sent shutdown request, disconnecting");
                        disconnect();
                    }
                }
            );
        }
    }

    void Rpc::setStatus(Status&& status) {
        if (status == mStatus) {
            return;
        }

        const Status oldStatus = mStatus;

        const bool connectionStateChanged = status.connectionState != oldStatus.connectionState;
        if (connectionStateChanged && oldStatus.connectionState == ConnectionState::Connected) {
            emit aboutToDisconnect();
        }

        mStatus = std::move(status);

        size_t removedTorrentsCount = 0;
        if (connectionStateChanged) {
            resetStateOnConnectionStateChanged(oldStatus.connectionState, removedTorrentsCount);
        }

        emit statusChanged();

        if (connectionStateChanged) {
            emitSignalsOnConnectionStateChanged(oldStatus.connectionState, removedTorrentsCount);
        }

        if (mStatus.error != oldStatus.error || mStatus.errorMessage != oldStatus.errorMessage) {
            emit errorChanged();
        }
    }

    void Rpc::resetStateOnConnectionStateChanged(ConnectionState oldConnectionState, size_t& removedTorrentsCount) {
        switch (mStatus.connectionState) {
        case ConnectionState::Disconnected: {
            info().log("Disconnected");

            mRequestRouter->cancelPendingRequestsAndClearSessionId();

            mUpdating = false;
            mServerIsLocal = std::nullopt;
            if (mPendingHostInfoLookupId.has_value()) {
                QHostInfo::abortHostLookup(*mPendingHostInfoLookupId);
                mPendingHostInfoLookupId = std::nullopt;
            }
            mUpdateTimer->stop();

            if (!mTorrents.empty() && oldConnectionState == ConnectionState::Connected) {
                removedTorrentsCount = mTorrents.size();
                emit onAboutToRemoveTorrents(0, removedTorrentsCount);
                mTorrents.clear();
                emit onRemovedTorrents(0, removedTorrentsCount);
            }

            break;
        }
        case ConnectionState::Connecting:
            info().log("Connecting");
            break;
        case ConnectionState::Connected: {
            info().log("Connected");
            break;
        }
        }
    }

    void
    Rpc::emitSignalsOnConnectionStateChanged(Rpc::ConnectionState oldConnectionState, size_t removedTorrentsCount) {
        switch (mStatus.connectionState) {
        case ConnectionState::Disconnected: {
            emit connectionStateChanged();
            if (oldConnectionState == ConnectionState::Connected) {
                emit connectedChanged();
                emit torrentsUpdated({{0, static_cast<int>(removedTorrentsCount)}}, {}, 0);
            }
            break;
        }
        case ConnectionState::Connecting:
            emit connectionStateChanged();
            break;
        case ConnectionState::Connected: {
            emit torrentsUpdated({}, {}, torrentsCount());
            emit connectionStateChanged();
            emit connectedChanged();
            break;
        }
        }
    }

    void Rpc::getServerSettings() {
        mRequestRouter->postRequest(
            "session-get"_l1,
            QByteArrayLiteral("{\"method\":\"session-get\"}"),
            RequestRouter::RequestType::DataUpdate,
            [=, this](const RequestRouter::Response& response) {
                if (response.success) {
                    mServerSettings->update(response.arguments);
                    if (connectionState() == ConnectionState::Connecting) {
                        if (mServerSettings->data().minimumRpcVersion > minimumRpcVersion) {
                            setStatus(
                                Status{.connectionState = ConnectionState::Disconnected, .error = Error::ServerIsTooNew}
                            );
                        } else if (mServerSettings->data().rpcVersion < minimumRpcVersion) {
                            setStatus(
                                Status{.connectionState = ConnectionState::Disconnected, .error = Error::ServerIsTooOld}
                            );
                        } else {
                            updateData();
                            checkIfServerIsLocal();
                        }
                    } else {
                        maybeFinishUpdateOrConnection();
                    }
                }
            }
        );
    }

    struct NewTorrent {
        int id{};
        QJsonValue json{};
    };

    class TorrentsListUpdater final : public ItemListUpdater<std::unique_ptr<Torrent>, std::vector<NewTorrent>> {
    public:
        inline explicit TorrentsListUpdater(Rpc& rpc) : mRpc(rpc) {}

        const std::vector<std::optional<TorrentData::UpdateKey>>* keys{};
        std::vector<std::pair<int, int>> removedIndexRanges{};
        std::vector<std::pair<int, int>> changedIndexRanges{};
        int addedCount{};
        std::vector<int> metadataCompletedIds{};

    protected:
        std::vector<NewTorrent>::iterator
        findNewItemForItem(std::vector<NewTorrent>& newTorrents, const std::unique_ptr<Torrent>& torrent) override {
            return std::ranges::find(newTorrents, torrent->data().id, &NewTorrent::id);
        }

        void onAboutToRemoveItems(size_t first, size_t last) override {
            emit mRpc.onAboutToRemoveTorrents(first, last);
        };

        void onRemovedItems(size_t first, size_t last) override {
            removedIndexRanges.emplace_back(static_cast<int>(first), static_cast<int>(last));
            emit mRpc.onRemovedTorrents(first, last);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        bool updateItem(std::unique_ptr<Torrent>& torrent, NewTorrent&& newTorrent) override {
            const bool wasFinished = torrent->data().isFinished();
            const bool wasPaused = (torrent->data().status == TorrentData::Status::Paused);
            const auto oldSizeWhenDone = torrent->data().sizeWhenDone;
            const bool metadataWasComplete = torrent->data().metadataComplete;

            bool changed{};
            if (keys) {
                changed = torrent->update(*keys, newTorrent.json.toArray());
            } else {
                changed = torrent->update(newTorrent.json.toObject());
            }
            if (changed) {
                // Don't emit torrentFinished() if torrent's size became smaller
                // since there is high chance that it happened because user unselected some files
                // and torrent immediately became finished. We don't want notification in that case
                if (!wasFinished && torrent->data().isFinished() && !wasPaused &&
                    torrent->data().sizeWhenDone >= oldSizeWhenDone) {
                    emit mRpc.torrentFinished(torrent.get());
                }
                if (!metadataWasComplete && torrent->data().metadataComplete) {
                    metadataCompletedIds.push_back(newTorrent.id);
                }
            }

            return changed;
        }

        void onChangedItems(size_t first, size_t last) override {
            changedIndexRanges.emplace_back(static_cast<int>(first), static_cast<int>(last));
            emit mRpc.onChangedTorrents(first, last);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        std::unique_ptr<Torrent> createItemFromNewItem(NewTorrent&& newTorrent) override {
            std::unique_ptr<Torrent> torrent{};
            if (keys) {
                torrent = std::make_unique<Torrent>(newTorrent.id, *keys, newTorrent.json.toArray(), &mRpc);
            } else {
                torrent = std::make_unique<Torrent>(newTorrent.id, newTorrent.json.toObject(), &mRpc);
            }
            if (mRpc.isConnected()) {
                emit mRpc.torrentAdded(torrent.get());
            }
            if (torrent->data().metadataComplete) {
                metadataCompletedIds.push_back(newTorrent.id);
            }
            return torrent;
        }

        void onAboutToAddItems(size_t count) override { emit mRpc.onAboutToAddTorrents(count); }

        void onAddedItems(size_t count) override {
            addedCount = static_cast<int>(count);
            emit mRpc.onAddedTorrents(count);
        };

    private:
        Rpc& mRpc;
    };

    void Rpc::getTorrents() {
        const QByteArray* requestData{};
        const bool tableMode = mServerSettings->data().hasTableMode();
        if (tableMode) {
            static const auto tableModeRequestData = RequestRouter::makeRequestData(
                "torrent-get"_l1,
                QJsonObject{{"fields"_l1, Torrent::updateFields()}, {"format"_l1, "table"_l1}}
            );
            requestData = &tableModeRequestData;
        } else {
            static const auto objectsModeRequestData =
                RequestRouter::makeRequestData("torrent-get"_l1, QJsonObject{{"fields"_l1, Torrent::updateFields()}});
            requestData = &objectsModeRequestData;
        }

        mRequestRouter->postRequest(
            "torrent-get"_l1,
            *requestData,
            RequestRouter::RequestType::DataUpdate,
            [=, this](const RequestRouter::Response& response) {
                if (!response.success) {
                    return;
                }

                TorrentsListUpdater updater(*this);
                {
                    const QJsonArray torrentsJsons = response.arguments.value("torrents"_l1).toArray();
                    std::vector<NewTorrent> newTorrents{};
                    if (tableMode) {
                        if (!torrentsJsons.empty()) {
                            const auto keys = Torrent::mapUpdateKeys(torrentsJsons.first().toArray());
                            const auto idKeyIndex = Torrent::idKeyIndex(keys);
                            if (idKeyIndex.has_value()) {
                                newTorrents.reserve(static_cast<size_t>(torrentsJsons.size() - 1));
                                for (const auto& json : torrentsJsons | std::views::drop(1)) {
                                    const auto array = json.toArray();
                                    if (static_cast<size_t>(array.size()) == keys.size()) {
                                        newTorrents.push_back(NewTorrent{array[*idKeyIndex].toInt(), json});
                                    }
                                }
                                updater.keys = &keys;
                                updater.update(mTorrents, std::move(newTorrents));
                            }
                        }
                    } else {
                        newTorrents.reserve(static_cast<size_t>(torrentsJsons.size()));
                        for (const auto& torrentJson : torrentsJsons) {
                            const auto id = Torrent::idFromJson(torrentJson.toObject());
                            if (id.has_value()) {
                                newTorrents.push_back(NewTorrent{*id, torrentJson});
                            }
                        }
                        updater.update(mTorrents, std::move(newTorrents));
                    }
                }

                if (!updater.metadataCompletedIds.empty()) {
                    checkTorrentsSingleFile(updater.metadataCompletedIds);
                }
                std::vector<int> getFilesIds{};
                std::vector<int> getPeersIds{};
                for (const auto& torrent : mTorrents) {
                    if (torrent->isFilesEnabled()) {
                        getFilesIds.push_back(torrent->data().id);
                    }
                    if (torrent->isPeersEnabled()) {
                        getPeersIds.push_back(torrent->data().id);
                    }
                }
                if (!getFilesIds.empty()) {
                    getTorrentsFiles(getFilesIds, true);
                }
                if (!getPeersIds.empty()) {
                    getTorrentsPeers(getPeersIds, true);
                }

                const bool wasConnecting = connectionState() == ConnectionState::Connecting;
                maybeFinishUpdateOrConnection();
                if (!wasConnecting) {
                    emit torrentsUpdated(updater.removedIndexRanges, updater.changedIndexRanges, updater.addedCount);
                }
            }
        );
    }

    void Rpc::checkTorrentsSingleFile(std::span<const int> torrentIds) {
        mRequestRouter->postRequest(
            "torrent-get"_l1,
            {{"fields"_l1, QJsonArray{"id"_l1, "priorities"_l1}}, {"ids"_l1, toJsonArray(torrentIds)}},
            RequestRouter::RequestType::DataUpdate,
            [=, this](const RequestRouter::Response& response) {
                if (response.success) {
                    const auto torrentJsons = response.arguments.value(torrentsKey).toArray();
                    for (const auto& torrentJson : torrentJsons) {
                        const auto object = torrentJson.toObject();
                        const auto torrentId = Torrent::idFromJson(object);
                        if (torrentId.has_value()) {
                            Torrent* torrent = torrentById(*torrentId);
                            if (torrent) {
                                torrent->checkSingleFile(object);
                            }
                        }
                    }
                    maybeFinishUpdateOrConnection();
                }
            }
        );
    }

    void Rpc::getServerStats() {
        mRequestRouter->postRequest(
            "session-stats"_l1,
            QByteArrayLiteral("{\"method\":\"session-stats\"}"),
            RequestRouter::RequestType::DataUpdate,
            [=, this](const RequestRouter::Response& response) {
                if (response.success) {
                    mServerStats->update(response.arguments);
                    maybeFinishUpdateOrConnection();
                }
            }
        );
    }

    bool Rpc::checkIfUpdateCompleted() { return !mRequestRouter->hasPendingDataUpdateRequests(); }

    bool Rpc::checkIfConnectionCompleted() { return checkIfUpdateCompleted() && mServerIsLocal.has_value(); }

    void Rpc::maybeFinishUpdateOrConnection() {
        const bool connecting = connectionState() == ConnectionState::Connecting;
        if (!mUpdating && !connecting) return;
        if (mUpdating) {
            if (checkIfUpdateCompleted()) {
                debug().log("Finished updating data");
                mUpdating = false;
            } else {
                return;
            }
        }
        if (connecting) {
            if (checkIfConnectionCompleted()) {
                setStatus(Status{.connectionState = ConnectionState::Connected});
            } else {
                return;
            }
        }
        if (!mUpdateDisabled) {
            mUpdateTimer->start();
        }
    }

    void Rpc::checkIfServerIsLocal() {
        info().log("checkIfServerIsLocal() called");
        if (mServerSettings->data().hasSessionIdFile() && !mRequestRouter->sessionId().isEmpty() &&
            isTransmissionSessionIdFileExists(mRequestRouter->sessionId())) {
            mServerIsLocal = true;
            info().log("checkIfServerIsLocal: server is running locally: true");
            return;
        }
        const auto configuration = mRequestRouter->configuration();
        if (!configuration.has_value()) {
            return;
        }
        const auto host = configuration->serverUrl.host();
        if (auto localIp = isLocalIpAddress(host); localIp.has_value()) {
            mServerIsLocal = *localIp;
            info().log("checkIfServerIsLocal: server is running locally: {}", *mServerIsLocal);
            return;
        }
        info().log("checkIfServerIsLocal: resolving IP address for host name {}", host);
        mPendingHostInfoLookupId = QHostInfo::lookupHost(host, this, [=, this](const QHostInfo& hostInfo) {
            info().log("checkIfServerIsLocal: resolved IP address for host name {}", host);
            const auto addresses = hostInfo.addresses();
            if (!addresses.isEmpty()) {
                info().log("checkIfServerIsLocal: IP addresses:");
                for (const auto& address : addresses) {
                    info().log("checkIfServerIsLocal: - {}", address);
                }
                info().log("checkIfServerIsLocal: checking first address");
                mServerIsLocal = isLocalIpAddress(addresses.first());
            } else {
                mServerIsLocal = false;
            }
            info().log("checkIfServerIsLocal: server is running locally: {}", *mServerIsLocal);
            mPendingHostInfoLookupId = std::nullopt;
            maybeFinishUpdateOrConnection();
        });
    }
}
