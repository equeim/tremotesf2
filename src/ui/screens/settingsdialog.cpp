// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "stdutils.h"
#include "target_os.h"
#include "settings.h"
#include "ui/systemcolorsprovider.h"

namespace tremotesf {
    SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Options"));

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        QComboBox* darkThemeComboBox{};
        std::vector<Settings::DarkThemeMode> darkThemeComboBoxValues{};
        QCheckBox* systemAccentColorCheckBox{};
        if constexpr (isTargetOsWindows) {
            //: Options section
            auto appearanceGroupBox = new QGroupBox(qApp->translate("tremotesf", "Appearance"), this);
            auto appearanceGroupBoxLayout = new QFormLayout(appearanceGroupBox);

            darkThemeComboBox = new QComboBox(this);
            //: Dark theme mode
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "Follow system"));
            darkThemeComboBoxValues.push_back(Settings::DarkThemeMode::FollowSystem);
            //: Dark theme mode
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "On"));
            darkThemeComboBoxValues.push_back(Settings::DarkThemeMode::On);
            //: Dark theme mode
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "Off"));
            darkThemeComboBoxValues.push_back(Settings::DarkThemeMode::Off);
            appearanceGroupBoxLayout->addRow(qApp->translate("tremotesf", "Dark theme"), darkThemeComboBox);

            //: Check box label
            systemAccentColorCheckBox = new QCheckBox(qApp->translate("tremotesf", "Use system accent color"), this);
            appearanceGroupBoxLayout->addRow(systemAccentColorCheckBox);

            layout->addWidget(appearanceGroupBox);
        }

        //: Options section
        auto connectionGroupBox = new QGroupBox(qApp->translate("tremotesf", "Connection"), this);
        auto connectionGroupBoxBoxLayout = new QVBoxLayout(connectionGroupBox);

        auto connectOnStartupCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Connect to server on startup"),
            this
        );
        connectionGroupBoxBoxLayout->addWidget(connectOnStartupCheckBox);

        layout->addWidget(connectionGroupBox);

        //: Options section
        auto addTorrentsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Adding torrents"), this);
        auto addTorrentsGroupBoxBoxLayout = new QVBoxLayout(addTorrentsGroupBox);

        auto rememberOpenTorrentDirCheckbox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Remember location of last opened torrent file"),
            this
        );
        addTorrentsGroupBoxBoxLayout->addWidget(rememberOpenTorrentDirCheckbox);

        auto rememberAddTorrentParameters = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Remember parameters of last added torrent"),
            this
        );
        addTorrentsGroupBoxBoxLayout->addWidget(rememberAddTorrentParameters);

        auto fillTorrentLinkFromKeyboardCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Automatically fill link from clipboard when adding torrent link"),
            this
        );
        addTorrentsGroupBoxBoxLayout->addWidget(fillTorrentLinkFromKeyboardCheckBox);
        auto pasteTipLabel = new QLabel(
            //: %1 is a key binding, e.g. "Ctrl + C"
            qApp->translate("tremotesf", "Tip: you can also press %1 in main window to add torrents from clipboard")
                .arg(QKeySequence(QKeySequence::Paste).toString(QKeySequence::NativeText))
        );
        addTorrentsGroupBoxBoxLayout->addWidget(pasteTipLabel);

        layout->addWidget(addTorrentsGroupBox);

        //: Options section
        auto notificationsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Notifications"), this);
        auto notificationsGroupBoxLayout = new QVBoxLayout(notificationsGroupBox);
        notificationsGroupBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto notificationOnDisconnectingCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Notify when disconnecting from server"),
            this
        );
        notificationsGroupBoxLayout->addWidget(notificationOnDisconnectingCheckBox);

        auto notificationOnAddingTorrentCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Notify on added torrents"),
            this
        );
        notificationsGroupBoxLayout->addWidget(notificationOnAddingTorrentCheckBox);

        auto notificationOfFinishedTorrentsCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Notify on finished torrents"),
            this
        );
        notificationsGroupBoxLayout->addWidget(notificationOfFinishedTorrentsCheckBox);

        auto trayIconCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Show icon in the notification area"),
            this
        );
        notificationsGroupBoxLayout->addWidget(trayIconCheckBox);

        //: Options section
        auto whenConnectingGroupBox = new QGroupBox(qApp->translate("tremotesf", "When connecting to server"), this);
        auto whenConnectingGroupBoxLayout = new QVBoxLayout(whenConnectingGroupBox);
        whenConnectingGroupBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto addedSinceLastConnectionCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Notify on added torrents since last connection to server"),
            this
        );
        whenConnectingGroupBoxLayout->addWidget(addedSinceLastConnectionCheckBox);

        auto finishedSinceLastConnectionCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Notify on finished torrents since last connection to server"),
            this
        );
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
        rememberOpenTorrentDirCheckbox->setChecked(settings->rememberOpenTorrentDir());
        rememberAddTorrentParameters->setChecked(settings->rememberAddTorrentParameters());
        fillTorrentLinkFromKeyboardCheckBox->setChecked(settings->fillTorrentLinkFromClipboard());
        trayIconCheckBox->setChecked(settings->showTrayIcon());
        addedSinceLastConnectionCheckBox->setChecked(settings->notificationsOnAddedTorrentsSinceLastConnection());
        finishedSinceLastConnectionCheckBox->setChecked(settings->notificationsOnFinishedTorrentsSinceLastConnection());

        if constexpr (isTargetOsWindows) {
            darkThemeComboBox->setCurrentIndex(
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                indexOfCasted<int>(darkThemeComboBoxValues, settings->darkThemeMode()).value()
            );
            if (systemAccentColorCheckBox) {
                systemAccentColorCheckBox->setChecked(settings->useSystemAccentColor());
            }
        }

        QObject::connect(this, &SettingsDialog::accepted, this, [=] {
            auto settings = Settings::instance();
            settings->setConnectOnStartup(connectOnStartupCheckBox->isChecked());
            settings->setNotificationOnDisconnecting(notificationOnDisconnectingCheckBox->isChecked());
            settings->setNotificationOnAddingTorrent(notificationOnAddingTorrentCheckBox->isChecked());
            settings->setNotificationOfFinishedTorrents(notificationOfFinishedTorrentsCheckBox->isChecked());
            settings->setShowTrayIcon(trayIconCheckBox->isChecked());
            settings->setNotificationsOnAddedTorrentsSinceLastConnection(addedSinceLastConnectionCheckBox->isChecked());
            settings->setNotificationsOnFinishedTorrentsSinceLastConnection(
                finishedSinceLastConnectionCheckBox->isChecked()
            );
            settings->setRememberOpenTorrentDir(rememberOpenTorrentDirCheckbox->isChecked());
            settings->setRememberTorrentAddParameters(rememberAddTorrentParameters->isChecked());
            settings->setFillTorrentLinkFromClipboard(fillTorrentLinkFromKeyboardCheckBox->isChecked());
            if constexpr (isTargetOsWindows) {
                if (const int index = darkThemeComboBox->currentIndex(); index != -1) {
                    settings->setDarkThemeMode(darkThemeComboBoxValues[static_cast<size_t>(index)]);
                }
                if (systemAccentColorCheckBox) {
                    settings->setUseSystemAccentColor(systemAccentColorCheckBox->isChecked());
                }
            }
        });
    }
}
