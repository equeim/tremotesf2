// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "signalhandler.h"

#include <atomic>
#include <QCoreApplication>

#include <windows.h>

#include "log/log.h"
#include "windowshelpers.h"

namespace tremotesf {
    namespace {
        std::atomic_bool exitRequested{};

        BOOL WINAPI consoleHandler(DWORD dwCtrlType) {
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
                info().log("Received signal with type = {}", type);
            } else {
                info().log("Received signal with type = {}", dwCtrlType);
            }
            const auto app = QCoreApplication::instance();
            if (app) {
                info().log("signalhandler: post QCoreApplication::quit() to event loop");
                QMetaObject::invokeMethod(app, &QCoreApplication::quit, Qt::QueuedConnection);
            } else {
                warning().log("signalhandler: QApplication is not created yet");
            }
            return TRUE;
        }
    }

    class SignalHandler::Impl {};

    SignalHandler::SignalHandler() : mImpl{} {
        try {
            checkWin32Bool(SetConsoleCtrlHandler(&consoleHandler, TRUE), "SetConsoleCtrlHandler");
            debug().log("signalhandler: added console signal handler");
        } catch (const std::system_error& e) {
            warning().logWithException(e, "signalhandler: failed to add console signal handler");
        }
    }

    SignalHandler::~SignalHandler() {
        try {
            checkWin32Bool(SetConsoleCtrlHandler(&consoleHandler, FALSE), "SetConsoleCtrlHandler");
            debug().log("signalhandler: removed console signal handler");
        } catch (const std::system_error& e) {
            warning().logWithException(e, "signalhandler: failed to remove console signal handler");
        }
    }

    bool SignalHandler::isExitRequested() const { return exitRequested; }
}
