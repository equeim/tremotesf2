// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <future>
#include <optional>
#include <variant>

#include <QHostAddress>
#include <QJsonDocument>
#include <QScopeGuard>
#include <QTest>
#include <QThreadPool>
#include <QTcpServer>
#include <QTcpSocket>

#include <fmt/chrono.h>
#include <httplib.h>

#include "coroutines/coroutines.h"
#include "fileutils.h"
#include "literals.h"
#include "log/log.h"
#include "requestrouter.h"

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace tremotesf;
using namespace tremotesf::impl;

#define QFAIL_THROW(message)                                                 \
    do {                                                                     \
        QTest::qFail(static_cast<const char*>(message), __FILE__, __LINE__); \
        throw std::exception();                                              \
    } while (false)

// NOLINTBEGIN(bugprone-unchecked-optional-access)

namespace {
    constexpr auto testApiPath = "/"_l1;
    constexpr auto testTimeout = 5s;
    constexpr auto testRetryAttempts = 0;

    const auto contentType = "application/json"s;
    const auto successResponse = "{\"result\":\"success\"}"s;
    const auto invalidJsonResponse = "{\"result\":\"success}"s;
    const auto sessionIdHeader = "X-Transmission-Session-Id"s;

    template<std::derived_from<httplib::Server> Server = httplib::Server>
    class TestHttpServer {
    public:
        template<typename... Args>
        explicit TestHttpServer(Args&&... args) : mServer(std::forward<Args>(args)...) {
            info().log("Server is valid = {}", mServer.is_valid());
            mServer.set_keep_alive_max_count(1);
            mServer.set_keep_alive_timeout(1);
            port = mServer.bind_to_any_port(host.toStdString());
            info().log("Bound to port {}", port);
            mServer.Post(testApiPath.data(), [=, this](const httplib::Request& req, httplib::Response& res) {
                httplib::Server::Handler handler{};
                {
                    const std::unique_lock lock(mHandlerMutex);
                    handler = mHandler;
                }
                if (handler) {
                    handler(req, res);
                } else {
                    res.status = 500;
                }
            });

            mListenFuture = std::async([=, this] {
                info().log("Starting listening on address {} and port {}", host, port);
                const bool ok = mServer.listen_after_bind();
                info().log("Stopped listening, ok = {}", ok);
            });
        }

        ~TestHttpServer() {
            // Wait until we've started listing or already finished because of error
            while (true) {
                if (mServer.is_running()) break;
                if (mListenFuture.wait_for(10ms) == std::future_status::ready) break;
            }
            if (mServer.is_running()) {
                info().log("Stopping server");
                mServer.stop();
            } else {
                warning().log("Server has already stopped");
            }
            mListenFuture.wait();
        }

        Q_DISABLE_COPY_MOVE(TestHttpServer)

        const QString host{QHostAddress(QHostAddress::LocalHost).toString()};
        int port{};

        void handle(httplib::Server::Handler&& handler) {
            const std::unique_lock lock(mHandlerMutex);
            mHandler = std::move(handler);
        }

        void clearHandler() {
            const std::unique_lock lock(mHandlerMutex);
            mHandler = {};
        }

    private:
        Server mServer{};
        std::future<void> mListenFuture{};
        httplib::Server::Handler mHandler{};
        std::mutex mHandlerMutex{};
    };

    void success(httplib::Response& res) { res.set_content(successResponse, contentType); }

    void checkAuthentication(
        const httplib::Request& req, httplib::Response& res, const std::string& username, const std::string& password
    ) {
        const auto header = httplib::make_basic_authentication_header(username, password);
        if (req.get_header_value(header.first) == header.second) {
            success(res);
        } else {
            res.status = 401;
            res.set_header("WWW-Authenticate", R"(Basic realm="Do it")");
        }
    }

    class RequestRouterTest final : public QObject {
        Q_OBJECT

    public:
        RequestRouterTest() { mThreadPool.setMaxThreadCount(1); }

    private slots:
        void init() {
            const int port = mServer.port;
            RequestRouter::RequestsConfiguration config{};
            config.serverUrl.setScheme("http"_l1);
            config.serverUrl.setHost(mServer.host);
            config.serverUrl.setPort(port);
            config.serverUrl.setPath(testApiPath);
            config.timeout = testTimeout;
            config.retryAttempts = testRetryAttempts;
            mRouter.setConfiguration(std::move(config));
        }

