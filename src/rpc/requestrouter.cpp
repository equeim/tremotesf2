// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Needed to access deprecated QSsl::SslProtocol enum values
#undef QT_DISABLE_DEPRECATED_BEFORE
#undef QT_DISABLE_DEPRECATED_UP_TO

#include "requestrouter.h"

#include <optional>

#include <QJsonDocument>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>

#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include "coroutines/network.h"
#include "coroutines/threadpool.h"
#include "log/log.h"
#include "pragmamacros.h"
#include "rpc.h"

using namespace Qt::StringLiterals;

DISABLE_RANGE_FORMATTING(QJsonObject)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QJsonObject)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QNetworkProxy)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QSslError)

namespace fmt {
    template<>
    struct formatter<QSslCertificate> : tremotesf::SimpleFormatter {
        fmt::format_context::iterator format(const QSslCertificate& certificate, fmt::format_context& ctx) const {
            // QSslCertificate::toText is implemented only for OpenSSL backend
            static const bool isOpenSSL = (QSslSocket::activeBackend() == "openssl"_L1);
            if (!isOpenSSL) {
                return tremotesf::impl::QDebugFormatter<QSslCertificate>{}.format(certificate, ctx);
            }
            return fmt::formatter<QString>{}.format(certificate.toText(), ctx);
        }
    };

