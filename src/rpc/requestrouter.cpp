// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "requestrouter.h"

#include <optional>
#include <utility>

#include <QAuthenticator>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QObject>
#include <QNetworkReply>
#include <QSslSocket>
#include <QtConcurrentRun>

#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QNetworkReply::NetworkError)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(QSslError::SslError)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QJsonObject)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QNetworkProxy)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QSslError)

namespace fmt {
    template<>
    struct formatter<QSsl::SslProtocol> : tremotesf::SimpleFormatter {
        fmt::format_context::iterator format(QSsl::SslProtocol protocol, fmt::format_context& ctx) FORMAT_CONST {
            const auto str = [&]() -> std::optional<std::string_view> {
                switch (protocol) {
                case QSsl::TlsV1_0:
                    return "TlsV1_0";
                case QSsl::TlsV1_1:
                    return "TlsV1_1";
                case QSsl::TlsV1_2:
                    return "TlsV1_2";
                case QSsl::AnyProtocol:
                    return "AnyProtocol";
                case QSsl::SecureProtocols:
                    return "SecureProtocols";
                case QSsl::TlsV1_0OrLater:
                    return "TlsV1_0OrLater";
                case QSsl::TlsV1_1OrLater:
                    return "TlsV1_1OrLater";
                case QSsl::TlsV1_2OrLater:
                    return "TlsV1_2OrLater";
                case QSsl::DtlsV1_0:
                    return "DtlsV1_0";
                case QSsl::DtlsV1_0OrLater:
                    return "DtlsV1_0OrLater";
                case QSsl::DtlsV1_2:
                    return "DtlsV1_2";
                case QSsl::DtlsV1_2OrLater:
                    return "DtlsV1_2OrLater";
                case QSsl::TlsV1_3:
                    return "TlsV1_3";
                case QSsl::TlsV1_3OrLater:
                    return "TlsV1_3OrLater";
                case QSsl::UnknownProtocol:
                    return "UnknownProtocol";
#if QT_VERSION_MAJOR < 6
                case QSsl::SslV3:
                    return "SslV3";
                case QSsl::SslV2:
                    return "SslV2";
                case QSsl::TlsV1SslV3:
                    return "TlsV1SslV3";
#endif
                }
                return std::nullopt;
            }();
            if (str) {
                return fmt::format_to(ctx.out(), tremotesf::impl::singleArgumentFormatString, *str);
            }
            return fmt::format_to(
                ctx.out(),
                tremotesf::impl::singleArgumentFormatString,
                static_cast<std::underlying_type_t<QSsl::SslProtocol>>(protocol)
            );
        }
    };
}

namespace tremotesf::impl {
    namespace {
        const auto sessionIdHeader = QByteArrayLiteral("X-Transmission-Session-Id");
        const auto authorizationHeader = QByteArrayLiteral("Authorization");

        constexpr auto metadataProperty = "tremotesf::impl::RequestRouter metadata";

        QJsonObject getReplyArguments(const QJsonObject& parseResult) {
            return parseResult.value("arguments"_l1).toObject();
        }

        bool isResultSuccessful(const QJsonObject& parseResult) {
            return (parseResult.value("result"_l1).toString() == "success"_l1);
        }

        using ParseFutureWatcher = QFutureWatcher<std::optional<QJsonObject>>;
    }

    struct RpcRequestMetadata {
        QLatin1String method{};
        RequestRouter::RequestType type{};
        std::function<void(RequestRouter::Response)> onResponse{};
    };

    struct NetworkRequestMetadata {
        QByteArray postData{};
        int retryAttempts{};
        RpcRequestMetadata rpcMetadata{};
    };

    RequestRouter::RequestRouter(QThreadPool* threadPool, QObject* parent)
        : QObject(parent),
          mNetwork(new QNetworkAccessManager(this)),
          mThreadPool(threadPool ? threadPool : QThreadPool::globalInstance()) {
        mNetwork->setAutoDeleteReplies(true);
    }

    RequestRouter::RequestRouter(QObject* parent) : RequestRouter(nullptr, parent) {}

