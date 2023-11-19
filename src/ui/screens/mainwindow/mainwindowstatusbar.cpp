// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindowstatusbar.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>

#include <KSeparator>

#include "rpc/serverstats.h"
#include "rpc/servers.h"
#include "rpc/rpc.h"
#include "formatutils.h"

namespace tremotesf {
    MainWindowStatusBar::MainWindowStatusBar(const Rpc* rpc, QWidget* parent) : QStatusBar(parent), mRpc(rpc) {
        setSizeGripEnabled(false);

        auto container = new QWidget(this);
        addPermanentWidget(container, 1);

        auto layout = new QHBoxLayout(container);
        layout->setContentsMargins(8, 4, 8, 4);

        mNoServersErrorImage = new QLabel(this);
        mNoServersErrorImage->setPixmap(QIcon::fromTheme("dialog-error"_l1).pixmap(16, 16));
        mNoServersErrorImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        layout->addWidget(mNoServersErrorImage);

        mServerLabel = new QLabel(this);
        layout->addWidget(mServerLabel);

        mFirstSeparator = new KSeparator(Qt::Vertical, this);
        layout->addWidget(mFirstSeparator);

        mStatusLabel = new QLabel(this);
        layout->addWidget(mStatusLabel);

        mSecondSeparator = new KSeparator(Qt::Vertical, this);
        layout->addWidget(mSecondSeparator);

        mDownloadSpeedImage = new QLabel(this);
        mDownloadSpeedImage->setPixmap(QIcon::fromTheme("go-down"_l1).pixmap(16, 16));
        mDownloadSpeedImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        layout->addWidget(mDownloadSpeedImage);

        mDownloadSpeedLabel = new QLabel(this);
        layout->addWidget(mDownloadSpeedLabel);

        mThirdSeparator = new KSeparator(Qt::Vertical, this);
        layout->addWidget(mThirdSeparator);

        mUploadSpeedImage = new QLabel(this);
        mUploadSpeedImage->setPixmap(QIcon::fromTheme("go-up"_l1).pixmap(16, 16));
        mUploadSpeedImage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        layout->addWidget(mUploadSpeedImage);

        mUploadSpeedLabel = new QLabel(this);
        layout->addWidget(mUploadSpeedLabel);

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
            } else {
                mSecondSeparator->hide();
                mDownloadSpeedImage->hide();
                mDownloadSpeedLabel->hide();
                mThirdSeparator->hide();
                mUploadSpeedImage->hide();
                mUploadSpeedLabel->hide();
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
        }
    }

    void MainWindowStatusBar::updateServerLabel() {
        if (Servers::instance()->hasServers()) {
            mServerLabel->setText(QString::fromLatin1("%1 (%2)").arg(
                Servers::instance()->currentServerName(),
                Servers::instance()->currentServerAddress()
            ));
        } else {
            mServerLabel->setText(qApp->translate("tremotesf", "No servers"));
        }
    }

    void MainWindowStatusBar::updateStatusLabels() {
        mStatusLabel->setText(mRpc->status().toString());
        if (mRpc->error() != RpcError::NoError) {
            setToolTip(mRpc->errorMessage());
        } else {
            setToolTip({});
        }
    }
}
