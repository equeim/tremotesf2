// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "main_windows.h"

#include <stdexcept>

#include <windows.h>
#include <winrt/base.h>

#include <QApplication>
#include <QIcon>
#include <QIconEnginePlugin>
#include <QProxyStyle>
#include <QStringBuilder>
#include <QStyle>

#include "libtremotesf/literals.h"
#include "libtremotesf/log.h"
#include "tremotesf/startup/windowsmessagehandler.h"
#include "tremotesf/ui/darkthemeapplier.h"
#include "tremotesf/ui/recoloringsvgiconengine.h"
#include "tremotesf/ui/systemcolorsprovider.h"
#include "tremotesf/windowshelpers.h"

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

        class WindowsStyle : public QProxyStyle {
            Q_OBJECT
        public:
            static thread_local bool drawingMenuItem;

            explicit WindowsStyle(QObject* parent = nullptr) : QProxyStyle("fusion"_l1) { setParent(parent); }

            void drawControl(
                ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = nullptr
            ) const override {
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

        class SvgIconEnginePlugin : public QIconEnginePlugin {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QIconEngineFactoryInterface" FILE "svgiconengineplugin.json")

        public:
            QIconEngine* create(const QString& file) override {
                SvgIconEngine* engine = new SvgIconEngine;
                if (!file.isNull()) engine->addFile(file, QSize(), QIcon::Normal, QIcon::Off);
                return engine;
            }
        };
    }

    void windowsInitPrelude() {
        initWindowsMessageHandler();
        std::set_terminate(onTerminate);
    }

    void windowsInitWinrt() {
        try {
            winrt::init_apartment(winrt::apartment_type::single_threaded);
        } catch (const winrt::hresult_error& e) {
            logWarning("winrt::init_apartment failed: {}", e);
        }
    }

    void windowsInitApplication() {
        try {
            checkWin32Bool(AllowSetForegroundWindow(ASFW_ANY), "AllowSetForegroundWindow");
        } catch (const std::system_error& e) {
            logWarning(e);
        }
        QApplication::setStyle(new WindowsStyle(QApplication::instance()));
        QIcon::setThemeSearchPaths({QCoreApplication::applicationDirPath() % '/' % TREMOTESF_BUNDLED_ICONS_DIR ""_l1});
        QIcon::setThemeName(TREMOTESF_BUNDLED_ICON_THEME ""_l1);
        const auto systemColorsProvider = tremotesf::SystemColorsProvider::createInstance(QApplication::instance());
        tremotesf::applyDarkThemeToPalette(systemColorsProvider);
    }

    void windowsDeinitWinrt() {
        try {
            winrt::uninit_apartment();
        } catch (const winrt::hresult_error& e) {
            logWarning("winrt::uninit_apartment failed: {}", e);
        }
    }

    void windowsDeinitPrelude() { deinitWindowsMessageHandler(); }
}

Q_IMPORT_PLUGIN(SvgIconEnginePlugin)

#include "main_windows.moc"