    template<>
    struct formatter<QSsl::SslProtocol> : tremotesf::SimpleFormatter {
        fmt::format_context::iterator format(QSsl::SslProtocol protocol, fmt::format_context& ctx) const {
            const auto str = [&]() -> std::optional<std::string_view> {
                SUPPRESS_DEPRECATED_WARNINGS_BEGIN
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
                }
                SUPPRESS_DEPRECATED_WARNINGS_END
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
    struct NetworkReplyDeleter {
        inline void operator()(QNetworkReply* reply) { reply->deleteLater(); }
    };

    namespace {
        const auto sessionIdHeader = QByteArrayLiteral("X-Transmission-Session-Id");
        const auto authorizationHeader = QByteArrayLiteral("Authorization");

        QJsonObject getReplyArguments(const QJsonObject& parseResult) {
            return parseResult.value("arguments"_L1).toObject();
        }

        bool isResultSuccessful(const QJsonObject& parseResult) {
            return (parseResult.value("result"_L1).toString() == "success"_L1);
        }

        QString httpStatus(QNetworkReply* reply) {
            const auto statusCodeAttr = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            if (!statusCodeAttr.isValid()) {
                return {};
            }
            auto status = QString::number(statusCodeAttr.toInt());
            const auto reasonAttr = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
            if (!reasonAttr.isValid()) {
                return status;
            }
            const auto reason = QString::fromUtf8(reasonAttr.toByteArray());
            if (reason.isEmpty()) {
                return status;
            }
            status += ' ';
            status += reason;
            return status;
        }

        QString makeDetailedErrorMessage(QNetworkReply* reply, const QList<QSslError>& sslErrors) {
            auto detailedErrorMessage =
                QString::fromStdString(fmt::format("{}: {}", reply->error(), reply->errorString()));
            if (reply->url() == reply->request().url()) {
                detailedErrorMessage += QString::fromStdString(fmt::format("\nURL: {}", reply->url().toString()));
            } else {
                detailedErrorMessage += QString::fromStdString(
                    fmt::format(
                        "\nOriginal URL: {}\nRedirected URL: {}",
                        reply->request().url().toString(),
                        reply->url().toString()
                    )
                );
            }
            if (const auto status = httpStatus(reply); !status.isEmpty()) {
                detailedErrorMessage += QString::fromStdString(
                    fmt::format(
                        "\nHTTP status: {}\nConnection was encrypted: {}",
                        status,
                        reply->attribute(QNetworkRequest::ConnectionEncryptedAttribute).toBool()
                    )
                );
                if (!reply->rawHeaderPairs().isEmpty()) {
                    detailedErrorMessage += "\nReply headers:"_L1;
                    for (const QNetworkReply::RawHeaderPair& pair : reply->rawHeaderPairs()) {
                        detailedErrorMessage +=
                            QString::fromStdString(fmt::format("\n  {}: {}", pair.first, pair.second));
                    }
                }
            } else {
                detailedErrorMessage += "\nDid not establish HTTP connection"_L1;
            }
            if (!sslErrors.isEmpty()) {
                detailedErrorMessage += QString::fromStdString(fmt::format("\n\n{} TLS errors:", sslErrors.size()));
                int i = 1;
                for (const QSslError& sslError : sslErrors) {
                    detailedErrorMessage += QString::fromStdString(
                        fmt::format(
                            "\n\n {}. {}: {} on certificate:\n - {}",
                            i,
                            sslError.error(),
                            sslError.errorString(),
                            sslError.certificate()
                        )
                    );
                    ++i;
                }
            }
            return detailedErrorMessage;
        }

        QNetworkRequest takeRequest(NetworkReplyUniquePtr reply) { return reply->request(); }
    }

    struct RpcRequestMetadata {
        QLatin1String method{};
    };

    struct NetworkRequestMetadata {
        QByteArray postData{};
        int retryAttempts{};
        RpcRequestMetadata rpcMetadata{};
    };

    RequestRouter::RequestRouter(QThreadPool* threadPool, QObject* parent)
        : QObject(parent),
          mNetwork(new QNetworkAccessManager(this)),
          mThreadPool(threadPool ? threadPool : QThreadPool::globalInstance()) {}

    RequestRouter::RequestRouter(QObject* parent) : RequestRouter(nullptr, parent) {}

    void RequestRouter::setConfiguration(RequestsConfiguration configuration) {
        debug().log("Setting requests configuration");

        mConfiguration = std::move(configuration);

        mNetwork->setProxy(mConfiguration->proxy);
        mNetwork->clearAccessCache();

        const bool https = mConfiguration->serverUrl.scheme() == "https"_L1;

        mSslConfiguration = QSslConfiguration::defaultConfiguration();
        if (https) {
            if (!mConfiguration->clientCertificate.isNull()) {
                mSslConfiguration.setLocalCertificate(mConfiguration->clientCertificate);
            }
            if (!mConfiguration->clientPrivateKey.isNull()) {
                mSslConfiguration.setPrivateKey(mConfiguration->clientPrivateKey);
            }
            mExpectedSslErrors.clear();
            mExpectedSslErrors.reserve(mConfiguration->serverCertificateChain.size() * 4);
            for (const auto& certificate : mConfiguration->serverCertificateChain) {
                mExpectedSslErrors.push_back(QSslError(QSslError::HostNameMismatch, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificate, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::SelfSignedCertificateInChain, certificate));
                mExpectedSslErrors.push_back(QSslError(QSslError::CertificateUntrusted, certificate));
            }
        }

        if (!mConfiguration->serverUrl.isEmpty()) {
            debug().log("Connection configuration:");
            debug().log(" - Server url: {}", mConfiguration->serverUrl.toString());
            if (mConfiguration->proxy.type() != QNetworkProxy::NoProxy) {
                debug().log(" - Proxy: {}", mConfiguration->proxy);
            }
            debug().log(" - Timeout: {}", mConfiguration->timeout);
            debug().log(" - HTTP Basic access authentication: {}", mConfiguration->authentication);
            if (mConfiguration->authentication) {
                auto base64Credentials = QString("%1:%2")
                                             .arg(mConfiguration->username, mConfiguration->password)
                                             .normalized(QString::NormalizationForm_C)
                                             .toUtf8()
                                             .toBase64();
                mAuthorizationHeaderValue = QByteArray("Basic ").append(base64Credentials);
            }
            if (https) {
                debug().log(" - Available TLS backends: {}", QSslSocket::availableBackends());
                debug().log(" - Active TLS backend: {}", QSslSocket::activeBackend());
                debug().log(" - Supported TLS protocols: {}", QSslSocket::supportedProtocols());
                debug().log(" - TLS library version: {}", QSslSocket::sslLibraryVersionString());
                debug().log(
                    " - Manually validating server's certificate chain: {}",
                    !mConfiguration->serverCertificateChain.isEmpty()
                );
                debug().log(
                    " - Client certificate authentication: {}",
                    !mConfiguration->clientCertificate.isNull() && !mConfiguration->clientPrivateKey.isNull()
                );
            }
        }
    }

    void RequestRouter::resetConfiguration() {
        debug().log("Resetting requests configuration");
        mConfiguration.reset();
        mNetwork->clearAccessCache();
    }

    Coroutine<RequestRouter::Response> RequestRouter::postRequest(QLatin1String method, QJsonObject arguments) {
        co_return co_await postRequest(method, makeRequestData(method, std::move(arguments)));
    }

    Coroutine<RequestRouter::Response> RequestRouter::postRequest(QLatin1String method, QByteArray data) {
        if (!mConfiguration.has_value()) {
            throw std::runtime_error("Requests configuration is not set");
        }

        QNetworkRequest request(mConfiguration->serverUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"_L1);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        if (mConfiguration->authentication) {
            request.setRawHeader(authorizationHeader, mAuthorizationHeaderValue);
        }
        request.setSslConfiguration(mSslConfiguration);
        request.setTransferTimeout(
            static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(mConfiguration->timeout).count())
        );
        NetworkRequestMetadata metadata{};
        metadata.postData = data;
        metadata.rpcMetadata = {method};
        co_return co_await performRequest(request, metadata);
    }

    void RequestRouter::abortNetworkRequestsAndClearSessionId() {
        auto children = mNetwork->children();
        for (auto* child : children) {
            if (auto* reply = qobject_cast<QNetworkReply*>(child); reply && reply->isRunning()) {
                reply->abort();
            }
        }
        mNetwork->clearConnectionCache();
        mSessionId.clear();
    }

