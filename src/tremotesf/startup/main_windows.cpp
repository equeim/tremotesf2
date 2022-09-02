#include "main_windows.h"

#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winrt/base.h>

#include <QApplication>
#include <QIcon>
#include <QIconEnginePlugin>
#include <QProxyStyle>
#include <QStringBuilder>
#include <QStyle>

#include "libtremotesf/log.h"
#include "tremotesf/ui/darkthemeapplier.h"
#include "tremotesf/ui/recoloringsvgiconengine.h"
#include "tremotesf/ui/systemcolorsprovider.h"
#include "tremotesf/windowshelpers.h"

namespace tremotesf {
    namespace {
        void on_terminate() {
            const auto exception_ptr = std::current_exception();
            if (exception_ptr) {
                try {
                    std::rethrow_exception(exception_ptr);
                } catch (const std::exception& e) {
                    logWarning("Unhandled exception: {}", e);
                } catch (const winrt::hresult_error& e) {
                    logWarning("Unhandled exception: {}", e);
                } catch (...) {
                    logWarning("Unhandled exception of unkown type");
                }
            }
        }

        class WindowsStyle : public QProxyStyle {
            Q_OBJECT
        public:
            static thread_local bool drawingMenuItem;

            explicit WindowsStyle(QObject* parent = nullptr) : QProxyStyle(QLatin1String("fusion")) {
                setParent(parent);
            }

            void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr) const override {
                if (element == CE_MenuItem) {
                    drawingMenuItem = true;
                }
                QProxyStyle::drawControl(element, option, painter, widget);
                if (element == CE_MenuItem) {
                    drawingMenuItem = false;
                }
            }
        };

        thread_local bool WindowsStyle::drawingMenuItem = false;

        class SvgIconEngine : public RecoloringSvgIconEngine {
        public:
            QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
                // QFusionStyle passes QIcon::Active for selected menu items, but KIconEngine expects QIcon::Selected
                if (WindowsStyle::drawingMenuItem && mode == QIcon::Active) {
                    mode = QIcon::Selected;
                }
                return RecoloringSvgIconEngine::pixmap(size, mode, state);
            }
        };

        class SvgIconEnginePlugin : public QIconEnginePlugin
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QIconEngineFactoryInterface" FILE "svgiconengineplugin.json")

        public:
            QIconEngine* create(const QString& file) override
            {
                SvgIconEngine* engine = new SvgIconEngine;
                if (!file.isNull())
                    engine->addFile(file, QSize(), QIcon::Normal, QIcon::Off);
                return engine;
            }
        };
    }

	void windowsInitPreApplication() {
        std::set_terminate(on_terminate);

        try {
            winrt::check_bool(SetConsoleOutputCP(GetACP()));
        } catch (const winrt::hresult_error& e) {
            logWarning("SetConsoleOutputCP failed: {}", e);
        }

        try {
            winrt::check_bool(AllowSetForegroundWindow(ASFW_ANY));
        } catch (const winrt::hresult_error& e) {
            logWarning("AllowSetForegroundWindow failed: {}", e);
        }
	}

    void windowsInitPostApplication() {
        try {
            winrt::init_apartment();
        } catch (const winrt::hresult_error& e) {
            if (e.code() != RPC_E_CHANGED_MODE) {
                logWarning("winrt::init_apartment failed: {}", e);
            }
        }
        QApplication::setStyle(new WindowsStyle(QApplication::instance()));
        QIcon::setThemeSearchPaths({QCoreApplication::applicationDirPath() % QLatin1Char('/') % QLatin1String(TREMOTESF_BUNDLED_ICONS_DIR)});
        QIcon::setThemeName(QLatin1String(TREMOTESF_BUNDLED_ICON_THEME));

        const auto systemColorsProvider = tremotesf::SystemColorsProvider::createInstance(QApplication::instance());
        tremotesf::applyDarkThemeToPalette(systemColorsProvider);
    }

    void windowsDeinit() {
        try {
            winrt::uninit_apartment();
        } catch (const winrt::hresult_error& e) {
            const auto msg = e.message();
            logWarning("winrt::uninit_apartment failed: {}: {}", QString::fromWCharArray(msg.c_str(), msg.size()));
        }
    }
}

Q_IMPORT_PLUGIN(SvgIconEnginePlugin)

#include "main_windows.moc"
