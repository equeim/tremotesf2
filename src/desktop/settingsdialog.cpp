/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "settingsdialog.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "../settings.h"

namespace tremotesf
{
    SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(qApp->translate("tremotesf", "Options"));

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        mConnectOnStartupCheckBox = new QCheckBox(qApp->translate("tremotesf", "Connect to server on startup"), this);
        layout->addWidget(mConnectOnStartupCheckBox);

        auto notificationsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Notifications"), this);
        auto notificationsGroupBoxLayout = new QVBoxLayout(notificationsGroupBox);
        notificationsGroupBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        mNotificationOnDisconnectingCheckBox = new QCheckBox(qApp->translate("tremotesf", "Show a notification on disconnecting from server"), this);
        notificationsGroupBoxLayout->addWidget(mNotificationOnDisconnectingCheckBox);

        mNotificationOnAddingTorrentCheckBox = new QCheckBox(qApp->translate("tremotesf", "Show a notification when torrents are added"), this);
        notificationsGroupBoxLayout->addWidget(mNotificationOnAddingTorrentCheckBox);

        mNotificationOfFinishedTorrentsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Show a notification when torrents finish"), this);
        notificationsGroupBoxLayout->addWidget(mNotificationOfFinishedTorrentsCheckBox);

        mTrayIconCheckBox = new QCheckBox(qApp->translate("tremotesf", "Show icon in the notification area"), this);
        notificationsGroupBoxLayout->addWidget(mTrayIconCheckBox);

        layout->addWidget(notificationsGroupBox);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
        layout->addWidget(dialogButtonBox);

        auto settings = Settings::instance();
        mConnectOnStartupCheckBox->setChecked(settings->connectOnStartup());
        mNotificationOnDisconnectingCheckBox->setChecked(settings->notificationOnDisconnecting());
        mNotificationOnAddingTorrentCheckBox->setChecked(settings->notificationOnAddingTorrent());
        mNotificationOfFinishedTorrentsCheckBox->setChecked(settings->notificationOfFinishedTorrents());
        mTrayIconCheckBox->setChecked(settings->showTrayIcon());
    }

    void SettingsDialog::accept()
    {
        auto settings = Settings::instance();
        settings->setConnectOnStartup(mConnectOnStartupCheckBox->isChecked());
        settings->setNotificationOnDisconnecting(mNotificationOnDisconnectingCheckBox->isChecked());
        settings->setNotificationOnAddingTorrent(mNotificationOnAddingTorrentCheckBox->isChecked());
        settings->setNotificationOfFinishedTorrents(mNotificationOfFinishedTorrentsCheckBox->isChecked());
        settings->setShowTrayIcon(mTrayIconCheckBox->isChecked());
        QDialog::accept();
    }
}
