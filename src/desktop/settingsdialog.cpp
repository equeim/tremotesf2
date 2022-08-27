/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "libtremotesf/stdutils.h"
#include "libtremotesf/target_os.h"
#include "../settings.h"
#include "systemcolorsprovider.h"

namespace tremotesf
{
    SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(qApp->translate("tremotesf", "Options"));

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        QComboBox* darkThemeComboBox{};
        std::vector<Settings::DarkThemeMode> darkThemeComboBoxValues{};
        QCheckBox* systemAccentColorCheckBox{};
        if constexpr (isTargetOsWindows) {
            auto appearanceGroupBox = new QGroupBox(qApp->translate("tremotesf", "Appearance"), this);
            auto appearanceGroupBoxLayout = new QFormLayout(appearanceGroupBox);

            darkThemeComboBox = new QComboBox(this);
            if (SystemColorsProvider::isDarkThemeFollowSystemSupported()) {
                darkThemeComboBox->addItem(qApp->translate("tremotesf", "Follow system"));
                darkThemeComboBoxValues.push_back(Settings::DarkThemeMode::FollowSystem);
            }
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "On"));
            darkThemeComboBoxValues.push_back(Settings::DarkThemeMode::On);
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "Off"));
            darkThemeComboBoxValues.push_back(Settings::DarkThemeMode::Off);
            appearanceGroupBoxLayout->addRow(qApp->translate("tremotesf", "Dark theme"), darkThemeComboBox);

            systemAccentColorCheckBox = new QCheckBox(qApp->translate("tremotesf", "Use system accent color"), this);
            appearanceGroupBoxLayout->addRow(systemAccentColorCheckBox);

            layout->addWidget(appearanceGroupBox);
        }

        auto connectionGroupBox = new QGroupBox(qApp->translate("tremotesf", "Connection"), this);
        auto connectionGroupBoxBoxLayout = new QVBoxLayout(connectionGroupBox);

        auto connectOnStartupCheckBox = new QCheckBox(qApp->translate("tremotesf", "Connect to server on startup"), this);
        connectionGroupBoxBoxLayout->addWidget(connectOnStartupCheckBox);

        layout->addWidget(connectionGroupBox);

        auto notificationsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Notifications"), this);
        auto notificationsGroupBoxLayout = new QVBoxLayout(notificationsGroupBox);
        notificationsGroupBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto notificationOnDisconnectingCheckBox = new QCheckBox(qApp->translate("tremotesf", "Notify when disconnecting from server"), this);
        notificationsGroupBoxLayout->addWidget(notificationOnDisconnectingCheckBox);

        auto notificationOnAddingTorrentCheckBox = new QCheckBox(qApp->translate("tremotesf", "Notify on added torrents"), this);
        notificationsGroupBoxLayout->addWidget(notificationOnAddingTorrentCheckBox);

        auto notificationOfFinishedTorrentsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Notify on finished torrents"), this);
        notificationsGroupBoxLayout->addWidget(notificationOfFinishedTorrentsCheckBox);

        auto trayIconCheckBox = new QCheckBox(qApp->translate("tremotesf", "Show icon in the notification area"), this);
        notificationsGroupBoxLayout->addWidget(trayIconCheckBox);

        auto whenConnectingGroupBox = new QGroupBox(qApp->translate("tremotesf", "When connecting to server"), this);
        auto whenConnectingGroupBoxLayout = new QVBoxLayout(whenConnectingGroupBox);
        whenConnectingGroupBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto addedSinceLastConnectionCheckBox = new QCheckBox(qApp->translate("tremotesf", "Notify on added torrents since last connection to server"), this);
        whenConnectingGroupBoxLayout->addWidget(addedSinceLastConnectionCheckBox);

        auto finishedSinceLastConnectionCheckBox = new QCheckBox(qApp->translate("tremotesf", "Notify on finished torrents since last connection to server"), this);
        whenConnectingGroupBoxLayout->addWidget(finishedSinceLastConnectionCheckBox);

        notificationsGroupBoxLayout->addWidget(whenConnectingGroupBox);

        layout->addWidget(notificationsGroupBox);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
        layout->addWidget(dialogButtonBox);

        setMinimumSize(minimumSizeHint());

        auto settings = Settings::instance();
        connectOnStartupCheckBox->setChecked(settings->connectOnStartup());
        notificationOnDisconnectingCheckBox->setChecked(settings->notificationOnDisconnecting());
        notificationOnAddingTorrentCheckBox->setChecked(settings->notificationOnAddingTorrent());
        notificationOfFinishedTorrentsCheckBox->setChecked(settings->notificationOfFinishedTorrents());
        trayIconCheckBox->setChecked(settings->showTrayIcon());
        addedSinceLastConnectionCheckBox->setChecked(settings->notificationsOnAddedTorrentsSinceLastConnection());
        finishedSinceLastConnectionCheckBox->setChecked(settings->notificationsOnFinishedTorrentsSinceLastConnection());

        if constexpr (isTargetOsWindows) {
            darkThemeComboBox->setCurrentIndex(index_of_i(darkThemeComboBoxValues, settings->darkThemeMode()));
            systemAccentColorCheckBox->setChecked(settings->useSystemAccentColor());
        }


        QObject::connect(this, &SettingsDialog::accepted, this, [=] {
            auto settings = Settings::instance();
            settings->setConnectOnStartup(connectOnStartupCheckBox->isChecked());
            settings->setNotificationOnDisconnecting(notificationOnDisconnectingCheckBox->isChecked());
            settings->setNotificationOnAddingTorrent(notificationOnAddingTorrentCheckBox->isChecked());
            settings->setNotificationOfFinishedTorrents(notificationOfFinishedTorrentsCheckBox->isChecked());
            settings->setShowTrayIcon(trayIconCheckBox->isChecked());
            settings->setNotificationsOnAddedTorrentsSinceLastConnection(addedSinceLastConnectionCheckBox->isChecked());
            settings->setNotificationsOnFinishedTorrentsSinceLastConnection(finishedSinceLastConnectionCheckBox->isChecked());
            if constexpr (isTargetOsWindows) {
                if (int index = darkThemeComboBox->currentIndex(); index != -1) {
                    settings->setDarkThemeMode(darkThemeComboBoxValues[static_cast<size_t>(index)]);
                }
                settings->setUseSystemAccentColor(systemAccentColorCheckBox->isChecked());
            }
        });
    }
}
