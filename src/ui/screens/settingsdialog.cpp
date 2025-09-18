// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsdialog.h"

#include <array>
#include <concepts>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

#include <KMessageWidget>
#include <KPageWidget>

#include "stdutils.h"
#include "target_os.h"
#include "settings.h"
#include "rpc/rpc.h"
#include "ui/screens/addtorrent/addtorrentdialog.h"
#include "ui/screens/addtorrent/addtorrenthelpers.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"

#ifdef Q_OS_WIN
#    include "ui/systemcolorsprovider.h"
#endif

namespace tremotesf {
    namespace {
#ifdef Q_OS_WIN
        constexpr std::array darkThemeComboBoxValues{
            Settings::DarkThemeMode::FollowSystem, Settings::DarkThemeMode::On, Settings::DarkThemeMode::Off
        };
#endif

        constexpr std::array torrentDoubleClickActionComboBoxValues{
            Settings::TorrentDoubleClickAction::OpenPropertiesDialog,
            Settings::TorrentDoubleClickAction::OpenTorrentFile,
            Settings::TorrentDoubleClickAction::OpenDownloadDirectory
        };

        std::invocable auto createGeneralPage(KPageWidget* pageWidget, Settings* settings) {
            auto page = new QWidget();
            //: Options tab
            pageWidget->addPage(page, qApp->translate("tremotesf", "General"))
                ->setIcon(QIcon::fromTheme("preferences-desktop"));
            auto layout = new QFormLayout(page);
            layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

#ifdef Q_OS_WIN
            auto darkThemeComboBox = new QComboBox(pageWidget);

            //: Dark theme mode
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "Follow system"));
            //: Dark theme mode
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "On"));
            //: Dark theme mode
            darkThemeComboBox->addItem(qApp->translate("tremotesf", "Off"));
            layout->addRow(qApp->translate("tremotesf", "Dark theme"), darkThemeComboBox);

            auto systemAccentColorCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Use system accent color"),
                page
            );
            layout->addRow(systemAccentColorCheckBox);

            darkThemeComboBox->setCurrentIndex(
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                indexOfCasted<int>(darkThemeComboBoxValues, settings->get_darkThemeMode()).value()
            );
            systemAccentColorCheckBox->setChecked(settings->get_useSystemAccentColor());