    void RequestRouter::setConfiguration(RequestsConfiguration configuration) {
        logDebug("Setting requests configuration");

        mConfiguration = std::move(configuration);

        mNetwork->setProxy(mConfiguration->proxy);
        mNetwork->clearAccessCache();

        const bool https = mConfiguration->serverUrl.scheme() == "https"_l1;

        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        if (https) {
            if (!mConfiguration->clientCertificate.isNull()) {
                mSslConfiguration.setLocalCertificate(mConfiguration->clientCertificate);
            }
            if (!mConfiguration->clientPrivateKey.isNull()) {
                mSslConfiguration.setPrivateKey(mConfiguration->clientPrivateKey);
            }
            mExpectedSslErrors.clear();
            mExpectedSslErrors.reserve(mConfiguration->serverCertificateChain.size() * 3);
            for (const auto& certificate : mConfiguration->serverCertificateChain) {
                mExpectedSslErrors.push_back(QSslError(QSslError::HostNameMismatch, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificate, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificateInChain, certificate));
            }
        }

        if (!mConfiguration->serverUrl.isEmpty()) {
            logDebug("Connection configuration:");
            logDebug(" - Server url: {}", mConfiguration->serverUrl.toString());
            if (mConfiguration->proxy.type() != QNetworkProxy::NoProxy) {
                logDebug(" - Proxy: {}", mConfiguration->proxy);
            }
            logDebug(" - Timeout: {}", mConfiguration->timeout);
            logDebug(" - HTTP Basic access authentication: {}", mConfiguration->authentication);
            if (mConfiguration->authentication) {
                auto base64Credentials = QString("%1:%2")
                                             .arg(mConfiguration->username, mConfiguration->password)
                                             .normalized(QString::NormalizationForm_C)
                                             .toUtf8()
                                             .toBase64();
                mAuthorizationHeaderValue = QByteArray("Basic ").append(base64Credentials);
            }
            if (https) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
                logDebug(" - Available TLS backends: {}", QSslSocket::availableBackends());
                logDebug(" - Active TLS backend: {}", QSslSocket::activeBackend());
                logDebug(" - Supported TLS protocols: {}", QSslSocket::supportedProtocols());
#endif
                logDebug(" - TLS library version: {}", QSslSocket::sslLibraryVersionString());
                logDebug(
                    " - Manually validating server's certificate chain: {}",
                    !mConfiguration->serverCertificateChain.isEmpty()
                );
                logDebug(
                    " - Client certificate authentication: {}",
                    !mConfiguration->clientCertificate.isNull() && !mConfiguration->clientPrivateKey.isNull()
                );
            }
        }
    }

    void RequestRouter::resetConfiguration() {
        logDebug("Resetting requests configuration");
        mConfiguration.reset();
        mNetwork->clearAccessCache();
    }

    void RequestRouter::postRequest(
        QLatin1String method, const QJsonObject& arguments, RequestType type, std::function<void(Response)>&& onResponse
    ) {
        postRequest(method, makeRequestData(method, arguments), type, std::move(onResponse));
    }

    void RequestRouter::postRequest(
        QLatin1String method, const QByteArray& data, RequestType type, std::function<void(Response)>&& onResponse
    ) {
        if (!mConfiguration.has_value()) {
            logWarning("Requests configuration is not set");
            return;
        }

        QNetworkRequest request(mConfiguration->serverUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"_l1);
        request.setSslConfiguration(mSslConfiguration);
        request.setTransferTimeout(static_cast<int>(mConfiguration->timeout.count()));
        NetworkRequestMetadata metadata{};
        metadata.postData = data;
        metadata.rpcMetadata = {method, type, std::move(onResponse)};
        postRequest(request, metadata);
    }

    bool RequestRouter::hasPendingDataUpdateRequests() const {
        return std::any_of(
                   mPendingNetworkRequests.begin(),
                   mPendingNetworkRequests.end(),
                   [](const auto* reply) {
                       const auto metadata = reply->property(metadataProperty).template value<NetworkRequestMetadata>();
                       return metadata.rpcMetadata.type == RequestType::DataUpdate;
                   }
               ) ||
               std::any_of(mPendingParseFutures.begin(), mPendingParseFutures.end(), [](const auto* future) {
                   const auto metadata = future->property(metadataProperty).template value<RpcRequestMetadata>();
                   return metadata.type == RequestType::DataUpdate;
               });
    }

    void RequestRouter::cancelPendingRequestsAndClearSessionId() {
        for (QNetworkReply* reply : std::unordered_set(std::move(mPendingNetworkRequests))) {
            reply->abort();
        }
        for (QObject* futureWatcher : std::unordered_set(std::move(mPendingParseFutures))) {
            static_cast<ParseFutureWatcher*>(futureWatcher)->cancel();
            futureWatcher->deleteLater();
        }
        mSessionId.clear();
    }

    QByteArray RequestRouter::makeRequestData(const QString& method, const QJsonObject& arguments) {
        return QJsonDocument(QJsonObject{
                                 {QStringLiteral("method"), method},
                                 {QStringLiteral("arguments"), arguments},
                             })
            .toJson(QJsonDocument::Compact);
    }

    void RequestRouter::postRequest(QNetworkRequest request, const NetworkRequestMetadata& metadata) {
        if (!mSessionId.isEmpty()) {
            request.setRawHeader(sessionIdHeader, mSessionId);
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        if (mConfiguration->authentication) {
            request.setRawHeader(authorizationHeader, mAuthorizationHeaderValue);
        }
        QNetworkReply* reply = mNetwork->post(request, metadata.postData);
        reply->setProperty(metadataProperty, QVariant::fromValue(metadata));
        mPendingNetworkRequests.insert(reply);

        reply->ignoreSslErrors(mExpectedSslErrors);
        auto sslErrors = std::make_shared<QList<QSslError>>();

        QObject::connect(reply, &QNetworkReply::sslErrors, this, [=, this](const QList<QSslError>& errors) {
            for (const QSslError& error : errors) {
                if (!mExpectedSslErrors.contains(error)) {
                    sslErrors->push_back(error);
                }
            }
        });

        QObject::connect(reply, &QNetworkReply::finished, this, [=, this]() mutable {
            onRequestFinished(reply, *sslErrors);
        });
    }

    bool RequestRouter::retryRequest(const QNetworkRequest& request, NetworkRequestMetadata metadata) {
        if (!mConfiguration.has_value()) {
            logWarning("Not retrying request, requests configuration is not set");
            return false;
        }
        metadata.retryAttempts++;
        if (metadata.retryAttempts > mConfiguration->retryAttempts) {
            return false;
        }
        logWarning("Retrying '{}' request, retry attempts = {}", metadata.rpcMetadata.method, metadata.retryAttempts);
        postRequest(request, metadata);
        return true;
    }

    void RequestRouter::onRequestFinished(QNetworkReply* reply, const QList<QSslError>& sslErrors) {
        if (mPendingNetworkRequests.erase(reply) == 0) {
            // Request was cancelled
            return;
        }
        const auto metadata = reply->property(metadataProperty).value<NetworkRequestMetadata>();
        if (reply->error() == QNetworkReply::NoError) {
            onRequestSuccess(reply, metadata.rpcMetadata);
        } else {
            onRequestError(reply, sslErrors, metadata);
        }
    }

    void RequestRouter::onRequestSuccess(QNetworkReply* reply, const RpcRequestMetadata& metadata) {
        logDebug(
            "HTTP request for method '{}' succeeded, HTTP status code: {} {}",
            metadata.method,
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
            reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()
        );

        const auto future =
            QtConcurrent::run(mThreadPool, [replyData = reply->readAll()]() -> std::optional<QJsonObject> {
                QJsonParseError error{};
                QJsonObject json = QJsonDocument::fromJson(replyData, &error).object();
                if (error.error != QJsonParseError::NoError) {
                    logWarning(
                        "Failed to parse JSON reply from server:\n{}\nError '{}' at offset {}",
                        replyData,
                        error.errorString(),
                        error.offset
                    );
                    return {};
                }
                return json;
            });
        auto watcher = new ParseFutureWatcher(this);
        watcher->setProperty(metadataProperty, QVariant::fromValue(metadata));
        QObject::connect(watcher, &ParseFutureWatcher::finished, this, [=, this] {
            if (!mPendingParseFutures.erase(watcher)) {
                // Future was cancelled
                return;
            }
            auto json = watcher->result();
            if (json.has_value()) {
                const bool success = isResultSuccessful(*json);
                if (!success) {
                    logWarning("method '{}' failed, response: {}", metadata.method, *json);
                }
                if (metadata.onResponse) {
                    metadata.onResponse({.arguments = getReplyArguments(*json), .success = success});
                }
            } else {
                emit requestFailed(RpcError::ParseError, {}, {});
            }
            watcher->deleteLater();
        });
        mPendingParseFutures.insert(watcher);
        watcher->setFuture(future);
    }

    void RequestRouter::onRequestError(
        QNetworkReply* reply, const QList<QSslError>& sslErrors, const NetworkRequestMetadata& metadata
    ) {
        if (reply->error() == QNetworkReply::ContentConflictError && reply->hasRawHeader(sessionIdHeader)) {
            QByteArray newSessionId = reply->rawHeader(sessionIdHeader);
            // Check against session id of request instead of current session id,
            // to handle case when current session id have already been overwritten by another failed request
            if (newSessionId != reply->request().rawHeader(sessionIdHeader)) {
                if (!mSessionId.isEmpty()) {
                    logInfo("Session id changed");
                }
                logDebug("Session id is {}, retrying '{}' request", newSessionId, metadata.rpcMetadata.method);
                mSessionId = std::move(newSessionId);
                // Retry without incrementing retryAttempts
                postRequest(reply->request(), metadata);
                return;
            }
        }

        const QString detailedErrorMessage = makeDetailedErrorMessage(reply, sslErrors);
        logWarning("HTTP request for method '{}' failed:\n{}", metadata.rpcMetadata.method, detailedErrorMessage);
        switch (reply->error()) {
        case QNetworkReply::AuthenticationRequiredError:
            logWarning("Authentication error");
            emit requestFailed(RpcError::AuthenticationError, reply->errorString(), detailedErrorMessage);
            break;
        case QNetworkReply::OperationCanceledError:
        case QNetworkReply::TimeoutError:
            logWarning("Timed out");
            if (!retryRequest(reply->request(), metadata)) {
                emit requestFailed(RpcError::TimedOut, reply->errorString(), detailedErrorMessage);
            }
            break;
        default: {
            if (!retryRequest(reply->request(), metadata)) {
                emit requestFailed(RpcError::ConnectionError, reply->errorString(), detailedErrorMessage);
            }
        }
        }
    }

    QString RequestRouter::makeDetailedErrorMessage(QNetworkReply* reply, const QList<QSslError>& sslErrors) {
        auto detailedErrorMessage = QString::fromStdString(fmt::format("{}: {}", reply->error(), reply->errorString()));
        if (reply->url() == reply->request().url()) {
            detailedErrorMessage += QString::fromStdString(fmt::format("\nURL: {}", reply->url().toString()));
        } else {
            detailedErrorMessage += QString::fromStdString(fmt::format(
                "\nOriginal URL: {}\nRedirected URL: {}",
                reply->request().url().toString(),
                reply->url().toString()
            ));
        }
        if (auto httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            httpStatusCode.isValid()) {
            detailedErrorMessage += QString::fromStdString(fmt::format(
                "\nHTTP status code: {} {}\nConnection was encrypted: {}",
                httpStatusCode.toInt(),
                reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                reply->attribute(QNetworkRequest::ConnectionEncryptedAttribute).toBool()
            ));
            if (!reply->rawHeaderPairs().isEmpty()) {
                detailedErrorMessage += "\nReply headers:"_l1;
                for (const QNetworkReply::RawHeaderPair& pair : reply->rawHeaderPairs()) {
                    detailedErrorMessage += QString::fromStdString(fmt::format("\n  {}: {}", pair.first, pair.second));
                }
            }
        } else {
            detailedErrorMessage += "\nDid not establish HTTP connection"_l1;
        }
        if (!sslErrors.isEmpty()) {
            detailedErrorMessage += QString::fromStdString(fmt::format("\n\n{} TLS errors:", sslErrors.size()));
            int i = 1;
            for (const QSslError& sslError : sslErrors) {
                detailedErrorMessage += QString::fromStdString(fmt::format(
                    "\n\n {}. {}: {} on certificate:\n - {}",
                    i,
                    sslError.error(),
                    sslError.errorString(),
                    sslError.certificate().toText()
                ));
                ++i;
            }
        }
        return detailedErrorMessage;
    }
}

Q_DECLARE_METATYPE(tremotesf::impl::RpcRequestMetadata)
Q_DECLARE_METATYPE(tremotesf::impl::NetworkRequestMetadata)
