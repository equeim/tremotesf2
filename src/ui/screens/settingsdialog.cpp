// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsdialog.h"

#include <array>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>

#include <KMessageWidget>

#include "stdutils.h"
#include "target_os.h"
#include "settings.h"
#include "rpc/rpc.h"
#include "ui/systemcolorsprovider.h"
#include "ui/screens/addtorrent/addtorrentdialog.h"
#include "ui/screens/addtorrent/addtorrenthelpers.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"

namespace tremotesf {
    namespace {
        constexpr std::array torrentDoubleClickActionComboBoxValues{
            Settings::TorrentDoubleClickAction::OpenPropertiesDialog,
            Settings::TorrentDoubleClickAction::OpenTorrentFile,
            Settings::TorrentDoubleClickAction::OpenDownloadDirectory
        };

        Settings::TorrentDoubleClickAction torrentDoubleClickActionFromComboBoxIndex(int index) {
            if (index == -1) {
                return Settings::TorrentDoubleClickAction::OpenPropertiesDialog;
            }
            return torrentDoubleClickActionComboBoxValues.at(static_cast<size_t>(index));
        }
    }

    SettingsDialog::SettingsDialog(Rpc* rpc, QWidget* parent) : QDialog(parent) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Options"));

        auto rootLayout = new QVBoxLayout(this);
        auto scrollArea = new QScrollArea(this);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidgetResizable(true);
        rootLayout->addWidget(scrollArea);
        auto widget = new QWidget(this);
        auto layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        scrollArea->setWidget(widget);

        QComboBox* darkThemeComboBox{};
        std::vector<Settings::DarkThemeMode> darkThemeComboBoxValues{};
        QCheckBox* systemAccentColorCheckBox{};
        if constexpr (targetOs == TargetOs::Windows) {
            //: Options section
            auto appearanceGroupBox = new QGroupBox(qApp->translate("tremotesf", "Appearance"), this);
            auto appearanceGroupBoxLayout = new QFormLayout(appearanceGroupBox);
            appearanceGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
        auto addTorrentsGroupBoxLayout = new QVBoxLayout(addTorrentsGroupBox);

        auto rememberOpenTorrentDirCheckbox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Remember location of last opened torrent file"),
            this
        );
        addTorrentsGroupBoxLayout->addWidget(rememberOpenTorrentDirCheckbox);

        auto rememberAddTorrentParametersCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Remember parameters of last added torrent"),
            this
        );
        addTorrentsGroupBoxLayout->addWidget(rememberAddTorrentParametersCheckBox);

        auto addTorrentParametersDisconnectedMessage = new KMessageWidget(
            //: Server connection status
            qApp->translate("tremotesf", "Disconnected"),
            this
        );
        addTorrentParametersDisconnectedMessage->setMessageType(KMessageWidget::Warning);
        addTorrentParametersDisconnectedMessage->setCloseButtonVisible(false);
        addTorrentsGroupBoxLayout->addWidget(addTorrentParametersDisconnectedMessage);

        auto addTorrentParametersGroupBox = new QGroupBox(qApp->translate("tremotesf", "Add torrent parameters"), this);
        auto addTorrentParametersGroupBoxLayout = new QFormLayout(addTorrentParametersGroupBox);
        addTorrentParametersGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        const auto addTorrentParametersWidgets = AddTorrentDialog::createAddTorrentParametersWidgets(
            AddTorrentDialog::Mode::File,
            addTorrentParametersGroupBoxLayout,
            rpc
        );
        auto addTorrentParametersResetButton = new QPushButton(qApp->translate("tremotesf", "Reset"), this);
        addTorrentParametersGroupBoxLayout->addRow(addTorrentParametersResetButton);

        addTorrentsGroupBoxLayout->addWidget(addTorrentParametersGroupBox);

        QCheckBox* showMainWindowWhenAddingTorrentsCheckBox{};
        // Disabling this option does not work on macOS since the app is always activated when files are opened,
        // which causes us to show main window
        if constexpr (targetOs != TargetOs::UnixMacOS) {
            showMainWindowWhenAddingTorrentsCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Show main window when adding torrents"),
                this
            );
            addTorrentsGroupBoxLayout->addWidget(showMainWindowWhenAddingTorrentsCheckBox);
        }

        auto showDialogWhenAddingTorrentsCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Show dialog when adding torrents"),
            this
        );
        addTorrentsGroupBoxLayout->addWidget(showDialogWhenAddingTorrentsCheckBox);

        auto fillTorrentLinkFromKeyboardCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Automatically fill link from clipboard when adding torrent link"),
            this
        );
        addTorrentsGroupBoxLayout->addWidget(fillTorrentLinkFromKeyboardCheckBox);
        auto pasteTipLabel = new QLabel(
            //: %1 is a key binding, e.g. "Ctrl + C"
            qApp->translate("tremotesf", "Tip: you can also press %1 in main window to add torrents from clipboard")
                .arg(QKeySequence(QKeySequence::Paste).toString(QKeySequence::NativeText))
        );
        addTorrentsGroupBoxLayout->addWidget(pasteTipLabel);

        auto askForMergingTrackersCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Ask for merging trackers when adding existing torrent"),
            this
        );
        addTorrentsGroupBoxLayout->addWidget(askForMergingTrackersCheckBox);

        auto mergeTrackersCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Merge trackers when adding existing torrent"),
            this
        );
        addTorrentsGroupBoxLayout->addWidget(mergeTrackersCheckBox);
        QObject::connect(
            askForMergingTrackersCheckBox,
            &QCheckBox::toggled,
            mergeTrackersCheckBox,
            [mergeTrackersCheckBox](bool checked) { mergeTrackersCheckBox->setEnabled(!checked); }
        );

        layout->addWidget(addTorrentsGroupBox);

        //: Options section
        auto otherBehaviourGroupBox = new QGroupBox(qApp->translate("tremotesf", "Other behaviour"), this);
        layout->addWidget(otherBehaviourGroupBox);
        auto otherBehaviourGroupBoxLayout = new QFormLayout(otherBehaviourGroupBox);
        otherBehaviourGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto torrentDoubleClickActionComboBox = new QComboBox(this);
        for (const auto action : torrentDoubleClickActionComboBoxValues) {
            switch (action) {
            case Settings::TorrentDoubleClickAction::OpenPropertiesDialog:
                torrentDoubleClickActionComboBox->addItem(qApp->translate("tremotesf", "Open properties dialog"));
                break;
            case Settings::TorrentDoubleClickAction::OpenTorrentFile:
                torrentDoubleClickActionComboBox->addItem(qApp->translate("tremotesf", "Open torrent's file"));
                break;
            case Settings::TorrentDoubleClickAction::OpenDownloadDirectory:
                torrentDoubleClickActionComboBox->addItem(qApp->translate("tremotesf", "Open download directory"));
                break;
            default:
                break;
            }
        }
        otherBehaviourGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "What to do when torrent in the list is double clicked:"),
            torrentDoubleClickActionComboBox
        );

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
        rootLayout->addWidget(dialogButtonBox);

        auto settings = Settings::instance();
        connectOnStartupCheckBox->setChecked(settings->connectOnStartup());
        rememberOpenTorrentDirCheckbox->setChecked(settings->rememberOpenTorrentDir());
        rememberAddTorrentParametersCheckBox->setChecked(settings->rememberAddTorrentParameters());
        addTorrentParametersDisconnectedMessage->setVisible(!rpc->isConnected());
        addTorrentParametersGroupBox->setEnabled(rpc->isConnected());
        QObject::connect(rpc, &Rpc::connectedChanged, this, [=] {
            const bool connected = rpc->isConnected();
            if (connected) {
                addTorrentParametersDisconnectedMessage->animatedHide();
            } else {
                addTorrentParametersDisconnectedMessage->animatedShow();
            }
            addTorrentParametersGroupBox->setEnabled(connected);
            if (connected) {
                // Update parameters which initial values depend on server state
                const auto parameters = getAddTorrentParameters(rpc);
                addTorrentParametersWidgets.downloadDirectoryWidget->updatePath(parameters.downloadDirectory);
                addTorrentParametersWidgets.startTorrentCheckBox->setChecked(parameters.startAfterAdding);
            }
        });
        QObject::connect(addTorrentParametersResetButton, &QPushButton::clicked, this, [=] {
            addTorrentParametersWidgets.reset(rpc);
        });
        torrentDoubleClickActionComboBox->setCurrentIndex(
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            indexOfCasted<int>(torrentDoubleClickActionComboBoxValues, settings->torrentDoubleClickAction()).value()
        );
        if (showMainWindowWhenAddingTorrentsCheckBox) {
            showMainWindowWhenAddingTorrentsCheckBox->setChecked(settings->showMainWindowWhenAddingTorrent());
        }
        showDialogWhenAddingTorrentsCheckBox->setChecked(settings->showAddTorrentDialog());
        fillTorrentLinkFromKeyboardCheckBox->setChecked(settings->fillTorrentLinkFromClipboard());
        askForMergingTrackersCheckBox->setChecked(settings->askForMergingTrackersWhenAddingExistingTorrent());
        mergeTrackersCheckBox->setChecked(settings->mergeTrackersWhenAddingExistingTorrent());
        mergeTrackersCheckBox->setEnabled(!askForMergingTrackersCheckBox->isChecked());
        notificationOnDisconnectingCheckBox->setChecked(settings->notificationOnDisconnecting());
        notificationOnAddingTorrentCheckBox->setChecked(settings->notificationOnAddingTorrent());
        notificationOfFinishedTorrentsCheckBox->setChecked(settings->notificationOfFinishedTorrents());
        trayIconCheckBox->setChecked(settings->showTrayIcon());
        addedSinceLastConnectionCheckBox->setChecked(settings->notificationsOnAddedTorrentsSinceLastConnection());
        finishedSinceLastConnectionCheckBox->setChecked(settings->notificationsOnFinishedTorrentsSinceLastConnection());

        if constexpr (targetOs == TargetOs::Windows) {
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
            settings->setRememberOpenTorrentDir(rememberOpenTorrentDirCheckbox->isChecked());
            settings->setRememberTorrentAddParameters(rememberAddTorrentParametersCheckBox->isChecked());
            addTorrentParametersWidgets.saveToSettings();
            if (showMainWindowWhenAddingTorrentsCheckBox) {
                settings->setShowMainWindowWhenAddingTorrent(showMainWindowWhenAddingTorrentsCheckBox->isChecked());
            }
            settings->setShowAddTorrentDialog(showDialogWhenAddingTorrentsCheckBox->isChecked());
            settings->setFillTorrentLinkFromClipboard(fillTorrentLinkFromKeyboardCheckBox->isChecked());
            settings->setAskForMergingTrackersWhenAddingExistingTorrent(askForMergingTrackersCheckBox->isChecked());
            settings->setMergeTrackersWhenAddingExistingTorrent(mergeTrackersCheckBox->isChecked());
            settings->setTorrentDoubleClickAction(
                torrentDoubleClickActionFromComboBoxIndex(torrentDoubleClickActionComboBox->currentIndex())
            );
            settings->setNotificationOnDisconnecting(notificationOnDisconnectingCheckBox->isChecked());
            settings->setNotificationOnAddingTorrent(notificationOnAddingTorrentCheckBox->isChecked());
            settings->setNotificationOfFinishedTorrents(notificationOfFinishedTorrentsCheckBox->isChecked());
            settings->setShowTrayIcon(trayIconCheckBox->isChecked());
            settings->setNotificationsOnAddedTorrentsSinceLastConnection(addedSinceLastConnectionCheckBox->isChecked());
            settings->setNotificationsOnFinishedTorrentsSinceLastConnection(
                finishedSinceLastConnectionCheckBox->isChecked()
            );
            if constexpr (targetOs == TargetOs::Windows) {
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
