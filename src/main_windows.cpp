#include "main_windows.h"

#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winrt/base.h>

#include <QApplication>
#include <QIcon>
#include <QStringBuilder>

#include "libtremotesf/println.h"
#include "desktop/darkthemeapplier.h"
#include "desktop/systemcolorsprovider.h"
#include "utils.h"

namespace tremotesf {
    namespace {
        void on_terminate() {
            const auto exception_ptr = std::current_exception();
            if (exception_ptr) {
                try {
                    std::rethrow_exception(exception_ptr);
                } catch (const std::exception& e) {
                    printlnWarning("Unhandled exception: {}", e.what());
                }
            }
        }
    }

	void windowsInitPreApplication() {
        std::set_terminate(on_terminate);

        try {
            tremotesf::Utils::callWinApiFunctionWithLastError([] { return SetConsoleOutputCP(GetACP()); });
        } catch (const std::exception& e) {
            printlnWarning("SetConsoleOutputCP failed: {}", e.what());
        }

        try {
            tremotesf::Utils::callWinApiFunctionWithLastError([] { return AllowSetForegroundWindow(ASFW_ANY); });
        } catch (const std::exception& e) {
            printlnWarning("AllowSetForegroundWindow failed: {}", e.what());
        }
	}

    void windowsInitPostApplication() {
        try {
            winrt::init_apartment();
        } catch (const winrt::hresult_error& e) {
            if (e.code() != RPC_E_CHANGED_MODE) {
                const auto msg = e.message();
                printlnWarning("winrt::init_apartment failed: {}: {}", QString::fromWCharArray(msg.c_str(), msg.size()));
            }
        }
        QApplication::setStyle(QLatin1String("fusion"));
        QIcon::setThemeSearchPaths({ QCoreApplication::applicationDirPath() % QLatin1Char('/') % QLatin1String(TREMOTESF_BUNDLED_ICONS_DIR) });
        QIcon::setThemeName(QLatin1String(TREMOTESF_BUNDLED_ICON_THEME));

        const auto systemColorsProvider = tremotesf::SystemColorsProvider::createInstance(QApplication::instance());
        tremotesf::applyDarkThemeToPalette(systemColorsProvider);
    }

    void windowsDeinit() {
        try {
            winrt::uninit_apartment();
        } catch (const winrt::hresult_error& e) {
            const auto msg = e.message();
            printlnWarning("winrt::uninit_apartment failed: {}: {}", QString::fromWCharArray(msg.c_str(), msg.size()));
        }
    }
}