#endif

            auto showTorrentPropertiesInMainWindowCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Show torrent properties in a panel in the main window"),
                page
            );
            layout->addRow(showTorrentPropertiesInMainWindowCheckBox);
            showTorrentPropertiesInMainWindowCheckBox->setChecked(settings->get_showTorrentPropertiesInMainWindow());

            auto torrentDoubleClickActionComboBox = new QComboBox(page);
            layout->addRow(
                qApp->translate("tremotesf", "What to do when torrent in the list is double clicked:"),
                torrentDoubleClickActionComboBox
            );
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

            torrentDoubleClickActionComboBox->setCurrentIndex(
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                indexOfCasted<int>(torrentDoubleClickActionComboBoxValues, settings->get_torrentDoubleClickAction())
                    .value()
            );

            auto dialogWarningMessage = new KMessageWidget(page);
            layout->addRow(dialogWarningMessage);
            dialogWarningMessage->setMessageType(KMessageWidget::Warning);
            dialogWarningMessage->setCloseButtonVisible(false);
            dialogWarningMessage->setText(qApp->translate(
                "tremotesf",
                "Properties dialog won't be shown because torrent properties are shown in the main window"
            ));
            const auto updateDialogWarningMessage = [=] {
                if (showTorrentPropertiesInMainWindowCheckBox->isChecked()
                    && torrentDoubleClickActionComboBox->currentIndex()
                           == indexOfCasted<int>(
                               torrentDoubleClickActionComboBoxValues,
                               Settings::TorrentDoubleClickAction::OpenPropertiesDialog
                           )) {
                    dialogWarningMessage->animatedShow();
                } else {
                    dialogWarningMessage->animatedHide();
                }
            };
            updateDialogWarningMessage();
            QObject::connect(
                showTorrentPropertiesInMainWindowCheckBox,
                &QCheckBox::toggled,
                dialogWarningMessage,
                updateDialogWarningMessage
            );
            QObject::connect(
                torrentDoubleClickActionComboBox,
                &QComboBox::currentIndexChanged,
                dialogWarningMessage,
                updateDialogWarningMessage
            );

            auto connectOnStartupCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Connect to server on startup"),
                page
            );
            layout->addRow(connectOnStartupCheckBox);
            connectOnStartupCheckBox->setChecked(settings->get_connectOnStartup());

            auto displayRelativeTimeCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Display relative time"),
                page
            );
            layout->addRow(displayRelativeTimeCheckBox);
            displayRelativeTimeCheckBox->setChecked(settings->get_displayRelativeTime());

            auto displayFullDownloadDirectoryPathCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Display full path of download directories in sidebar and torrents list"),
                page
            );
            layout->addRow(displayFullDownloadDirectoryPathCheckBox);
            displayFullDownloadDirectoryPathCheckBox->setChecked(settings->get_displayFullDownloadDirectoryPath());

            return [=] {
#ifdef Q_OS_WIN
                if (const auto index = darkThemeComboBox->currentIndex(); index != -1) {
                    settings->set_darkThemeMode(darkThemeComboBoxValues.at(static_cast<size_t>(index)));
                }
                if (systemAccentColorCheckBox) {
                    settings->set_useSystemAccentColor(systemAccentColorCheckBox->isChecked());
                }
#endif
                settings->set_showTorrentPropertiesInMainWindow(showTorrentPropertiesInMainWindowCheckBox->isChecked());
                if (const auto index = torrentDoubleClickActionComboBox->currentIndex(); index != -1) {
                    settings->set_torrentDoubleClickAction(
                        torrentDoubleClickActionComboBoxValues.at(static_cast<size_t>(index))
                    );
                }
                settings->set_connectOnStartup(connectOnStartupCheckBox->isChecked());
                settings->set_displayRelativeTime(displayRelativeTimeCheckBox->isChecked());
                settings->set_displayFullDownloadDirectoryPath(displayFullDownloadDirectoryPathCheckBox->isChecked());
            };
        }

        std::invocable auto createAddingTorrentsPage(KPageWidget* pageWidget, Settings* settings, Rpc* rpc) {
            auto page = new QWidget();
            //: Options tab
            pageWidget->addPage(page, qApp->translate("tremotesf", "Adding torrents"))
                ->setIcon(QIcon::fromTheme("folder-download"));
            auto layout = new QVBoxLayout(page);
            layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

            auto rememberOpenTorrentDirCheckbox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Remember location of last opened torrent file"),
                page
            );
            layout->addWidget(rememberOpenTorrentDirCheckbox);

            auto rememberAddTorrentParametersCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Remember parameters of last added torrent"),
                page
            );
            layout->addWidget(rememberAddTorrentParametersCheckBox);

            auto addTorrentParametersDisconnectedMessage = new KMessageWidget(
                //: Server connection status
                qApp->translate("tremotesf", "Disconnected"),
                page
            );
            addTorrentParametersDisconnectedMessage->setMessageType(KMessageWidget::Warning);
            addTorrentParametersDisconnectedMessage->setCloseButtonVisible(false);
            layout->addWidget(addTorrentParametersDisconnectedMessage);

            auto addTorrentParametersGroupBox =
                new QGroupBox(qApp->translate("tremotesf", "Add torrent parameters"), page);
            auto addTorrentParametersGroupBoxLayout = new QFormLayout(addTorrentParametersGroupBox);
            addTorrentParametersGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
            const auto addTorrentParametersWidgets =
                AddTorrentDialog::createAddTorrentParametersWidgets(true, addTorrentParametersGroupBoxLayout, rpc);
            auto addTorrentParametersResetButton = new QPushButton(qApp->translate("tremotesf", "Reset"), page);
            addTorrentParametersGroupBoxLayout->addRow(addTorrentParametersResetButton);

            layout->addWidget(addTorrentParametersGroupBox);

            QCheckBox* showMainWindowWhenAddingTorrentsCheckBox{};
            // Disabling this option does not work on macOS since the app is always activated when files are opened,
            // which causes us to show main window
            if constexpr (targetOs != TargetOs::UnixMacOS) {
                showMainWindowWhenAddingTorrentsCheckBox = new QCheckBox(
                    //: Check box label
                    qApp->translate("tremotesf", "Show main window when adding torrents"),
                    page
                );
                layout->addWidget(showMainWindowWhenAddingTorrentsCheckBox);
            }

            auto showDialogWhenAddingTorrentsCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Show dialog when adding torrents"),
                page
            );
            layout->addWidget(showDialogWhenAddingTorrentsCheckBox);

            auto fillTorrentLinkFromKeyboardCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Automatically fill link from clipboard when adding torrent link"),
                page
            );
            layout->addWidget(fillTorrentLinkFromKeyboardCheckBox);
            auto pasteTipLabel = new QLabel(
                //: %1 is a key binding, e.g. "Ctrl + C"
                qApp->translate("tremotesf", "Tip: you can also press %1 in main window to add torrents from clipboard")
                    .arg(QKeySequence(QKeySequence::Paste).toString(QKeySequence::NativeText))
            );
            layout->addWidget(pasteTipLabel);

            auto askForMergingTrackersCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Ask for merging trackers when adding existing torrent"),
                page
            );
            layout->addWidget(askForMergingTrackersCheckBox);

            auto mergeTrackersCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Merge trackers when adding existing torrent"),
                page
            );
            layout->addWidget(mergeTrackersCheckBox);
            QObject::connect(
                askForMergingTrackersCheckBox,
                &QCheckBox::toggled,
                mergeTrackersCheckBox,
                [mergeTrackersCheckBox](bool checked) { mergeTrackersCheckBox->setEnabled(!checked); }
            );

            layout->addStretch();

            rememberOpenTorrentDirCheckbox->setChecked(settings->get_rememberOpenTorrentDir());
            rememberAddTorrentParametersCheckBox->setChecked(settings->get_rememberAddTorrentParameters());
            addTorrentParametersDisconnectedMessage->setVisible(!rpc->isConnected());
            addTorrentParametersGroupBox->setEnabled(rpc->isConnected());
            QObject::connect(rpc, &Rpc::connectedChanged, page, [=] {
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
            QObject::connect(addTorrentParametersResetButton, &QPushButton::clicked, page, [=] {
                addTorrentParametersWidgets.reset(rpc);
            });

            if (showMainWindowWhenAddingTorrentsCheckBox) {
                showMainWindowWhenAddingTorrentsCheckBox->setChecked(settings->get_showMainWindowWhenAddingTorrent());
            }
            showDialogWhenAddingTorrentsCheckBox->setChecked(settings->get_showAddTorrentDialog());
            fillTorrentLinkFromKeyboardCheckBox->setChecked(settings->get_fillTorrentLinkFromClipboard());
            askForMergingTrackersCheckBox->setChecked(settings->get_askForMergingTrackersWhenAddingExistingTorrent());
            mergeTrackersCheckBox->setChecked(settings->get_mergeTrackersWhenAddingExistingTorrent());
            mergeTrackersCheckBox->setEnabled(!askForMergingTrackersCheckBox->isChecked());

            return [=] {
                settings->set_rememberOpenTorrentDir(rememberOpenTorrentDirCheckbox->isChecked());
                settings->set_rememberAddTorrentParameters(rememberAddTorrentParametersCheckBox->isChecked());
                addTorrentParametersWidgets.saveToSettings();
                if (showMainWindowWhenAddingTorrentsCheckBox) {
                    settings->set_showMainWindowWhenAddingTorrent(showMainWindowWhenAddingTorrentsCheckBox->isChecked()
                    );
                }
                settings->set_showAddTorrentDialog(showDialogWhenAddingTorrentsCheckBox->isChecked());
                settings->set_fillTorrentLinkFromClipboard(fillTorrentLinkFromKeyboardCheckBox->isChecked());
                settings->set_askForMergingTrackersWhenAddingExistingTorrent(askForMergingTrackersCheckBox->isChecked()
                );
                settings->set_mergeTrackersWhenAddingExistingTorrent(mergeTrackersCheckBox->isChecked());
            };
        }

        std::invocable auto createNotificationsPage(KPageWidget* pageWidget, Settings* settings) {
            auto page = new QWidget();
            //: Options tab
            pageWidget->addPage(page, qApp->translate("tremotesf", "Notifications"))
                ->setIcon(QIcon::fromTheme("preferences-desktop-notification"));
            auto layout = new QVBoxLayout(page);
            layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

            auto notificationOnDisconnectingCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Notify when disconnecting from server"),
                page
            );
            layout->addWidget(notificationOnDisconnectingCheckBox);

            auto notificationOnAddingTorrentCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Notify on added torrents"),
                page
            );
            layout->addWidget(notificationOnAddingTorrentCheckBox);

            auto notificationOfFinishedTorrentsCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Notify on finished torrents"),
                page
            );
            layout->addWidget(notificationOfFinishedTorrentsCheckBox);

            auto trayIconCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Show icon in the notification area"),
                page
            );
            layout->addWidget(trayIconCheckBox);

            //: Notifications options section
            auto whenConnectingGroupBox =
                new QGroupBox(qApp->translate("tremotesf", "When connecting to server"), page);
            layout->addWidget(whenConnectingGroupBox);
            auto whenConnectingGroupBoxLayout = new QVBoxLayout(whenConnectingGroupBox);
            whenConnectingGroupBoxLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

            auto addedSinceLastConnectionCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Notify on added torrents since last connection to server"),
                whenConnectingGroupBox
            );
            whenConnectingGroupBoxLayout->addWidget(addedSinceLastConnectionCheckBox);

            auto finishedSinceLastConnectionCheckBox = new QCheckBox(
                //: Check box label
                qApp->translate("tremotesf", "Notify on finished torrents since last connection to server"),
                whenConnectingGroupBox
            );
            whenConnectingGroupBoxLayout->addWidget(finishedSinceLastConnectionCheckBox);

            notificationOnDisconnectingCheckBox->setChecked(settings->get_notificationOnDisconnecting());
            notificationOnAddingTorrentCheckBox->setChecked(settings->get_notificationOnAddingTorrent());
            notificationOfFinishedTorrentsCheckBox->setChecked(settings->get_notificationOfFinishedTorrents());
            trayIconCheckBox->setChecked(settings->get_showTrayIcon());
            addedSinceLastConnectionCheckBox->setChecked(settings->get_notificationsOnAddedTorrentsSinceLastConnection()
            );
            finishedSinceLastConnectionCheckBox->setChecked(
                settings->get_notificationsOnFinishedTorrentsSinceLastConnection()
            );

            return [=] {
                settings->set_notificationOnDisconnecting(notificationOnDisconnectingCheckBox->isChecked());
                settings->set_notificationOnAddingTorrent(notificationOnAddingTorrentCheckBox->isChecked());
                settings->set_notificationOfFinishedTorrents(notificationOfFinishedTorrentsCheckBox->isChecked());
                settings->set_showTrayIcon(trayIconCheckBox->isChecked());
                settings->set_notificationsOnAddedTorrentsSinceLastConnection(
                    addedSinceLastConnectionCheckBox->isChecked()
                );
                settings->set_notificationsOnFinishedTorrentsSinceLastConnection(
                    finishedSinceLastConnectionCheckBox->isChecked()
                );
            };
        }
    }

    SettingsDialog::SettingsDialog(Rpc* rpc, QWidget* parent) : QDialog(parent) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Options"));

        auto rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);

        auto pageWidget = new KPageWidget(this);
        rootLayout->addWidget(pageWidget);

        auto settings = Settings::instance();

        const auto saveGeneralPage = createGeneralPage(pageWidget, settings);
        const auto saveAddingTorrentsPage = createAddingTorrentsPage(pageWidget, settings, rpc);
        const auto saveNotificationsPage = createNotificationsPage(pageWidget, settings);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
        pageWidget->setPageFooter(dialogButtonBox);

        QObject::connect(this, &SettingsDialog::accepted, this, [=] {
            saveGeneralPage();
            saveAddingTorrentsPage();
            saveNotificationsPage();
        });
    }
}
