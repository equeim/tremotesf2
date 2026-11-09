// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindowstatusbar.h"

#include <ranges>

#include <QActionGroup>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QStyleOption>
#include <QToolButton>

#include "log/log.h"
#include "rpc/serversettings.h"
#include "rpc/serverstats.h"
#include "rpc/servers.h"
#include "rpc/rpc.h"
#include "formatutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    MainWindowStatusBar::MainWindowStatusBar(const Rpc* rpc, QWidget* parent) : QStatusBar(parent), mRpc(rpc) {
        setSizeGripEnabled(false);

        auto container = new QWidget(this);
        addPermanentWidget(container, 1);

        mLayout = new QHBoxLayout(container);
        // Top/bottom margins are set on mServerLabel below so that they don't affect separators
        mLayout->setContentsMargins(8, 0, 8, 0);

        mServerButton = new QToolButton(this);
        mServerButton->setPopupMode(QToolButton::InstantPopup);
        mServerButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mServerButton->setAutoRaise(true);
        mServerButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mServerMenu = new QMenu(this);
        mServerButton->setMenu(mServerMenu);
        mLayout->addWidget(mServerButton);

        mServerSpacer = new QWidget(this);
        mServerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mLayout->addWidget(mServerSpacer);

        mFirstSeparator = new StatusBarSeparator(this);
        mLayout->addWidget(mFirstSeparator);

        mStatusLabel = new QLabel(this);
        mLayout->addWidget(mStatusLabel);

        auto altSpeedLimitsButton = new QToolButton(this);
        altSpeedLimitsButton->setAutoRaise(true);
        altSpeedLimitsButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        auto altSpeedLimitsAction = altSpeedLimitsButton->addAction(QIcon(":/turtle.svg"), QString{});
        altSpeedLimitsButton->setDefaultAction(altSpeedLimitsAction);
        altSpeedLimitsAction->setCheckable(true);
        mLayout->addWidget(altSpeedLimitsButton);
        const auto updateAltSpeedLimitsButton = [=, this] {
            if (mRpc->serverSettings()->data().alternativeSpeedLimitsEnabled) {
                altSpeedLimitsAction->setChecked(true);
                altSpeedLimitsAction->setText(
                    qApp->translate("tremotesf", "Alternative speed limits are enabled. Click to disable")
                );
            } else {
                altSpeedLimitsAction->setChecked(false);
                altSpeedLimitsAction->setText(
                    qApp->translate("tremotesf", "Alternative speed limits are disabled. Click to enable")
                );
            }
        };
        updateAltSpeedLimitsButton();
        QObject::connect(mRpc->serverSettings(), &ServerSettings::changed, this, updateAltSpeedLimitsButton);
        QObject::connect(altSpeedLimitsAction, &QAction::triggered, this, [=, this](bool checked) {
            mRpc->serverSettings()->setAlternativeSpeedLimitsEnabled(checked);
            updateAltSpeedLimitsButton();
        });

        mLayout->addWidget(new StatusBarSeparator(this));

        auto downloadSpeedImage = new QLabel(this);
        downloadSpeedImage->setPixmap(QIcon::fromTheme("go-down"_L1).pixmap(16, 16));
        downloadSpeedImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mLayout->addWidget(downloadSpeedImage);

        auto downloadSpeedLabel = new QLabel(this);
        mLayout->addWidget(downloadSpeedLabel);

        mLayout->addWidget(new StatusBarSeparator(this));

        auto uploadSpeedImage = new QLabel(this);
        uploadSpeedImage->setPixmap(QIcon::fromTheme("go-up"_L1).pixmap(16, 16));
        uploadSpeedImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mLayout->addWidget(uploadSpeedImage);

        auto uploadSpeedLabel = new QLabel(this);
        mLayout->addWidget(uploadSpeedLabel);

        mLayout->addWidget(new StatusBarSeparator(this));

        auto freeSpaceLabel = new QLabel(this);
        freeSpaceLabel->setContentsMargins(8, 0, 0, 0);
        mLayout->addWidget(freeSpaceLabel);

        updateLayout();
        QObject::connect(mRpc, &Rpc::connectionStateChanged, this, &MainWindowStatusBar::updateLayout);
        QObject::connect(Servers::instance(), &Servers::hasServersChanged, this, &MainWindowStatusBar::updateLayout);

        updateServerText();
        QObject::connect(
            Servers::instance(),
            &Servers::currentServerChanged,
            this,
            &MainWindowStatusBar::updateServerText
        );
        QObject::connect(
            Servers::instance(),
            &Servers::hasServersChanged,
            this,
            &MainWindowStatusBar::updateServerText
        );
        updateServerMenu();
        QObject::connect(
            Servers::instance(),
            &Servers::currentServerChanged,
            this,
            &MainWindowStatusBar::updateServerMenu
        );
        QObject::connect(Servers::instance(), &Servers::serversChanged, this, &MainWindowStatusBar::updateServerMenu);

        updateStatusLabels();
        QObject::connect(mRpc, &Rpc::statusChanged, this, &MainWindowStatusBar::updateStatusLabels);

        const auto updateSpeedAndFreeSpaceLabels = [=, this] {
            downloadSpeedLabel->setText(formatutils::formatByteSpeed(mRpc->serverStats()->downloadSpeed()));
            uploadSpeedLabel->setText(formatutils::formatByteSpeed(mRpc->serverStats()->uploadSpeed()));
            freeSpaceLabel->setText(qApp->translate("tremotesf", "Free space: %1")
                                        .arg(formatutils::formatByteSize(mRpc->serverStats()->freeSpace())));
        };
        updateSpeedAndFreeSpaceLabels();
        QObject::connect(mRpc->serverStats(), &ServerStats::updated, this, updateSpeedAndFreeSpaceLabels);
    }

    namespace {
        auto widgetsAfter(QWidget* widget, QLayout* layout) {
            return std::views::iota(layout->indexOf(widget) + 1, layout->count())
                   | std::views::transform([layout](int i) { return layout->itemAt(i)->widget(); });
        }
    }

    void MainWindowStatusBar::updateLayout() {
        if (Servers::instance()->hasServers()) {
            mServerButton->setIcon(QIcon{});
            mServerSpacer->hide();
            mFirstSeparator->show();
            mStatusLabel->show();
            if (mRpc->connectionState() == Rpc::ConnectionState::Connected) {
                for (auto* widget : widgetsAfter(mStatusLabel, mLayout)) {
                    widget->show();
                }
            } else {
                for (auto* widget : widgetsAfter(mStatusLabel, mLayout)) {
                    widget->hide();
                }
            }
        } else {
            mServerButton->setIcon(QIcon::fromTheme("dialog-error"_L1));
            mServerSpacer->show();
            for (auto* widget : widgetsAfter(mServerSpacer, mLayout)) {
                widget->hide();
            }
        }
    }

    void MainWindowStatusBar::updateServerText() {
        if (Servers::instance()->hasServers()) {
            mServerButton->setText(
                QString("%1 (%2)"_L1)
                    .arg(Servers::instance()->currentServerName(), Servers::instance()->currentServerAddress())
            );
        } else {
            mServerButton->setText(qApp->translate("tremotesf", "No servers"));
        }
    }

    void MainWindowStatusBar::updateServerMenu() {
        mServerMenu->clear();
        auto* const group = new QActionGroup(mServerMenu);
        group->setExclusive(true);
        const auto servers = Servers::instance()->servers();
        const auto currentServerName = Servers::instance()->currentServerName();
        for (const auto& server : servers) {
            auto* const action = mServerMenu->addAction(server.name);
            action->setData(server.name);
            action->setCheckable(true);
            group->addAction(action);
            if (server.name == currentServerName) {
                action->setChecked(true);
            }
        }
        mServerMenu->addSeparator();
        auto* const connectionSettingsAction = mServerMenu->addAction(
            QIcon::fromTheme("network-server"_L1),
            qApp->translate("tremotesf", "&Connection Settings")
        );
        QObject::connect(mServerMenu, &QMenu::triggered, this, [this, connectionSettingsAction](QAction* action) {
            if (action == connectionSettingsAction) {
                emit showConnectionSettingsDialog();
            } else {
                const auto selectedName = action->data().toString();
                const auto servers = Servers::instance()->servers();
                if (std::ranges::contains(servers, selectedName, &Server::name)) {
                    Servers::instance()->setCurrentServer(selectedName);
                } else {
                    warning().log("Selected server {} which no longer exists", selectedName);
                }
            }
        });
    }

    void MainWindowStatusBar::updateStatusLabels() {
        mStatusLabel->setText(mRpc->status().toString());
        if (mRpc->error() != RpcError::NoError) {
            mStatusLabel->setToolTip(mRpc->errorMessage());
        } else {
            mStatusLabel->setToolTip({});
        }
    }

    StatusBarSeparator::StatusBarSeparator(QWidget* parent) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }

    QSize StatusBarSeparator::sizeHint() const {
        QStyleOption opt{};
        opt.initFrom(this);
        opt.state.setFlag(QStyle::State_Horizontal);
        const int extent = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, &opt, this);
        return {extent, extent};
    }

    void StatusBarSeparator::paintEvent(QPaintEvent*) {
        QPainter p(this);
        QStyleOption opt{};
        opt.initFrom(this);
        opt.state.setFlag(QStyle::State_Horizontal);
        style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, this);
    }
}
