// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    void initSignalHandler()
    {
        try {
            checkWin32Bool(SetConsoleCtrlHandler(&consoleHandler, TRUE), "SetConsoleCtrlHandler");
            logDebug("signalhandler: added console signal handler");
        } catch (const std::system_error& e) {
            logWarningWithException(e, "signalhandler: failed to add console signal handler");
        }
    }

    void deinitSignalHandler() {
        try {
            checkWin32Bool(SetConsoleCtrlHandler(&consoleHandler, FALSE), "SetConsoleCtrlHandler");
            logDebug("signalhandler: removed console signal handler");
        } catch (const std::system_error& e) {
            logWarningWithException(e, "signalhandler: failed to remove console signal handler");
        }
    }

    bool isExitRequested() {
        return exitRequested;
    }
}