        void cleanup() {
            mServer.clearHandler();
            mRouter.abortNetworkRequestsAndClearSessionId();
        }

        void checkUrlIsCorrect() {
            mServer.handle([&](const httplib::Request&, httplib::Response& res) { success(res); });
            const auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->arguments, QJsonObject{});
            QCOMPARE(response->success, true);
        }

        void checkThatJsonIsFormedCorrectly() {
            const auto method = "foo"_l1;
            const QJsonObject arguments{{"bar", "foobar"}};

            mServer.handle([&](const httplib::Request& req, httplib::Response& res) {
                const auto json = QJsonDocument::fromJson(req.body.c_str());
                const auto expectedJson =
                    QJsonDocument(QJsonObject{{"method"_l1, method}, {"arguments"_l1, arguments}});
                if (json == expectedJson) {
                    success(res);
                } else {
                    res.status = 400;
                }
            });

            const auto response = waitForResponse(method, arguments);
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->success, true);
        }

        void checkTimeoutIsHandled() {
            QTcpServer tcpServer{};
            tcpServer.listen(QHostAddress::LocalHost);
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setPort(tcpServer.serverPort());
                config.timeout = 100ms;
                mRouter.setConfiguration(std::move(config));
            }

            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::TimedOut);
        }

        void checkTcpConnectionRefusedIsHandled() {
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                // It is likely that there is nothing listening on this port, so we will get ConnectionRefusedError
                config.serverUrl.setPort(9);
                mRouter.setConfiguration(std::move(config));
            }
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            if (error.value() == RpcError::TimedOut) {
                // This is not what we test here but it can happen on some systems
                QWARN("Connection to port 9 timed out instead of being refused");
            } else {
                QCOMPARE(error.value(), RpcError::ConnectionError);
            }
        }

        void checkTcpConnectionClosedErrorIsHandled() {
            QTcpServer tcpServer{};
            QObject::connect(&tcpServer, &QTcpServer::newConnection, this, [&tcpServer] {
                tcpServer.nextPendingConnection()->close();
            });
            tcpServer.listen(QHostAddress::LocalHost);
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setPort(tcpServer.serverPort());
                mRouter.setConfiguration(std::move(config));
            }
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ConnectionError);
        }

        void checkThatRequestsAreRetried() {
            const int retryAttempts = 2;
            std::atomic_int requestsCount{};
            mServer.handle([&](const httplib::Request&, httplib::Response& res) {
                ++requestsCount;
                res.status = 500;
            });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.retryAttempts = retryAttempts;
                mRouter.setConfiguration(std::move(config));
            }
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ConnectionError);
            QCOMPARE(requestsCount.load(), retryAttempts + 1);
        }

        void checkSelfSignedCertificateError() {
            TestHttpServer<httplib::SSLServer> server(
                TEST_DATA_PATH "/root-certificate.pem",
                TEST_DATA_PATH "/root-certificate-key.pem"
            );
            server.handle([&](const httplib::Request&, httplib::Response& res) { success(res); });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setScheme("https"_l1);
                config.serverUrl.setPort(server.port);
                mRouter.setConfiguration(std::move(config));
            }
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ConnectionError);
        }

        void checkSelfSignedCertificateSuccess() {
            TestHttpServer<httplib::SSLServer> server(
                TEST_DATA_PATH "/root-certificate.pem",
                TEST_DATA_PATH "/root-certificate-key.pem"
            );
            server.handle([&](const httplib::Request&, httplib::Response& res) { success(res); });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setScheme("https"_l1);
                config.serverUrl.setPort(server.port);
                config.serverCertificateChain =
                    QSslCertificate::fromPath(TEST_DATA_PATH "/root-certificate.pem", QSsl::Pem);
                mRouter.setConfiguration(std::move(config));
            }
            const auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->success, true);
        }

        void checkSelfSignedCertificateChainSuccess() {
            TestHttpServer<httplib::SSLServer> server(
                TEST_DATA_PATH "/chain.pem",
                TEST_DATA_PATH "/signed-certificate-key.pem"
            );
            server.handle([&](const httplib::Request&, httplib::Response& res) { success(res); });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setScheme("https"_l1);
                config.serverUrl.setPort(server.port);
                config.serverCertificateChain = QSslCertificate::fromPath(TEST_DATA_PATH "/chain.pem", QSsl::Pem);
                mRouter.setConfiguration(std::move(config));
            }
            const auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->success, true);
        }

        void checkClientCertificateError() {
            TestHttpServer<httplib::SSLServer> server(
                TEST_DATA_PATH "/root-certificate.pem",
                TEST_DATA_PATH "/root-certificate-key.pem",
                TEST_DATA_PATH "/client-certificate-and-key.pem"
            );
            server.handle([&](const httplib::Request&, httplib::Response& res) { success(res); });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setScheme("https"_l1);
                config.serverUrl.setPort(server.port);
                config.serverCertificateChain =
                    QSslCertificate::fromPath(TEST_DATA_PATH "/root-certificate.pem", QSsl::Pem);
                mRouter.setConfiguration(std::move(config));
            }
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ConnectionError);
        }

        void checkClientCertificateSuccess() {
            TestHttpServer<httplib::SSLServer> server(
                TEST_DATA_PATH "/root-certificate.pem",
                TEST_DATA_PATH "/root-certificate-key.pem",
                TEST_DATA_PATH "/client-certificate-and-key.pem"
            );
            server.handle([&](const httplib::Request&, httplib::Response& res) { success(res); });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setScheme("https"_l1);
                config.serverUrl.setPort(server.port);
                config.serverCertificateChain =
                    QSslCertificate::fromPath(TEST_DATA_PATH "/root-certificate.pem", QSsl::Pem);
                config.clientCertificate =
                    QSslCertificate::fromPath(TEST_DATA_PATH "/client-certificate-and-key.pem", QSsl::Pem).first();
                {
                    QFile file(TEST_DATA_PATH "/client-certificate-and-key.pem");
                    openFile(file, QIODevice::ReadOnly);
                    config.clientPrivateKey = QSslKey(&file, QSsl::Rsa);
                }
                mRouter.setConfiguration(std::move(config));
            }
            const auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->success, true);
        }

        void checkInvalidJsonIsHandled() {
            mServer.handle([&](const httplib::Request&, httplib::Response& res) {
                res.set_content(invalidJsonResponse, contentType);
            });
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ParseError);
        }

        void checkConflictErrorWithoutSessionIdIsHandled() {
            mServer.handle([&](const httplib::Request&, httplib::Response& res) { res.status = 409; });
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ConnectionError);
        }

        void checkPersistentConflictErrorWithSessionIdIsHandled() {
            const std::string sessionIdValue = "id";
            mServer.handle([&](const httplib::Request&, httplib::Response& res) {
                res.status = 409;
                res.set_header(sessionIdHeader, sessionIdValue);
            });
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::ConnectionError);
        }

        void checkConflictErrorWithSessionIdIsHandled() {
            const std::string sessionIdValue = "id";
            mServer.handle([&](const httplib::Request& req, httplib::Response& res) {
                if (req.get_header_value(sessionIdHeader) == sessionIdValue) {
                    success(res);
                } else {
                    res.status = 409;
                }
                res.set_header(sessionIdHeader, sessionIdValue);
            });
            const auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->arguments, QJsonObject{});
            QCOMPARE(response->success, true);
        }

        void checkConflictErrorWithChangingSessionIdIsHandled() {
            std::string sessionIdValue = "session id";
            mServer.handle([&](const httplib::Request& req, httplib::Response& res) {
                if (req.get_header_value(sessionIdHeader) == sessionIdValue) {
                    success(res);
                } else {
                    res.status = 409;
                }
                res.set_header(sessionIdHeader, sessionIdValue);
            });
            auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->arguments, QJsonObject{});
            QCOMPARE(response->success, true);

            sessionIdValue = "session id 2";
            response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->arguments, QJsonObject{});
            QCOMPARE(response->success, true);
        }

        void checkThatAuthenticationWorks() {
            const QString user = "foo"_l1;
            const QString password = "bar"_l1;
            const auto header = httplib::make_basic_authentication_header(user.toStdString(), password.toStdString());
            mServer.handle([&](const httplib::Request& req, httplib::Response& res) {
                checkAuthentication(req, res, user.toStdString(), password.toStdString());
            });
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.authentication = true;
                config.username = user;
                config.password = password;
                mRouter.setConfiguration(std::move(config));
            }
            const auto response = waitForResponse("foo"_l1, QByteArray{});
            QCOMPARE(response.has_value(), true);
            QCOMPARE(response->arguments, QJsonObject{});
            QCOMPARE(response->success, true);
        }

        void checkAuthenticationErrorIsHandled() {
            mServer.handle([&](const httplib::Request& req, httplib::Response& res) {
                checkAuthentication(req, res, "foo", "bar");
            });
            const auto error = waitForError("foo"_l1, QByteArray{});
            QCOMPARE(error.has_value(), true);
            QCOMPARE(error.value(), RpcError::AuthenticationError);
        }

        void checkRequestAbort() {
            QTcpServer tcpServer{};
            tcpServer.listen(QHostAddress::LocalHost);
            {
                RequestRouter::RequestsConfiguration config = mRouter.configuration().value();
                config.serverUrl.setPort(tcpServer.serverPort());
                mRouter.setConfiguration(std::move(config));
            }

            std::variant<RequestRouter::Response, RpcError, std::monostate> responseOrError = std::monostate{};
            CoroutineScope scope{};
            scope.launch(waitForResponseOrErrorCoroutine(mRouter.postRequest("foo"_l1, QByteArray{}), responseOrError));

            QTest::qWait(100);
            mRouter.abortNetworkRequestsAndClearSessionId();

            const bool ok = QTest::qWaitFor(
                [&] { return !std::holds_alternative<std::monostate>(responseOrError); },
                static_cast<int>(
                    duration_cast<milliseconds>(testTimeout * (mRouter.configuration().value().retryAttempts + 1) + 1s)
                        .count()
                )
            );
            if (!ok) {
                QWARN("Timed out when waiting for response");
            }

            if (std::holds_alternative<RpcError>(responseOrError)) {
                QCOMPARE(std::get<RpcError>(responseOrError), RpcError::TimedOut);
            } else {
                QFAIL("Request must return error");
            }
        }

    private:
        Coroutine<> waitForResponseOrErrorCoroutine(
            Coroutine<RequestRouter::Response> requestCoroutine,
            std::variant<RequestRouter::Response, RpcError, std::monostate>& destination
        ) {
            const auto connection = QObject::connect(
                &mRouter,
                &RequestRouter::requestFailed,
                this,
                [&](RpcError error,
                    [[maybe_unused]] const QString& errorMessage,
                    [[maybe_unused]] const QString& detailedErrorMessage) { destination = error; },
                Qt::DirectConnection
            );
            const auto connectionGuard = QScopeGuard([connection] { QObject::disconnect(connection); });
            destination = co_await requestCoroutine;
        }

        template<typename... Args>
        std::variant<RequestRouter::Response, RpcError, std::monostate> waitForResponseOrError(const Args&... args) {
            std::variant<RequestRouter::Response, RpcError, std::monostate> responseOrError = std::monostate{};
            CoroutineScope scope{};
            scope.launch(waitForResponseOrErrorCoroutine(mRouter.postRequest(args...), responseOrError));
            const bool ok = QTest::qWaitFor(
                [&] { return !std::holds_alternative<std::monostate>(responseOrError); },
                static_cast<int>(
                    duration_cast<milliseconds>(testTimeout * (mRouter.configuration().value().retryAttempts + 1) + 1s)
                        .count()
                )
            );
            if (!ok) {
                QWARN("Timed out when waiting for response");
            }
            return responseOrError;
        }

        template<typename... Args>
        std::optional<RequestRouter::Response> waitForResponse(const Args&... args) {
            const auto responseOrError = waitForResponseOrError(args...);
            if (std::holds_alternative<RequestRouter::Response>(responseOrError)) {
                return std::get<RequestRouter::Response>(responseOrError);
            }
            return {};
        }

        template<typename... Args>
        std::optional<RpcError> waitForError(const Args&... args) {
            const auto responseOrError = waitForResponseOrError(args...);
            if (std::holds_alternative<RpcError>(responseOrError)) {
                return std::get<RpcError>(responseOrError);
            }
            return {};
        }

        template<typename... Args>
        Coroutine<> detachRequest(const Args&... args) {
            co_await mRouter.postRequest(args...);
        }

        TestHttpServer<httplib::Server> mServer{};
        QThreadPool mThreadPool{};
        RequestRouter mRouter{nullptr, &mThreadPool};
    };
}

// NOLINTEND(bugprone-unchecked-optional-access)

QTEST_GUILESS_MAIN(RequestRouterTest)

#include "requestrouter_test.moc"
