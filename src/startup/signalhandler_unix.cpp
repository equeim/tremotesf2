// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "signalhandler.h"

#include <array>
#include <atomic>
#include <csignal>
#include <limits>
#include <optional>
#include <string_view>
#include <thread>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QScopeGuard>
#include <QFileInfo>

#include "log/log.h"
#include "unixhelpers.h"

using namespace std::string_view_literals;

namespace tremotesf {
    namespace {
        constexpr std::array expectedSignals{
            std::pair{SIGINT, "SIGINT"sv},
            std::pair{SIGTERM, "SIGTERM"sv},
            std::pair{SIGHUP, "SIGHUP"sv},
            std::pair{SIGQUIT, "SIGQUIT"sv}
        };

        std::optional<std::string_view> signalName(int signal) {
            for (auto [expectedSignal, name] : expectedSignals) {
                if (signal == expectedSignal) {
                    return name;
                }
            }
            return std::nullopt;
        }

        int writeSocket{};

        // Not using std::atomic<std::optional<int>> because Clang might require linking to libatomic
        constexpr int notReceivedSignal = std::numeric_limits<int>::min();
        std::atomic_int receivedSignal{notReceivedSignal};
        static_assert(std::atomic_int::is_always_lock_free, "std::atomic_int must be lock-free");

        void signalHandler(int signal) {
            int expected = notReceivedSignal;
            if (!receivedSignal.compare_exchange_strong(expected, signal)) {
                // Already requested exit
                return;
            }
            while (true) {
                const char byte{};
                const auto bytes = write(writeSocket, &byte, 1);
                if (bytes == -1 && errno == EINTR) {
                    continue;
                }
                break;
            }
        }
    }

    class SignalHandler::Impl {
    public:
        Impl() {
            try {
                int sockets[2]{};
                checkPosixError(socketpair(AF_UNIX, SOCK_STREAM, 0, static_cast<int*>(sockets)), "socketpair");
                writeSocket = sockets[0];
                const int readSocket = sockets[1];

                struct sigaction action {};
                action.sa_handler = signalHandler;
                action.sa_flags |= SA_RESTART;
                for (auto [signal, _] : expectedSignals) {
                    checkPosixError(sigaction(signal, &action, nullptr), "sigaction");
                }

                logDebug("signalhandler: created socket pair and set up signal handlers");
                try {
                    logDebug("signalhandler: starting read socket thread");
                    mThread = std::thread(&Impl::readFromSocket, this, readSocket);
                } catch (const std::system_error& e) {
                    logWarningWithException(e, "signalhandler: failed to start thread");
                }
            } catch (const std::system_error& e) {
                logWarningWithException(e, "Failed to setup signal handlers");
                return;
            }
        }

        ~Impl() {
            logDebug("signalhandler: closing write socket");
            try {
                checkPosixError(close(writeSocket), "close");
            } catch (const std::system_error& e) {
                logWarningWithException(e, "signalhandler: failed to close write socket");
            }
            logDebug("signalhandler: joining read socket thread");
            mThread.join();
            logDebug("signalhandler: joined read socket thread");
        }

        Q_DISABLE_COPY_MOVE(Impl)

    private:
        void readFromSocket(int readSocket) const {
            logDebug("signalhandler: started read socket thread");
            auto finishGuard = QScopeGuard([readSocket] {
                logDebug("signalhandler: closing read socket");
                try {
                    checkPosixError(close(readSocket), "close");
                } catch (const std::system_error& e) {
                    logWarningWithException(e, "signalhandler: failed to close read socket");
                }
                logDebug("signalhandler: finished read socket thread");
            });

            while (true) {
                char byte{};
                try {
                    const ssize_t bytes = checkPosixError(read(readSocket, &byte, 1), "read");
                    if (bytes == 0) {
                        logDebug("signalhandler: write socket was closed, end thread");
                        return;
                    }
                } catch (const std::system_error& e) {
                    if (e.code() == std::errc::interrupted) {
                        logWarning("signalhandler: read interrupted, continue");
                        continue;
                    }
                    logWarningWithException(e, "signalhandler: failed to read from socket, end thread");
                    return;
                }
                break;
            }
            if (int signal = receivedSignal; signal != notReceivedSignal) {
                if (const auto name = signalName(signal); name.has_value()) {
                    logInfo("signalhandler: received signal {}", *name);
                } else {
                    logInfo("signalhandler: received signal {}", signal);
                }
            } else {
                logWarning("signalhandler: read from socket but signal was not received");
                return;
            }
            const auto app = QCoreApplication::instance();
            if (app) {
                logInfo("signalhandler: post QCoreApplication::quit() to event loop");
                QMetaObject::invokeMethod(app, &QCoreApplication::quit, Qt::QueuedConnection);
            } else {
                logWarning("signalhandler: QApplication is not created yet");
            }
        }

        std::thread mThread{};
    };

    SignalHandler::SignalHandler() : mImpl(std::make_unique<Impl>()) {}

    SignalHandler::~SignalHandler() = default;

    bool SignalHandler::isExitRequested() const { return receivedSignal.load() != notReceivedSignal; }
}
