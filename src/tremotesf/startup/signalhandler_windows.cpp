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

#include <atomic>
#include <QCoreApplication>

#include <windows.h>

#include "libtremotesf/log.h"
#include "tremotesf/windowshelpers.h"

namespace tremotesf::signalhandler
{
    namespace
    {
        std::atomic_bool exitRequested{};

        BOOL WINAPI consoleHandler(DWORD dwCtrlType)
        {
            exitRequested = true;
            std::string_view type{};
            switch (dwCtrlType) {
            case CTRL_C_EVENT:
                type = "CTRL_C_EVENT";
                break;
            case CTRL_BREAK_EVENT:
                type = "CTRL_BREAK_EVENT";
                break;
            case CTRL_CLOSE_EVENT:
                type = "CTRL_CLOSE_EVENT";
                break;
            case CTRL_LOGOFF_EVENT:
                type = "CTRL_LOGOFF_EVENT";
                break;
            case CTRL_SHUTDOWN_EVENT:
                type = "CTRL_SHUTDOWN_EVENT";
                break;
            default:
                break;
            }
            if (!type.empty()) {
                logInfo("Received signal with type = {}", type);
            } else {
                logInfo("Received signal with type = {}", dwCtrlType);
            }
            const auto app = QCoreApplication::instance();
            if (app) {
                logInfo("signalhandler: post QCoreApplication::quit() to event loop");
                QMetaObject::invokeMethod(app, &QCoreApplication::quit, Qt::QueuedConnection);
            } else {
                logWarning("signalhandler: QApplication is not created yet");
            }
            return TRUE;
        }
    }

    void setupSignalHandlers()
    {
        try {
            checkWin32Bool(SetConsoleCtrlHandler(&consoleHandler, TRUE), "SetConsoleCtrlHandler");
        } catch (const std::system_error& e) {
            logWarningWithException(e, "Failed to setup signal handler");
        }
    }

    bool isExitRequested() {
        return exitRequested;
    }
}
