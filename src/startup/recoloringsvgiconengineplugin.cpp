// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QIconEnginePlugin>
#include <QStyleOptionMenuItem>
#include <QGuiApplication>

#include "literals.h"
#include "recoloringsvgiconengineplugin.h"
#include "target_os.h"
#include "ui/recoloringsvgiconengine.h"

namespace tremotesf {
    namespace {
        thread_local bool drawingSelectedMenuItem = false;

        constexpr auto overrideStyleEnvVariable = "QT_STYLE_OVERRIDE";

        QString defaultStyle() {
            if (qEnvironmentVariableIsSet(overrideStyleEnvVariable)) {
                return qEnvironmentVariable(overrideStyleEnvVariable);
            }
            if constexpr (targetOs == TargetOs::UnixMacOS) {
                return "macOS"_l1;
            }
            return "fusion"_l1;
        }
    }

    RecoloringSvgIconStyle::RecoloringSvgIconStyle(QObject* parent) : QProxyStyle(defaultStyle()) { setParent(parent); }
    void RecoloringSvgIconStyle::drawControl(
        QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget
    ) const {
        if (const auto mi = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
            if (mi->state & State_Selected) {
                drawingSelectedMenuItem = true;
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
        if (drawingSelectedMenuItem) {
            drawingSelectedMenuItem = false;
        }
    }

    class EngineForStyle final : public RecoloringSvgIconEngine {
    public:
        QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
            // QFusionStyle passes QIcon::Active for selected menu items, but RecoloringSvgIconEngine/KIconEngine expects QIcon::Selected
            if (drawingSelectedMenuItem) {
                mode = QIcon::Selected;
            }
            return RecoloringSvgIconEngine::pixmap(size, mode, state);
        }
    };

    class RecoloringSvgIconEnginePlugin final : public QIconEnginePlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QIconEngineFactoryInterface" FILE "recoloringsvgiconengineplugin.json")

    public:
        QIconEngine* create(const QString& file) override {
            auto engine = new EngineForStyle{};
            if (!file.isNull()) {
                engine->addFile(file, QSize(), QIcon::Normal, QIcon::Off);
            }
            return engine;
        }
    };
}

Q_IMPORT_PLUGIN(RecoloringSvgIconEnginePlugin)

#include "recoloringsvgiconengineplugin.moc"
