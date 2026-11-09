// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindowstatusbar.h"

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

        auto layout = new QHBoxLayout(container);
        // Top/bottom margins are set on mServerLabel below so that they don't affect separators
        layout->setContentsMargins(8, 0, 8, 0);

        mServerButton = new QToolButton(this);
        mServerButton->setPopupMode(QToolButton::InstantPopup);
        mServerButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        mServerButton->setAutoRaise(true);
        mServerButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        mServerMenu = new QMenu(this);
        mServerButton->setMenu(mServerMenu);
        layout->addWidget(mServerButton);

        mServerSpacer = new QWidget(this);
        mServerSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(mServerSpacer);

        mFirstSeparator = new StatusBarSeparator(this);
        layout->addWidget(mFirstSeparator);

        mStatusLabel = new QLabel(this);
        layout->addWidget(mStatusLabel);

        mSecondSeparator = new StatusBarSeparator(this);
        layout->addWidget(mSecondSeparator);

        mDownloadSpeedImage = new QLabel(this);
        mDownloadSpeedImage->setPixmap(QIcon::fromTheme("go-down"_L1).pixmap(16, 16));
        mDownloadSpeedImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        layout->addWidget(mDownloadSpeedImage);

        mDownloadSpeedLabel = new QLabel(this);
        layout->addWidget(mDownloadSpeedLabel);

        mThirdSeparator = new StatusBarSeparator(this);
        layout->addWidget(mThirdSeparator);

        mUploadSpeedImage = new QLabel(this);
        mUploadSpeedImage->setPixmap(QIcon::fromTheme("go-up"_L1).pixmap(16, 16));
        mUploadSpeedImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        layout->addWidget(mUploadSpeedImage);

        mUploadSpeedLabel = new QLabel(this);
        layout->addWidget(mUploadSpeedLabel);

        mFourthSeparator = new StatusBarSeparator(this);
        layout->addWidget(mFourthSeparator);

        mFreeSpaceLabel = new QLabel(this);
        mFreeSpaceLabel->setContentsMargins(8, 0, 0, 0);
        layout->addWidget(mFreeSpaceLabel);

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

        QObject::connect(mRpc->serverStats(), &ServerStats::updated, this, [=, this] {
            mDownloadSpeedLabel->setText(formatutils::formatByteSpeed(mRpc->serverStats()->downloadSpeed()));
            mUploadSpeedLabel->setText(formatutils::formatByteSpeed(mRpc->serverStats()->uploadSpeed()));
            mFreeSpaceLabel->setText(qApp->translate("tremotesf", "Free space: %1")
                                         .arg(formatutils::formatByteSize(mRpc->serverStats()->freeSpace())));
        });
    }

    void MainWindowStatusBar::updateLayout() {
        if (Servers::instance()->hasServers()) {
            mServerButton->setIcon(QIcon{});
            mServerSpacer->hide();
            mFirstSeparator->show();
            mStatusLabel->show();
            if (mRpc->connectionState() == Rpc::ConnectionState::Connected) {
                mSecondSeparator->show();
                mDownloadSpeedImage->show();
                mDownloadSpeedLabel->show();
                mThirdSeparator->show();
                mUploadSpeedImage->show();
                mUploadSpeedLabel->show();
                mFourthSeparator->show();
                mFreeSpaceLabel->show();
            } else {
                mSecondSeparator->hide();
                mDownloadSpeedImage->hide();
                mDownloadSpeedLabel->hide();
                mThirdSeparator->hide();
                mUploadSpeedImage->hide();
                mUploadSpeedLabel->hide();
                mFourthSeparator->hide();
                mFreeSpaceLabel->hide();
            }
        } else {
            mServerButton->setIcon(QIcon::fromTheme("dialog-error"_L1));
            mServerSpacer->show();
            mFirstSeparator->setVisible(false);
            mStatusLabel->hide();
            mSecondSeparator->hide();
            mDownloadSpeedImage->hide();
            mDownloadSpeedLabel->hide();
            mThirdSeparator->hide();
            mUploadSpeedImage->hide();
            mUploadSpeedLabel->hide();
            mFourthSeparator->hide();
            mFreeSpaceLabel->hide();
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
