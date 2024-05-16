// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_REQUESTROUTER_H
#define TREMOTESF_RPC_REQUESTROUTER_H

#include <chrono>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QtContainerFwd>

#include "rpc.h"

class QNetworkReply;
class QSslError;
class QThreadPool;

namespace tremotesf {
    namespace impl {
        struct RpcRequestMetadata;
        struct NetworkRequestMetadata;
    }

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

        enum class RequestType { DataUpdate, Independent };

        const std::optional<RequestsConfiguration>& configuration() const { return mConfiguration; }
        void setConfiguration(RequestsConfiguration configuration);
        void resetConfiguration();

        struct Response {
            QJsonObject arguments{};
            bool success{};
        };

        void postRequest(
            QLatin1String method,
            const QJsonObject& arguments,
            RequestType type,
            std::function<void(Response)>&& onResponse = {}
        );

        void postRequest(
            QLatin1String method,
            const QByteArray& data,
            RequestType type,
            std::function<void(Response)>&& onResponse = {}
        );

        const QByteArray& sessionId() const { return mSessionId; };

        bool hasPendingDataUpdateRequests() const;
        void cancelPendingRequestsAndClearSessionId();

        static QByteArray makeRequestData(const QString& method, const QJsonObject& arguments);

    private:
        void postRequest(QNetworkRequest request, const impl::NetworkRequestMetadata& metadata);

        bool retryRequest(const QNetworkRequest& request, impl::NetworkRequestMetadata metadata);

        void onRequestFinished(QNetworkReply* reply, const QList<QSslError>& sslErrors);
        void onRequestSuccess(QNetworkReply* reply, const impl::RpcRequestMetadata& metadata);
        void onRequestError(
            QNetworkReply* reply, const QList<QSslError>& sslErrors, const impl::NetworkRequestMetadata& metadata
        );
        static QString makeDetailedErrorMessage(QNetworkReply* reply, const QList<QSslError>& sslErrors);

        QNetworkAccessManager* mNetwork{};
        QThreadPool* mThreadPool{};
        std::unordered_set<QNetworkReply*> mPendingNetworkRequests{};
        std::unordered_set<QObject*> mPendingParseFutures{};
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