    QByteArray RequestRouter::makeRequestData(QLatin1String method, QJsonObject arguments) {
        return QJsonDocument(
                   QJsonObject{
                       {u"method"_s, method},
                       {u"arguments"_s, std::move(arguments)},
                   }
        )
            .toJson(QJsonDocument::Compact);
    }

    Coroutine<RequestRouter::Response>
    RequestRouter::performRequest(QNetworkRequest request, NetworkRequestMetadata metadata) {
        if (!mSessionId.isEmpty()) {
            request.setRawHeader(sessionIdHeader, mSessionId);
        }
        NetworkReplyUniquePtr reply(mNetwork->post(request, metadata.postData));

        auto expectedSslErrors = mExpectedSslErrors;
        reply->ignoreSslErrors(expectedSslErrors);
        auto sslErrors = std::make_shared<QList<QSslError>>();
        QObject::connect(reply.get(), &QNetworkReply::sslErrors, reply.get(), [=](const QList<QSslError>& errors) {
            for (const QSslError& error : errors) {
                if (!expectedSslErrors.contains(error)) {
                    sslErrors->push_back(error);
                }
            }
        });
        co_await *reply;
        if (reply->error() == QNetworkReply::NoError) {
            co_return co_await onRequestSuccess(std::move(reply), metadata.rpcMetadata);
        } else {
            co_return co_await onRequestError(std::move(reply), *sslErrors, metadata);
        }
    }

    Coroutine<RequestRouter::Response>
    RequestRouter::onRequestSuccess(NetworkReplyUniquePtr reply, RpcRequestMetadata metadata) {
        debug()
            .log("HTTP request for method '{}' succeeded, HTTP status: {}", metadata.method, httpStatus(reply.get()));
        const auto json = co_await runOnThreadPool(
            [](NetworkReplyUniquePtr reply) -> std::optional<QJsonObject> {
                const auto replyData = reply->readAll();
                QJsonParseError error{};
                QJsonObject json = QJsonDocument::fromJson(replyData, &error).object();
                if (error.error != QJsonParseError::NoError) {
                    warning().log(
                        "Failed to parse JSON reply from server:\n{}\nError '{}' at offset {}",
                        replyData,
                        error.errorString(),
                        error.offset
                    );
                    return {};
                }
                return json;
            },
            std::move(reply)
        );
        if (!json.has_value()) {
            emit requestFailed(RpcError::ParseError, {}, {});
            cancelCoroutine();
        }
        const bool success = isResultSuccessful(*json);
        if (!success) {
            warning().log("method '{}' failed, response: {}", metadata.method, *json);
        }
        co_return Response{.arguments = getReplyArguments(*json), .success = success};
    }

    Coroutine<RequestRouter::Response> RequestRouter::onRequestError(
        NetworkReplyUniquePtr reply, QList<QSslError> sslErrors, NetworkRequestMetadata metadata
    ) {
        if (reply->error() == QNetworkReply::ContentConflictError && reply->hasRawHeader(sessionIdHeader)) {
            QByteArray newSessionId = reply->rawHeader(sessionIdHeader);
            // Check against session id of request instead of current session id,
            // to handle case when current session id have already been overwritten by another failed request
            if (newSessionId != reply->request().rawHeader(sessionIdHeader)) {
                if (!mSessionId.isEmpty()) {
                    info().log("Session id changed");
                }
                debug().log("Session id is {}, retrying '{}' request", newSessionId, metadata.rpcMetadata.method);
                mSessionId = std::move(newSessionId);
                // Retry without incrementing retryAttempts
                co_return co_await performRequest(takeRequest(std::move(reply)), metadata);
            }
        }

        const QString detailedErrorMessage = makeDetailedErrorMessage(reply.get(), sslErrors);
        warning().log("HTTP request for method '{}' failed:\n{}", metadata.rpcMetadata.method, detailedErrorMessage);

        RpcError error{};
        bool shouldRetry{};
        switch (reply->error()) {
        case QNetworkReply::AuthenticationRequiredError:
            warning().log("Authentication error");
            error = RpcError::AuthenticationError;
            shouldRetry = false;
            break;
        case QNetworkReply::OperationCanceledError:
        case QNetworkReply::TimeoutError:
            warning().log("Timed out");
            error = RpcError::TimedOut;
            shouldRetry = true;
            break;
        default:
            error = RpcError::ConnectionError;
            shouldRetry = true;
            break;
        }

        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        if (shouldRetry && metadata.retryAttempts < mConfiguration.value().retryAttempts) {
            metadata.retryAttempts++;
            warning()
                .log("Retrying '{}' request, retry attempts = {}", metadata.rpcMetadata.method, metadata.retryAttempts);
            co_return co_await performRequest(takeRequest(std::move(reply)), metadata);
        }
        emit requestFailed(error, reply->errorString(), detailedErrorMessage);
        cancelCoroutine();
    }
}
