// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rpc.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkProxy>
#include <QSslCertificate>
#include <QSslKey>

#include "addressutils.h"
#include "coroutines/hostinfo.h"
#include "coroutines/threadpool.h"
#include "coroutines/timer.h"
#include "coroutines/waitall.h"
#include "fileutils.h"
#include "jsonutils.h"
#include "itemlistupdater.h"
#include "log/log.h"
#include "requestrouter.h"
#include "serversettings.h"
#include "serverstats.h"
#include "torrent.h"

using namespace Qt::StringLiterals;

SPECIALIZE_FORMATTER_FOR_QDEBUG(QHostAddress)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf {
    using namespace impl;

    namespace {
        // Transmission 2.40+
        constexpr int minimumRpcVersion = 14;

        constexpr auto torrentsKey = "torrents"_L1;
        constexpr auto torrentDuplicateKey = "torrent-duplicate"_L1;
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
          mServerSettings(new ServerSettings(this, this)),
          mServerStats(new ServerStats(this)) {
        QObject::connect(mRequestRouter, &RequestRouter::requestFailed, this, &Rpc::onRequestFailed);
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

    void Rpc::setConnectionConfiguration(const ConnectionConfiguration& configuration) {
        disconnect();

        RequestRouter::RequestsConfiguration requestsConfig{};
        if (configuration.https) {
            requestsConfig.serverUrl.setScheme("https"_L1);
        } else {
            requestsConfig.serverUrl.setScheme("http"_L1);
        }
        requestsConfig.serverUrl.setHost(configuration.address);
        if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
            warning().log("Error setting URL hostname: {}", error);
        }
        requestsConfig.serverUrl.setPort(configuration.port);
        if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
            warning().log("Error setting URL port: {}", error);
        }
        if (auto queryIndex = configuration.apiPath.indexOf('?'); queryIndex != -1) {
            requestsConfig.serverUrl.setPath(configuration.apiPath.sliced(0, queryIndex));
            if (auto error = requestsConfig.serverUrl.errorString(); !error.isEmpty()) {
                warning().log("Error setting URL path: {}", error);
            }
            if ((queryIndex + 1) < configuration.apiPath.size()) {
                requestsConfig.serverUrl.setQuery(configuration.apiPath.sliced(queryIndex + 1));
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
        case ConnectionConfiguration::ProxyType::None:
            requestsConfig.proxy = QNetworkProxy(QNetworkProxy::NoProxy);
            break;
        }

        if (configuration.https) {
            switch (configuration.serverCertificateMode) {
            case ConnectionConfiguration::ServerCertificateMode::SelfSigned:
                requestsConfig.serverCertificate = RequestRouter::RequestsConfiguration::SelfSignedCertificate(
                    QSslCertificate(configuration.serverRootCertificate, QSsl::Pem)
                );
                break;
            case ConnectionConfiguration::ServerCertificateMode::CustomRoot:
                requestsConfig.serverCertificate = RequestRouter::RequestsConfiguration::CustomRoot{
                    .rootCertificate = QSslCertificate(configuration.serverRootCertificate, QSsl::Pem),
                    .leafCertificate = QSslCertificate(configuration.serverLeafCertificate, QSsl::Pem)
                };
                break;
            case ConnectionConfiguration::ServerCertificateMode::None:
                break;
            }
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

        mUpdateInterval = std::chrono::seconds(configuration.updateInterval);

        mAutoReconnectEnabled = configuration.autoReconnectEnabled;
        mAutoReconnectInterval = std::chrono::seconds(configuration.autoReconnectInterval);
    }

    void Rpc::resetConnectionConfiguration() {
        disconnect();
        mRequestRouter->resetConfiguration();
        mAutoReconnectEnabled = false;
        mAutoReconnectCoroutineScope.cancelAll();
    }

    void Rpc::connect() {
        if (connectionState() == ConnectionState::Disconnected && mRequestRouter->configuration().has_value()) {
            setStatus(Status{.connectionState = ConnectionState::Connecting});
            mBackgroundRequestsCoroutineScope.launch(connectAndPerformDataUpdates());
        }
    }

    void Rpc::disconnect() { setStatus(Status{.connectionState = ConnectionState::Disconnected}); }

    void Rpc::addTorrentFile(
        QString filePath,
        QString downloadDirectory,
        std::vector<int> unwantedFiles,
        std::vector<int> highPriorityFiles,
        std::vector<int> lowPriorityFiles,
        std::map<QString, QString> renamedFiles,
        TorrentData::Priority bandwidthPriority,
        bool start,
        DeleteFileMode deleteFileMode,
        std::vector<QString> labels
    ) {
        if (isConnected()) {
            mBackgroundRequestsCoroutineScope.launch(addTorrentFileImpl(
                std::move(filePath),
                std::move(downloadDirectory),
                std::move(unwantedFiles),
                std::move(highPriorityFiles),
                std::move(lowPriorityFiles),
                std::move(renamedFiles),
                bandwidthPriority,
                start,
                deleteFileMode,
                std::move(labels)
            ));
        }
    }

    namespace {
        std::optional<QByteArray> makeAddTorrentFileRequestData(
            const QString& filePath,
            const QString& downloadDirectory,
            const std::vector<int>& unwantedFiles,
            const std::vector<int>& highPriorityFiles,
            const std::vector<int>& lowPriorityFiles,
            TorrentData::Priority bandwidthPriority,
            bool start,
            const std::vector<QString>& labels
        ) {
            QString fileData{};
            try {
                QFile file(filePath);
                openFile(file, QIODevice::ReadOnly);
                fileData = readFileAsBase64String(file);
            } catch (const QFileError& e) {
                warning().logWithException(e, "addTorrentFile: failed to read torrent file");
                return std::nullopt;
            }
            QJsonObject arguments{
                {u"metainfo"_s, fileData},
                {u"download-dir"_s, downloadDirectory},
                {u"bandwidthPriority"_s, TorrentData::priorityToInt(bandwidthPriority)},
                {u"paused"_s, !start}
            };
            if (!unwantedFiles.empty()) {
                arguments.insert("files-unwanted"_L1, toJsonArray(unwantedFiles));
            }
            if (!highPriorityFiles.empty()) {
                arguments.insert("priority-high"_L1, toJsonArray(highPriorityFiles));
            }
            if (!lowPriorityFiles.empty()) {
                arguments.insert("priority-low"_L1, toJsonArray(lowPriorityFiles));
            }
            if (!labels.empty()) {
                arguments.insert("labels"_L1, toJsonArray(labels));
            }
            return RequestRouter::makeRequestData("torrent-add"_L1, std::move(arguments));
        }

        Coroutine<> deleteTorrentFile(QString filePath, bool moveToTrash) {
            co_await runOnThreadPool([moveToTrash, filePath = std::move(filePath)] {
                try {
                    if (moveToTrash) {
                        moveFileToTrashOrDelete(filePath);
                    } else {
                        deleteFile(filePath);
                    }
                } catch (const QFileError& e) {
                    warning().logWithException(e, "Failed to delete torrent file");
                }
            });
        }
    }

    Coroutine<> Rpc::addTorrentFileImpl(
        QString filePath,
        QString downloadDirectory,
        std::vector<int> unwantedFiles,
        std::vector<int> highPriorityFiles,
        std::vector<int> lowPriorityFiles,
        std::map<QString, QString> renamedFiles,
        TorrentData::Priority bandwidthPriority,
        bool start,
        DeleteFileMode deleteFileMode,
        std::vector<QString> labels
    ) {
        std::optional<QByteArray> requestData = co_await runOnThreadPool(
            makeAddTorrentFileRequestData,
            filePath,
            downloadDirectory,
            std::move(unwantedFiles),
            std::move(highPriorityFiles),
            std::move(lowPriorityFiles),
            bandwidthPriority,
            start,
            std::move(labels)
        );
        if (!requestData.has_value()) {
            emit torrentAddError(filePath);
            co_return;
        }
        if (deleteFileMode != DeleteFileMode::No) {
            mDeletingFilesCoroutineScope.launch(
                deleteTorrentFile(filePath, deleteFileMode == DeleteFileMode::MoveToTrash)
            );
        }
        if (!isConnected()) co_return;
        const auto response = co_await mRequestRouter->postRequest("torrent-add"_L1, std::move(requestData).value());
        if (response.arguments.contains(torrentDuplicateKey)) {
            emit torrentAddDuplicate();
        } else if (response.success) {
            if (!renamedFiles.empty()) {
                const auto torrentJson = response.arguments.value("torrent-added"_L1).toObject();
                const auto id = Torrent::idFromJson(torrentJson);
                if (id.has_value()) {
                    for (const auto& [filePathToRename, newName] : renamedFiles) {
                        renameTorrentFile(*id, filePathToRename, newName);
                    }
                }
            }
            mBackgroundRequestsCoroutineScope.launch(updateData());
        } else {
            emit torrentAddError(filePath);
        }
    }

    void Rpc::addTorrentLinks(
        QStringList links,
        QString downloadDirectory,
        TorrentData::Priority bandwidthPriority,
        bool start,
        std::vector<QString> labels
    ) {
        if (isConnected()) {
            mBackgroundRequestsCoroutineScope.launch(addTorrentLinksImpl(
                std::move(links),
                std::move(downloadDirectory),
                bandwidthPriority,
                start,
                std::move(labels)
            ));
        }
    }

    Coroutine<> Rpc::addTorrentLinksImpl(
        QStringList links,
        QString downloadDirectory,
        TorrentData::Priority bandwidthPriority,
        bool start,
        std::vector<QString> labels
    ) {
        const int priorityInt = TorrentData::priorityToInt(bandwidthPriority);
        co_await waitAll(
            links
            | std::views::transform([&](QString& link) {
                  return addTorrentLinkImpl(std::move(link), downloadDirectory, priorityInt, start, labels);
              })
            | std::ranges::to<std::vector>()
        );
        mBackgroundRequestsCoroutineScope.launch(updateData());
    }

    Coroutine<> Rpc::addTorrentLinkImpl(
        QString link, QString downloadDirectory, int bandwidthPriority, bool start, std::vector<QString> labels
    ) {
        QJsonObject arguments{
            {u"filename"_s, link},
            {u"download-dir"_s, downloadDirectory},
            {u"bandwidthPriority"_s, bandwidthPriority},
            {u"paused"_s, !start}
        };
        if (!labels.empty()) {
            arguments.insert("labels"_L1, toJsonArray(labels));
        }
        const auto response = co_await mRequestRouter->postRequest("torrent-add"_L1, std::move(arguments));
        if (response.arguments.contains(torrentDuplicateKey)) {
            emit torrentAddDuplicate();
        } else if (!response.success) {
            emit torrentAddError(link);
        }
    }

    void Rpc::startTorrents(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("torrent-start"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::startTorrentsNow(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("torrent-start-now"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::pauseTorrents(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("torrent-stop"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::removeTorrents(std::span<const int> ids, bool deleteFiles) {
        mBackgroundRequestsCoroutineScope.launch(
            postRequest("torrent-remove"_L1, {{u"ids"_s, toJsonArray(ids)}, {u"delete-local-data"_s, deleteFiles}})
        );
    }

    void Rpc::checkTorrents(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("torrent-verify"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::moveTorrentsToTop(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("queue-move-top"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::moveTorrentsUp(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("queue-move-up"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::moveTorrentsDown(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("queue-move-down"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::moveTorrentsToBottom(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("queue-move-bottom"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::reannounceTorrents(std::span<const int> ids) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("torrent-reannounce"_L1, {{u"ids"_s, toJsonArray(ids)}}));
    }

    void Rpc::setSessionProperty(QString property, QJsonValue value) {
        setSessionProperties({{property, std::move(value)}});
    }

    void Rpc::setSessionProperties(QJsonObject properties) {
        mBackgroundRequestsCoroutineScope.launch(postRequest("session-set"_L1, std::move(properties), false));
    }

    void Rpc::setTorrentProperty(int id, QString property, QJsonValue value, bool updateIfSuccessful) {
        mBackgroundRequestsCoroutineScope.launch(postRequest(
            "torrent-set"_L1,
            {{u"ids"_s, QJsonArray{id}}, {std::move(property), std::move(value)}},
            updateIfSuccessful
        ));
    }

    void Rpc::setTorrentsLocation(std::span<const int> ids, QString location, bool moveFiles) {
        mBackgroundRequestsCoroutineScope.launch(postRequest(
            "torrent-set-location"_L1,
            {{u"ids"_s, toJsonArray(ids)}, {u"location"_s, location}, {u"move"_s, moveFiles}}
        ));
    }

    void Rpc::setTorrentsLabels(std::span<const int> ids, std::span<const QString> labels) {
        mBackgroundRequestsCoroutineScope.launch(
            postRequest("torrent-set"_L1, {{u"ids"_s, toJsonArray(ids)}, {u"labels"_s, toJsonArray(labels)}})
        );
    }

    void Rpc::getTorrentFiles(int torrentId) {
        if (isConnected()) {
            mBackgroundRequestsCoroutineScope.launch(getTorrentsFiles({torrentId}));
        }
    }

    Coroutine<> Rpc::getTorrentsFiles(QJsonArray ids) {
        QJsonObject arguments{
            {u"fields"_s, QJsonArray{"id"_L1, "files"_L1, "fileStats"_L1}},
            {u"ids"_s, std::move(ids)}
        };
        const auto response = co_await mRequestRouter->postRequest("torrent-get"_L1, std::move(arguments));
        if (!response.success) co_return;
        const QJsonArray torrents = response.arguments.value(torrentsKey).toArray();
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
    }

    void Rpc::getTorrentPeers(int torrentId) {
        if (isConnected()) {
            mBackgroundRequestsCoroutineScope.launch(getTorrentsPeers({torrentId}));
        }
    }

    Coroutine<> Rpc::getTorrentsPeers(QJsonArray ids) {
        QJsonObject arguments{{u"fields"_s, QJsonArray{u"id"_s, u"peers"_s}}, {u"ids"_s, std::move(ids)}};
        const auto response = co_await mRequestRouter->postRequest("torrent-get"_L1, std::move(arguments));
        if (!response.success) co_return;
        const QJsonArray torrents = response.arguments.value(torrentsKey).toArray();
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
    }

    void Rpc::renameTorrentFile(int torrentId, QString filePath, QString newName) {
        if (isConnected()) {
            mBackgroundRequestsCoroutineScope.launch(
                renameTorrentFileImpl(torrentId, std::move(filePath), std::move(newName))
            );
        }
    }

    Coroutine<> Rpc::renameTorrentFileImpl(int torrentId, QString filePath, QString newName) {
        QJsonObject arguments = {{u"ids"_s, QJsonArray{torrentId}}, {u"path"_s, filePath}, {u"name"_s, newName}};
        const auto response = co_await mRequestRouter->postRequest("torrent-rename-path"_L1, std::move(arguments));
        if (response.success) {
            Torrent* torrent = torrentById(torrentId);
            if (torrent) {
                const QString filePathFromReponse = response.arguments.value("path"_L1).toString();
                const QString newNameFromReponse = response.arguments.value("name"_L1).toString();
                emit torrent->fileRenamed(filePathFromReponse, newNameFromReponse);
                mBackgroundRequestsCoroutineScope.launch(updateData());
            }
        }
    }

    Coroutine<std::optional<qint64>> Rpc::getDownloadDirFreeSpace() {
        if (isConnected()) {
            if (mServerSettings->data().canShowFreeSpaceForPath()) {
                co_return co_await getFreeSpaceForPathImpl(mServerSettings->data().downloadDirectory);
            }
            co_return co_await getDownloadDirFreeSpaceImpl();
        }
        cancelCoroutine();
    }

    Coroutine<std::optional<qint64>> Rpc::getDownloadDirFreeSpaceImpl() {
        const auto response = co_await mRequestRouter->postRequest(
            "download-dir-free-space"_L1,

            "{"
            "\"arguments\":{"
            "\"fields\":["
            "\"download-dir-free-space\""
            "]"
            "},"
            "\"method\":\"session-get\""
            "}"_ba

        );
        if (response.success) {
            co_return response.arguments.value("download-dir-free-space"_L1).toInteger();
        }
        co_return std::nullopt;
    }

    Coroutine<std::optional<qint64>> Rpc::getFreeSpaceForPath(QString path) {
        if (isConnected()) {
            if (mServerSettings->data().canShowFreeSpaceForPath()) {
                co_return co_await getFreeSpaceForPathImpl(std::move(path));
            }
            if (path == mServerSettings->data().downloadDirectory) {
                co_return co_await getDownloadDirFreeSpaceImpl();
            }
        }
        cancelCoroutine();
    }

    Coroutine<std::optional<qint64>> Rpc::getFreeSpaceForPathImpl(QString path) {
        QJsonObject arguments{{u"path"_s, path}};
        const auto response = co_await mRequestRouter->postRequest("free-space"_L1, std::move(arguments));
        if (response.success) {
            co_return response.arguments.value("size-bytes"_L1).toInteger();
        }
        co_return std::nullopt;
    }

    void Rpc::shutdownServer() {
        if (isConnected()) {
            mBackgroundRequestsCoroutineScope.launch(shutdownServerImpl());
        }
    }

    Coroutine<> Rpc::shutdownServerImpl() {
        const auto response = co_await mRequestRouter->postRequest("session-close"_L1, QJsonObject{});
        if (response.success) {
            info().log("Successfully sent shutdown request, disconnecting");
            disconnect();
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

        if (connectionStateChanged) {
            resetStateOnConnectionStateChanged(oldStatus.connectionState);
        }

        emit statusChanged();

        if (connectionStateChanged) {
            emitSignalsOnConnectionStateChanged(oldStatus.connectionState);
        }
    }

    void Rpc::resetStateOnConnectionStateChanged(ConnectionState oldConnectionState) {
        switch (mStatus.connectionState) {
        case ConnectionState::Disconnected: {
            info().log("Disconnected");

            mBackgroundRequestsCoroutineScope.cancelAll();
            mRequestRouter->abortNetworkRequestsAndClearSessionId();
            mServerIsLocal = std::nullopt;
            mGetTorrentsRequestData.clear();

            if (!mTorrents.empty() && oldConnectionState == ConnectionState::Connected) {
                const auto removedTorrentsCount = mTorrents.size();
                emit onAboutToRemoveTorrents(0, removedTorrentsCount);
                mTorrents.clear();
                emit onRemovedTorrents(0, removedTorrentsCount);
            }

            if (error() != RpcError::NoError && mAutoReconnectEnabled) {
                mAutoReconnectCoroutineScope.launch(autoReconnect());
            }

            break;
        }
        case ConnectionState::Connecting:
            info().log("Connecting");
            mAutoReconnectCoroutineScope.cancelAll();
            break;
        case ConnectionState::Connected: {
            info().log("Connected");
            break;
        }
        }
    }

    void Rpc::emitSignalsOnConnectionStateChanged(Rpc::ConnectionState oldConnectionState) {
        switch (mStatus.connectionState) {
        case ConnectionState::Disconnected: {
            emit connectionStateChanged();
            if (oldConnectionState == ConnectionState::Connected) {
                emit connectedChanged();
                emit torrentsUpdated();
            }
            break;
        }
        case ConnectionState::Connecting:
            emit connectionStateChanged();
            break;
        case ConnectionState::Connected: {
            emit torrentsUpdated();
            emit connectionStateChanged();
            emit connectedChanged();
            break;
        }
        }
    }

    Coroutine<> Rpc::postRequest(QLatin1String method, QJsonObject arguments, bool updateIfSuccessful) {
        if (isConnected()) {
            const auto response = co_await mRequestRouter->postRequest(method, std::move(arguments));
            if (updateIfSuccessful && response.success) {
                mBackgroundRequestsCoroutineScope.launch(updateData());
            }
        }
    }

    Coroutine<> Rpc::getServerSettings() {
        const auto response = co_await mRequestRouter->postRequest("session-get"_L1, "{\"method\":\"session-get\"}"_ba);
        if (response.success) {
            mServerSettings->update(response.arguments);
            if (mServerSettings->data().hasTableMode()) {
                mGetTorrentsRequestData = RequestRouter::makeRequestData(
                    "torrent-get"_L1,
                    QJsonObject{{u"fields"_s, Torrent::updateFields(mServerSettings)}, {u"format"_s, "table"_L1}}
                );
            } else {
                mGetTorrentsRequestData = RequestRouter::makeRequestData(
                    "torrent-get"_L1,
                    QJsonObject{{u"fields"_s, Torrent::updateFields(mServerSettings)}}
                );
            }
        }
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
                if (!wasFinished
                    && torrent->data().isFinished()
                    && !wasPaused
                    && torrent->data().sizeWhenDone >= oldSizeWhenDone) {
                    emit mRpc.torrentFinished(torrent.get());
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

    Coroutine<> Rpc::getTorrents() {
        const auto response = co_await mRequestRouter->postRequest("torrent-get"_L1, mGetTorrentsRequestData);
        if (!response.success) co_return;

        TorrentsListUpdater updater(*this);
        {
            const QJsonArray torrentsJsons = response.arguments.value("torrents"_L1).toArray();
            std::vector<NewTorrent> newTorrents{};
            if (mServerSettings->data().hasTableMode()) {
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

        std::vector<Coroutine<>> additionalRequests{};
        QJsonArray getFilesIds{};
        QJsonArray getPeersIds{};
        for (const auto& torrent : mTorrents) {
            if (torrent->isFilesEnabled()) {
                getFilesIds.push_back(torrent->data().id);
            }
            if (torrent->isPeersEnabled()) {
                getPeersIds.push_back(torrent->data().id);
            }
        }
        if (!getFilesIds.empty()) {
            additionalRequests.push_back(getTorrentsFiles(getFilesIds));
        }
        if (!getPeersIds.empty()) {
            additionalRequests.push_back(getTorrentsPeers(getPeersIds));
        }
        if (!additionalRequests.empty()) {
            co_await waitAll(std::move(additionalRequests));
        }

        const bool wasConnecting = connectionState() == ConnectionState::Connecting;
        if (!wasConnecting) {
            emit torrentsUpdated();
        }
    }

    Coroutine<> Rpc::getServerStats() {
        const auto response =
            co_await mRequestRouter->postRequest("session-stats"_L1, "{\"method\":\"session-stats\"}"_ba);
        if (response.success) {
            mServerStats->update(response.arguments);
        }
    }

    Coroutine<> Rpc::connectAndPerformDataUpdates() {
        co_await getServerSettings();
        if (mServerSettings->data().minimumRpcVersion > minimumRpcVersion) {
            setStatus(Status{.connectionState = ConnectionState::Disconnected, .error = Error::ServerIsTooNew});
            co_return;
        }
        if (mServerSettings->data().rpcVersion < minimumRpcVersion) {
            setStatus(Status{.connectionState = ConnectionState::Disconnected, .error = Error::ServerIsTooOld});
            co_return;
        }
        co_await waitAll(checkIfServerIsLocal(), getTorrents(), getServerStats());
        setStatus(Status{.connectionState = ConnectionState::Connected});
        while (true) {
            co_await waitFor(mUpdateInterval);
            co_await updateData();
        }
    }

    Coroutine<> Rpc::updateData() {
        debug().log("Updating data");
        co_await waitAll(getServerSettings(), getTorrents(), getServerStats());
        debug().log("Finished updating data");
    }

    Coroutine<> Rpc::checkIfServerIsLocal() {
        info().log("checkIfServerIsLocal() called");
        if (mServerSettings->data().hasSessionIdFile()
            && !mRequestRouter->sessionId().isEmpty()
            && isTransmissionSessionIdFileExists(mRequestRouter->sessionId())) {
            mServerIsLocal = true;
            info().log("checkIfServerIsLocal: server is running locally: true");
            co_return;
        }
        const auto configuration = mRequestRouter->configuration();
        if (!configuration.has_value()) {
            co_return;
        }
        const auto host = configuration->serverUrl.host();
        if (auto localIp = isLocalIpAddress(host); localIp.has_value()) {
            mServerIsLocal = localIp;
            info().log("checkIfServerIsLocal: server is running locally: {}", *mServerIsLocal);
            co_return;
        }
        info().log("checkIfServerIsLocal: resolving IP address for host name {}", host);
        const QHostInfo hostInfo = co_await lookupHost(host);
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
    }

    void Rpc::onRequestFailed(RpcError error, const QString& errorMessage, const QString& detailedErrorMessage) {
        setStatus({RpcConnectionState::Disconnected, error, errorMessage, detailedErrorMessage});
    }

    Coroutine<> Rpc::autoReconnect() {
        info().log("Auto reconnecting in {} seconds", mAutoReconnectInterval.count());
        co_await waitFor(mAutoReconnectInterval);
        info().log("Auto reconnection");
        connect();
    }
}
