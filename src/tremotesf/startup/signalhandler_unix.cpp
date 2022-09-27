#include "signalhandler.h"

#include <atomic>
#include <csignal>
#include <limits>
#include <string_view>
#include <thread>
#include <unordered_map>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <QCoreApplication>

#include "libtremotesf/log.h"
#include "tremotesf/unixhelpers.h"

namespace tremotesf::signalhandler
{
    namespace
    {
        int writeSocket{};
        int readSocket{};

        // Not using std::atomic<std::optional<int>> because Clang might require linking to libatomic
        constexpr int notReceivedSignal = std::numeric_limits<int>::min();
        std::atomic_int receivedSignal{notReceivedSignal};
        static_assert(std::atomic_int::is_always_lock_free, "std::atomic_int must be lock-free");

        void signalHandler(int signal)
        {
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
                } else {
                    break;
                }
            }
        }

        class SignalSocketReader {
        public:
            explicit SignalSocketReader(std::unordered_map<int, std::string_view>&& signalNames)
                : mSignalNames(std::move(signalNames)) {}

            ~SignalSocketReader() {
                close(writeSocket);
                mThread.join();
            }
        private:
            void readFromSocket() {
                while (true) {
                    char byte{};
                    try {
                        const auto bytes = checkPosixError(read(readSocket, &byte, 1), "read");
                        if (bytes == 0) {
                            // Write socket was closed
                            return;
                        }
                    } catch (const std::system_error& e) {
                        if (e.code() == std::errc::interrupted) {
                            logWarning("signalhandler: read interrupted, continue");
                            continue;
                        } else {
                            logWarningWithException(e, "signalhandler: failed to read from socket, end thread");
                            return;
                        }
                    }
                    break;
                }
                if (int signal = receivedSignal; signal != notReceivedSignal) {
                    const auto found = mSignalNames.find(signal);
                    if (found != mSignalNames.end()) {
                        logInfo("signalhandler: received signal {}", found->second);
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

            std::unordered_map<int, std::string_view> mSignalNames{};
            std::thread mThread{&SignalSocketReader::readFromSocket, this};
        };
    }

    void setupSignalHandlers() {
        std::unordered_map<int, std::string_view> signalNames{
            {SIGINT, "SIGINT"},
            {SIGTERM, "SIGTERM"},
            {SIGHUP, "SIGHUP"},
            {SIGQUIT, "SIGQUIT"}
        };

        try {
            int sockets[2]{};
            checkPosixError(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), "socketpair");
            writeSocket = sockets[0];
            readSocket = sockets[1];

            struct sigaction action{};
            action.sa_handler = signalHandler;
            action.sa_flags |= SA_RESTART;
            for (const auto& [signal, _] : signalNames) {
                checkPosixError(sigaction(signal, &action, nullptr), "sigaction");
            }
        } catch (const std::system_error& e) {
            logWarningWithException(e, "Failed to setup signal handlers");
            return;
        }

        static SignalSocketReader reader(std::move(signalNames));
    }

    bool isExitRequested() {
        return receivedSignal.load() != notReceivedSignal;
    }
}
