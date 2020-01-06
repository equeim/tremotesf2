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

#include <QtGlobal>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <QSocketNotifier>
#endif // Q_OS_UNIX

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // Q_OS_WIN

#include <QCoreApplication>

namespace tremotesf
{
    std::atomic_bool SignalHandler::exitRequested{false};

#ifdef Q_OS_UNIX
    namespace
    {
        int signalsFd[2]{};

        void signalHandler(int)
        {
            SignalHandler::exitRequested = true;
            char tmp = 0;
            write(signalsFd[0], &tmp, sizeof(tmp));
        }
    }

    void SignalHandler::setupHandlers()
    {
        int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, signalsFd);
        if (ret != 0) {
            qFatal("Failed to create socketpair, errno=%d", errno);
        }

        struct sigaction action{};
        action.sa_handler = signalHandler;
        action.sa_flags |= SA_RESTART;

        ret = sigaction(SIGINT, &action, nullptr);
        if (ret != 0) {
            qFatal("Failed to set signal handler on SIGINT, errno=%d", errno);
        }
        ret = sigaction(SIGTERM, &action, nullptr);
        if (ret != 0) {
            qFatal("Failed to set signal handler on SIGTERM, errno=%d", errno);
        }
        ret = sigaction(SIGHUP, &action, nullptr);
        if (ret != 0) {
            qFatal("Failed to set signal handler on SIGHUP, errno=%d", errno);
        }
        ret = sigaction(SIGQUIT, &action, nullptr);
        if (ret != 0) {
            qFatal("Failed to set signal handler on SIGQUIT, errno=%d", errno);
        }
    }

    void SignalHandler::setupNotifier()
    {
        auto notifier = new QSocketNotifier(signalsFd[1], QSocketNotifier::Read, qApp);
        QObject::connect(notifier, &QSocketNotifier::activated, qApp, [notifier](int socket) {
            // This lambda will be executed only after calling QCoreApplication::exec()
            notifier->setEnabled(false);
            char tmp;
            read(socket, &tmp, sizeof(tmp));
            QCoreApplication::quit();
            notifier->setEnabled(true);
        });
    }
#endif // Q_OS_UNIX

#ifdef Q_OS_WIN
    namespace
    {
        WINBOOL WINAPI consoleHandler(DWORD)
        {
            SignalHandler::exitRequested = true;
            QCoreApplication::quit();
            return TRUE;
        }
    }

    void SignalHandler::setupHandlers()
    {
        if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
            qFatal("SetConsoleCtrlHandler failed");
        }
    }

    void SignalHandler::setupNotifier()
    {
    }
#endif // Q_OS_WIN
}
