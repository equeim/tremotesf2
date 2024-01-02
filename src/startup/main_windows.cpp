// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "main_windows.h"

#include <stdexcept>

#include <windows.h>
#include <winrt/base.h>

#include <QApplication>

#include "log/log.h"
#include "startup/windowsmessagehandler.h"
#include "ui/darkthemeapplier_windows.h"
#include "ui/systemcolorsprovider.h"
#include "windowshelpers.h"

namespace tremotesf {
    namespace {
        void onTerminate() {
            const auto exception_ptr = std::current_exception();
            if (exception_ptr) {
                try {
                    std::rethrow_exception(exception_ptr);
                } catch (const std::exception& e) {
                    logWarningWithException(e, "Unhandled exception");
                } catch (const winrt::hresult_error& e) {
                    logWarningWithException(e, "Unhandled exception");
                } catch (...) {
                    logWarning("Unhandled exception of unknown type");
                }
            }
            std::abort();
        }
    }

    WindowsLogger::WindowsLogger() {
        initWindowsMessageHandler();
        std::set_terminate(onTerminate);
    }

    WindowsLogger::~WindowsLogger() { deinitWindowsMessageHandler(); }

    WinrtApartment::WinrtApartment() {
        try {
            winrt::init_apartment(winrt::apartment_type::single_threaded);
        } catch (const winrt::hresult_error& e) {
            logWarning("winrt::init_apartment failed: {}", e);
        }
    }

    WinrtApartment::~WinrtApartment() { winrt::uninit_apartment(); }

    void windowsInitApplication() {
        try {
            checkWin32Bool(AllowSetForegroundWindow(ASFW_ANY), "AllowSetForegroundWindow");
        } catch (const std::system_error& e) {
            logWarning(e);
        }
        const auto systemColorsProvider = SystemColorsProvider::createInstance(QApplication::instance());
        applyDarkThemeToPalette(systemColorsProvider);
    }
}
