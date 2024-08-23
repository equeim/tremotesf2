// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_REQUESTROUTER_H
#define TREMOTESF_RPC_REQUESTROUTER_H

#include <chrono>
#include <memory>
#include <optional>

#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QPointer>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QtContainerFwd>

#include "coroutines/coroutinefwd.h"
#include "rpc.h"

class QNetworkReply;
class QSslError;
class QThreadPool;

namespace tremotesf::impl {
    struct RpcRequestMetadata;
    struct NetworkRequestMetadata;

    struct NetworkReplyDeleter;
    using NetworkReplyUniquePtr = std::unique_ptr<QNetworkReply, NetworkReplyDeleter>;

    class RequestRouter final : public QObject {
        Q_OBJECT

    public:
        explicit RequestRouter(QThreadPool* threadPool, QObject* parent = nullptr);
        explicit RequestRouter(QObject* parent = nullptr);

        struct RequestsConfiguration {
            QUrl serverUrl{};
            QNetworkProxy proxy{QNetworkProxy::applicationProxy()};
            QList<QSslCertificate> serverCertificateChain{};
            QSslCertificate clientCertificate{};
            QSslKey clientPrivateKey{};
            std::chrono::milliseconds timeout{};
            int retryAttempts{2};
            bool authentication{};
            QString username{};
            QString password{};
        };

        const std::optional<RequestsConfiguration>& configuration() const { return mConfiguration; }
        void setConfiguration(RequestsConfiguration configuration);
        void resetConfiguration();

        struct Response {
            QJsonObject arguments{};
            bool success{};
        };

        Coroutine<Response> postRequest(QLatin1String method, QJsonObject arguments);

        Coroutine<Response> postRequest(QLatin1String method, QByteArray data);

        const QByteArray& sessionId() const { return mSessionId; };

        void abortNetworkRequestsAndClearSessionId();

        static QByteArray makeRequestData(QLatin1String method, QJsonObject arguments);

    private:
        Coroutine<Response> performRequest(QNetworkRequest request, NetworkRequestMetadata metadata);
        Coroutine<Response> onRequestSuccess(NetworkReplyUniquePtr reply, RpcRequestMetadata metadata);
        Coroutine<Response>
        onRequestError(NetworkReplyUniquePtr reply, QList<QSslError> sslErrors, NetworkRequestMetadata metadata);

        QNetworkAccessManager* mNetwork{};
        QThreadPool* mThreadPool{};

        QByteArray mSessionId{};
        QByteArray mAuthorizationHeaderValue{};

        std::optional<RequestsConfiguration> mConfiguration{};
        QSslConfiguration mSslConfiguration{};
        QList<QSslError> mExpectedSslErrors{};

    signals:
        /**
         * Emitted if request has failed with network or HTTP error
         * @brief requestFailed
         * @param error Error type
         * @param errorMessage Short error message
         * @param detailedErrorMessage Detailed error message
         */
        void requestFailed(RpcError error, const QString& errorMessage, const QString& detailedErrorMessage);
    };
}

#endif // TREMOTESF_RPC_REQUESTROUTER_H
