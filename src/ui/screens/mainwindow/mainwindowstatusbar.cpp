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

#include "log/log.h"
#include "rpc/serverstats.h"
#include "rpc/servers.h"
#include "rpc/rpc.h"
#include "formatutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    MainWindowStatusBar::MainWindowStatusBar(const Rpc* rpc, QWidget* parent) : QStatusBar(parent), mRpc(rpc) {
        setSizeGripEnabled(false);

        setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(this, &QWidget::customContextMenuRequested, this, &MainWindowStatusBar::showContextMenu);

        auto container = new QWidget(this);
        addPermanentWidget(container, 1);

        auto layout = new QHBoxLayout(container);
        // Top/bottom margins are set on mServerLabel below so that they don't affect separators
        layout->setContentsMargins(8, 0, 8, 0);

        mNoServersErrorImage = new QLabel(this);
        mNoServersErrorImage->setPixmap(QIcon::fromTheme("dialog-error"_L1).pixmap(16, 16));
        mNoServersErrorImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        layout->addWidget(mNoServersErrorImage);

        mServerLabel = new QLabel(this);
        mServerLabel->setContentsMargins(0, 5, 0, 5);
        layout->addWidget(mServerLabel);

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

        updateServerLabel();
        QObject::connect(
            Servers::instance(),
            &Servers::currentServerChanged,
            this,
            &MainWindowStatusBar::updateServerLabel
        );
        QObject::connect(
            Servers::instance(),
            &Servers::hasServersChanged,
            this,
            &MainWindowStatusBar::updateServerLabel
        );

        updateStatusLabels();
        QObject::connect(mRpc, &Rpc::statusChanged, this, &MainWindowStatusBar::updateStatusLabels);

        QObject::connect(mRpc->serverStats(), &ServerStats::updated, this, [=, this] {
            mDownloadSpeedLabel->setText(formatutils::formatByteSpeed(mRpc->serverStats()->downloadSpeed()));
            mUploadSpeedLabel->setText(formatutils::formatByteSpeed(mRpc->serverStats()->uploadSpeed()));
            mFreeSpaceLabel->setText(
                QObject::tr("Free space: %1").arg(formatutils::formatByteSize(mRpc->serverStats()->freeSpace()))
            );
        });
    }

    void MainWindowStatusBar::updateLayout() {
        if (Servers::instance()->hasServers()) {
            mNoServersErrorImage->hide();
            mServerLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
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
            mNoServersErrorImage->show();
            mServerLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            mFirstSeparator->hide();
            mStatusLabel->hide();
            mSecondSeparator->hide();
            mDownloadSpeedImage->hide();
            mDownloadSpeedLabel->hide();
            mThirdSeparator->hide();
            mUploadSpeedImage->hide();
            mUploadSpeedLabel->hide();
            mFreeSpaceLabel->hide();
        }
    }

    void MainWindowStatusBar::updateServerLabel() {
        if (Servers::instance()->hasServers()) {
            mServerLabel->setText(
                QString::fromLatin1("%1 (%2)")
                    .arg(Servers::instance()->currentServerName(), Servers::instance()->currentServerAddress())
            );
        } else {
            mServerLabel->setText(qApp->translate("tremotesf", "No servers"));
        }
    }

    void MainWindowStatusBar::updateStatusLabels() {
        mStatusLabel->setText(mRpc->status().toString());
        if (mRpc->error() != RpcError::NoError) {
            mStatusLabel->setToolTip(mRpc->errorMessage());
        } else {
            mStatusLabel->setToolTip({});
        }
    }

    void MainWindowStatusBar::showContextMenu(QPoint pos) {
        auto* const menu = new QMenu(this);
        auto* const group = new QActionGroup(menu);
        group->setExclusive(true);
        menu->setAttribute(Qt::WA_DeleteOnClose, true);
        const auto servers = Servers::instance()->servers();
        const auto currentServerName = Servers::instance()->currentServerName();
        for (const auto& server : servers) {
            auto* const action = menu->addAction(server.name);
            action->setData(server.name);
            action->setCheckable(true);
            group->addAction(action);
            if (server.name == currentServerName) {
                action->setChecked(true);
            }
        }
        menu->addSeparator();
        auto* const connectionSettingsAction = menu->addAction(
            QIcon::fromTheme("network-server"_L1),
            qApp->translate("tremotesf", "&Connection Settings")
        );
        QObject::connect(menu, &QMenu::triggered, this, [this, connectionSettingsAction](QAction* action) {
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
        menu->popup(mapToGlobal(pos));
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
