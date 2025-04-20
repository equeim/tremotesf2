// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <windows.h>
#include <winrt/base.h>

#include <QCoreApplication>

#include "log/log.h"
#include "startup/windowsmessagehandler.h"
#include "ui/darkthemeapplier_windows.h"
#include "ui/systemcolorsprovider.h"
#include "main_windows.h"
#include "windowshelpers.h"

namespace tremotesf {
    WindowsLogger::WindowsLogger() { initWindowsMessageHandler(); }

    WindowsLogger::~WindowsLogger() { deinitWindowsMessageHandler(); }

    WinrtApartment::WinrtApartment() {
        try {
            winrt::init_apartment(winrt::apartment_type::single_threaded);
        } catch (const winrt::hresult_error& e) {
            warning().log("winrt::init_apartment failed: {}", e);
        }
    }

    WinrtApartment::~WinrtApartment() { winrt::uninit_apartment(); }

    void windowsInitApplication() {
        try {
            checkWin32Bool(AllowSetForegroundWindow(ASFW_ANY), "AllowSetForegroundWindow");
        } catch (const std::system_error& e) {
            warning().log(e);
        }
        const auto systemColorsProvider = SystemColorsProvider::createInstance(QCoreApplication::instance());
        applyDarkThemeToPalette(systemColorsProvider);
    }
}
