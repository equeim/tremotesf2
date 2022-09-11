/*
 * Tremotesf
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "signalhandler.h"

#include <QCoreApplication>

#ifdef Q_OS_UNIX
#include <array>
#include <csignal>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <QSocketNotifier>
#endif // Q_OS_UNIX

#ifdef Q_OS_WIN
#include <windows.h>
#include "tremotesf/windowshelpers.h"
#else
#include "tremotesf/unixhelpers.h"
#endif

#include "libtremotesf/log.h"

namespace tremotesf
{
    std::atomic_bool SignalHandler::exitRequested{false};

#ifdef Q_OS_UNIX
    namespace
    {
        std::array<int, 2> signalFds{};

        void signalHandler(int)
        {
            SignalHandler::exitRequested = true;
            std::array<char, 1> tmp{};
            [[maybe_unused]] const auto written = write(signalFds[0], tmp.data(), tmp.size());
        }
    }

    void SignalHandler::setupHandlers()
    {
        try {
            checkPosixError(socketpair(AF_UNIX, SOCK_STREAM, 0, signalFds.data()), "socketpair");

            struct sigaction action{};
            action.sa_handler = signalHandler;
            action.sa_flags |= SA_RESTART;

            checkPosixError(sigaction(SIGINT, &action, nullptr), "sigaction");
            checkPosixError(sigaction(SIGTERM, &action, nullptr), "sigaction");
            checkPosixError(sigaction(SIGHUP, &action, nullptr), "sigaction");
            checkPosixError(sigaction(SIGQUIT, &action, nullptr), "sigaction");
        } catch (const std::system_error& e) {
            logWarningWithException(e, "Failed to setup signal handlers");
        }
    }

    void SignalHandler::setupNotifier()
    {
        auto notifier = new QSocketNotifier(signalFds[1], QSocketNotifier::Read, qApp);
        QObject::connect(notifier, &QSocketNotifier::activated, qApp, [notifier](int socket) {
            // This lambda will be executed only after calling QCoreApplication::exec()
            notifier->setEnabled(false);
            std::array<char, 1> tmp{};
            [[maybe_unused]] const auto r = read(socket, &tmp, sizeof(tmp));
            QCoreApplication::quit();
            notifier->setEnabled(true);
        });
    }
#endif // Q_OS_UNIX

#ifdef Q_OS_WIN
    namespace
    {
        BOOL WINAPI consoleHandler(DWORD dwCtrlType)
        {
            logInfo("Received signal with type = {}", dwCtrlType);
            SignalHandler::exitRequested = true;
            QCoreApplication::quit();
            return TRUE;
        }
    }

    void SignalHandler::setupHandlers()
    {
        try {
            checkWin32Bool(SetConsoleCtrlHandler(consoleHandler, TRUE), "SetConsoleCtrlHandler");
        } catch (const std::system_error& e) {
            logWarningWithException(e, "Failed to setup signal handler");
        }
    }

    void SignalHandler::setupNotifier()
    {
    }
#endif // Q_OS_WIN
}
