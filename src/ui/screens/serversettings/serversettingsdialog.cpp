// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serversettingsdialog.h"

#include <array>
#include <limits>

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <KColumnResizer>
#include <KMessageWidget>
#include <KPageModel>
#include <KPageWidget>

#include "rpc/serversettings.h"
#include "stdutils.h"
#include "rpc/rpc.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"

namespace tremotesf {
    namespace {
        constexpr std::array encryptionModeComboBoxItems{
            ServerSettingsData::EncryptionMode::Allowed,
            ServerSettingsData::EncryptionMode::Preferred,
            ServerSettingsData::EncryptionMode::Required
        };

        ServerSettingsData::EncryptionMode encryptionModeFromComboBoxItem(int index) {
            if (index == -1) {
                return {};
            }
            return encryptionModeComboBoxItems.at(static_cast<size_t>(index));
        }
    }

    ServerSettingsDialog::ServerSettingsDialog(const Rpc* rpc, QWidget* parent) : QDialog(parent), mRpc(rpc) {
        //: Dialog title
        setWindowTitle(qApp->translate("tremotesf", "Server Options"));

        setupUi();

        const auto onConnectedChanged = [this] {
            if (mRpc->isConnected()) {
                mDisconnectedMessageWidget->animatedHide();
                mDisconnectedMessageWidget->setEnabled(true);
                mDownloadingPageWidget->setEnabled(true);
                mSeedingPageWidget->setEnabled(true);
                mQueuePageWidget->setEnabled(true);
                mSpeedPageWidget->setEnabled(true);
                mNetworkPageWidget->setEnabled(true);
                loadSettings();
            } else {
                mDisconnectedMessageWidget->hide();
                mDisconnectedMessageWidget->animatedShow();
                mDownloadingPageWidget->setEnabled(false);
                mSeedingPageWidget->setEnabled(false);
                mQueuePageWidget->setEnabled(false);
                mSpeedPageWidget->setEnabled(false);
                mNetworkPageWidget->setEnabled(false);
            }
        };
        QObject::connect(mRpc, &Rpc::connectedChanged, this, onConnectedChanged);
        onConnectedChanged();

        loadSettings();
    }

    QSize ServerSettingsDialog::sizeHint() const { return minimumSizeHint().expandedTo(QSize(700, 550)); }

    void ServerSettingsDialog::accept() {
        ServerSettings* settings = mRpc->serverSettings();

        settings->setSaveOnSet(false);

        settings->setDownloadDirectory(mDownloadDirectoryWidget->path());
        settings->setStartAddedTorrents(mStartAddedTorrentsCheckBox->isChecked());
        //settings->setTrashTorrentFiles(mTrashTorrentFilesCheckBox->isChecked());
        settings->setRenameIncompleteFiles(mIncompleteFilesCheckBox->isChecked());
        settings->setIncompleteDirectoryEnabled(mIncompleteDirectoryCheckBox->isChecked());
        settings->setIncompleteDirectory(mIncompleteDirectoryWidget->path());

        settings->setRatioLimited(mRatioLimitCheckBox->isChecked());
        settings->setRatioLimit(mRatioLimitSpinBox->value());
        settings->setIdleSeedingLimited(mIdleSeedingLimitCheckBox->isChecked());
        settings->setIdleSeedingLimit(mIdleSeedingLimitSpinBox->value());

        settings->setDownloadQueueEnabled(mMaximumActiveDownloadsCheckBox->isChecked());
        settings->setDownloadQueueSize(mMaximumActiveDownloadsSpinBox->value());
        settings->setSeedQueueEnabled(mMaximumActiveUploadsCheckBox->isChecked());
        settings->setSeedQueueSize(mMaximumActiveUploadsSpinBox->value());
        settings->setIdleQueueLimited(mIdleQueueLimitCheckBox->isChecked());
        settings->setIdleQueueLimit(mIdleQueueLimitSpinBox->value());

        settings->setDownloadSpeedLimited(mDownloadSpeedLimitCheckBox->isChecked());
        settings->setDownloadSpeedLimit(mDownloadSpeedLimitSpinBox->value());
        settings->setUploadSpeedLimited(mUploadSpeedLimitCheckBox->isChecked());
        settings->setUploadSpeedLimit(mUploadSpeedLimitSpinBox->value());
        settings->setAlternativeSpeedLimitsEnabled(mEnableAlternativeSpeedLimitsGroupBox->isChecked());
        settings->setAlternativeDownloadSpeedLimit(mAlternativeDownloadSpeedLimitSpinBox->value());
        settings->setAlternativeUploadSpeedLimit(mAlternativeUploadSpeedLimitSpinBox->value());
        settings->setAlternativeSpeedLimitsScheduled(mLimitScheduleGroupBox->isChecked());
        settings->setAlternativeSpeedLimitsBeginTime(mLimitScheduleBeginTimeEdit->time());
        settings->setAlternativeSpeedLimitsEndTime(mLimitScheduleEndTimeEdit->time());

        settings->setAlternativeSpeedLimitsDays(static_cast<ServerSettingsData::AlternativeSpeedLimitsDays>(
            mLimitScheduleDaysComboBox->currentData().toInt()
        ));

        settings->setPeerPort(mPeerPortSpinBox->value());
        settings->setRandomPortEnabled(mRandomPortCheckBox->isChecked());
        settings->setPortForwardingEnabled(mPortForwardingCheckBox->isChecked());
        settings->setEncryptionMode(encryptionModeFromComboBoxItem(mEncryptionComboBox->currentIndex()));
        settings->setUtpEnabled(mUtpCheckBox->isChecked());
        settings->setPexEnabled(mPexCheckBox->isChecked());
        settings->setDhtEnabled(mDhtCheckBox->isChecked());
        settings->setLpdEnabled(mLpdCheckBox->isChecked());
        settings->setMaximumPeersPerTorrent(mTorrentPeerLimitSpinBox->value());
        settings->setMaximumPeersGlobally(mGlobalPeerLimitSpinBox->value());

        settings->save();
        settings->setSaveOnSet(true);

        QDialog::accept();
    }

    void ServerSettingsDialog::setupUi() {
        //
        // Creating layout
        //

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        //: Message that appears when disconnected from server
        mDisconnectedMessageWidget = new KMessageWidget(qApp->translate("tremotesf", "Disconnected"), this);
        mDisconnectedMessageWidget->setCloseButtonVisible(false);
        mDisconnectedMessageWidget->setMessageType(KMessageWidget::Warning);
        mDisconnectedMessageWidget->setVisible(false);
        layout->addWidget(mDisconnectedMessageWidget);

        auto pageWidget = new KPageWidget(this);

        // Downloading page
        mDownloadingPageWidget = new QWidget(this);

        KPageWidgetItem* downloadingPageItem = pageWidget->addPage(
            mDownloadingPageWidget,
            //: "Downloading" server setting page
            qApp->translate("tremotesf", "Downloading", "Noun")
        );
        downloadingPageItem->setIcon(QIcon::fromTheme("folder-download"_l1));

        auto downloadingPageLayout = new QFormLayout(mDownloadingPageWidget);
        downloadingPageLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mDownloadDirectoryWidget = new RemoteDirectorySelectionWidget(this);
        mDownloadDirectoryWidget->setup({}, mRpc);
        downloadingPageLayout->addRow(qApp->translate("tremotesf", "Download directory:"), mDownloadDirectoryWidget);

        //: Check box label
        mStartAddedTorrentsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Start added torrents"), this);
        downloadingPageLayout->addRow(mStartAddedTorrentsCheckBox);

        /*mTrashTorrentFilesCheckBox = new QCheckBox(qApp->translate("tremotesf", "Trash .torrent files"), this);
        downloadingPageLayout->addRow(mTrashTorrentFilesCheckBox);*/

        mIncompleteFilesCheckBox = new QCheckBox(
            //: Check box label
            qApp->translate("tremotesf", "Append \".part\" to names of incomplete files"),
            this
        );
        downloadingPageLayout->addRow(mIncompleteFilesCheckBox);

        mIncompleteDirectoryCheckBox =
            new QCheckBox(qApp->translate("tremotesf", "Directory for incomplete files:"), this);
        downloadingPageLayout->addRow(mIncompleteDirectoryCheckBox);

        auto incompleteDirectoryWidgetLayout = new QHBoxLayout();
        downloadingPageLayout->addRow(incompleteDirectoryWidgetLayout);
        mIncompleteDirectoryWidget = new RemoteDirectorySelectionWidget(this);
        mIncompleteDirectoryWidget->setup({}, mRpc);
        mIncompleteDirectoryWidget->setEnabled(false);
        QObject::connect(
            mIncompleteDirectoryCheckBox,
            &QCheckBox::toggled,
            mIncompleteDirectoryWidget,
            &RemoteDirectorySelectionWidget::setEnabled
        );
        //downloadingPageLayout->addRow(mIncompleteDirectoryCheckBox, mIncompleteDirectoryWidget);
        incompleteDirectoryWidgetLayout->addSpacing(28);
        incompleteDirectoryWidgetLayout->addWidget(mIncompleteDirectoryWidget);

        // Seeding page
        mSeedingPageWidget = new QWidget(this);
        KPageWidgetItem* seedingPageItem = pageWidget->addPage(
            mSeedingPageWidget,
            //: "Seeding" server setting page
            qApp->translate("tremotesf", "Seeding", "Noun")
        );
        seedingPageItem->setIcon(QIcon::fromTheme("network-server"_l1));

        auto seedingPageLayout = new QGridLayout(mSeedingPageWidget);

        mRatioLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Stop seeding at ratio:"), this);
        seedingPageLayout->addWidget(mRatioLimitCheckBox, 0, 0, 1, 2, Qt::AlignTop);

        mRatioLimitSpinBox = new QDoubleSpinBox(this);
        mRatioLimitSpinBox->setEnabled(false);
        mRatioLimitSpinBox->setMaximum(std::numeric_limits<int>::max());
        QObject::connect(mRatioLimitCheckBox, &QCheckBox::toggled, mRatioLimitSpinBox, &QSpinBox::setEnabled);
        seedingPageLayout->addWidget(mRatioLimitSpinBox, 1, 1, Qt::AlignTop);

        mIdleSeedingLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Stop seeding if idle for:"), this);
        seedingPageLayout->addWidget(mIdleSeedingLimitCheckBox, 2, 0, 1, 2, Qt::AlignTop);

        mIdleSeedingLimitSpinBox = new QSpinBox(this);
        mIdleSeedingLimitSpinBox->setEnabled(false);
        mIdleSeedingLimitSpinBox->setMaximum(9999);
        //: Suffix that is added to input field with number of minuts, e.g. "5 min"
        mIdleSeedingLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));
        QObject::connect(
            mIdleSeedingLimitCheckBox,
            &QCheckBox::toggled,
            mIdleSeedingLimitSpinBox,
            &QSpinBox::setEnabled
        );
        seedingPageLayout->addWidget(mIdleSeedingLimitSpinBox, 3, 1, Qt::AlignTop);

        seedingPageLayout->setRowStretch(3, 1);
        seedingPageLayout->setColumnStretch(1, 1);
        seedingPageLayout->setColumnMinimumWidth(0, 28 - seedingPageLayout->spacing());

        // Queue page
        mQueuePageWidget = new QWidget(this);
        KPageWidgetItem* queuePageItem = pageWidget->addPage(
            mQueuePageWidget,
            //: "Queue" server settings page
            qApp->translate("tremotesf", "Queue")
        );
        queuePageItem->setIcon(QIcon::fromTheme("applications-utilities"_l1));

        auto queuePageLayout = new QGridLayout(mQueuePageWidget);

        mMaximumActiveDownloadsCheckBox =
            new QCheckBox(qApp->translate("tremotesf", "Maximum active downloads:"), this);
        queuePageLayout->addWidget(mMaximumActiveDownloadsCheckBox, 0, 0, 1, 2, Qt::AlignTop);

        mMaximumActiveDownloadsSpinBox = new QSpinBox(this);
        mMaximumActiveDownloadsSpinBox->setEnabled(false);
        mMaximumActiveDownloadsSpinBox->setMaximum(std::numeric_limits<int>::max());
        QObject::connect(
            mMaximumActiveDownloadsCheckBox,
            &QCheckBox::toggled,
            mMaximumActiveDownloadsSpinBox,
            &QSpinBox::setEnabled
        );
        queuePageLayout->addWidget(mMaximumActiveDownloadsSpinBox, 1, 1, Qt::AlignTop);

        mMaximumActiveUploadsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Maximum active uploads:"), this);
        queuePageLayout->addWidget(mMaximumActiveUploadsCheckBox, 2, 0, 1, 2, Qt::AlignTop);

        mMaximumActiveUploadsSpinBox = new QSpinBox(this);
        mMaximumActiveUploadsSpinBox->setEnabled(false);
        mMaximumActiveUploadsSpinBox->setMaximum(std::numeric_limits<int>::max());
        QObject::connect(
            mMaximumActiveUploadsCheckBox,
            &QCheckBox::toggled,
            mMaximumActiveUploadsSpinBox,
            &QSpinBox::setEnabled
        );
        queuePageLayout->addWidget(mMaximumActiveUploadsSpinBox, 3, 1, Qt::AlignTop);

        mIdleQueueLimitCheckBox =
            new QCheckBox(qApp->translate("tremotesf", "Ignore queue position if idle for:"), this);
        queuePageLayout->addWidget(mIdleQueueLimitCheckBox, 4, 0, 1, 2, Qt::AlignTop);

        mIdleQueueLimitSpinBox = new QSpinBox(this);
        mIdleQueueLimitSpinBox->setEnabled(false);
        mIdleQueueLimitSpinBox->setMaximum(9999);
        //: Suffix that is added to input field with number of minuts, e.g. "5 min"
        mIdleQueueLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));
        QObject::connect(mIdleQueueLimitCheckBox, &QCheckBox::toggled, mIdleQueueLimitSpinBox, &QSpinBox::setEnabled);
        queuePageLayout->addWidget(mIdleQueueLimitSpinBox, 5, 1, Qt::AlignTop);

        queuePageLayout->setRowStretch(5, 1);
        queuePageLayout->setColumnStretch(1, 1);
        queuePageLayout->setColumnMinimumWidth(0, 28 - queuePageLayout->spacing());

        // Speed page
        mSpeedPageWidget = new QWidget(this);
        KPageWidgetItem* speedPageItem = pageWidget->addPage(
            mSpeedPageWidget,
            //: "Speed" server settings page
            qApp->translate("tremotesf", "Speed")
        );
        speedPageItem->setIcon(QIcon::fromTheme("preferences-system-time"_l1));

        auto speedPageLayout = new QVBoxLayout(mSpeedPageWidget);

        //: Speed limits section
        auto speedLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Limits"), this);
        auto speedLimitsGroupBoxLayout = new QGridLayout(speedLimitsGroupBox);

        const int maxSpeedLimit = static_cast<int>(std::numeric_limits<uint>::max() / 1024);
        //: Suffix that is added to input field with download/upload speed limit, e.g. "5000 kB/s". 'k' prefix means SI prefix, i.e kB = 1000 bytes
        const QString suffix(qApp->translate("tremotesf", " kB/s"));

        //: Download speed limit input field label
        mDownloadSpeedLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Download:"), this);
        speedLimitsGroupBoxLayout->addWidget(mDownloadSpeedLimitCheckBox, 0, 0, 1, 2);

        mDownloadSpeedLimitSpinBox = new QSpinBox(this);
        mDownloadSpeedLimitSpinBox->setEnabled(false);
        mDownloadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mDownloadSpeedLimitSpinBox->setSuffix(suffix);
        QObject::connect(
            mDownloadSpeedLimitCheckBox,
            &QCheckBox::toggled,
            mDownloadSpeedLimitSpinBox,
            &QSpinBox::setEnabled
        );
        speedLimitsGroupBoxLayout->addWidget(mDownloadSpeedLimitSpinBox, 1, 1);

        //: Upload speed limit input field label
        mUploadSpeedLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Upload:"), this);
        speedLimitsGroupBoxLayout->addWidget(mUploadSpeedLimitCheckBox, 2, 0, 1, 2);

        mUploadSpeedLimitSpinBox = new QSpinBox(this);
        mUploadSpeedLimitSpinBox->setEnabled(false);
        mUploadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mUploadSpeedLimitSpinBox->setSuffix(suffix);
        QObject::connect(
            mUploadSpeedLimitCheckBox,
            &QCheckBox::toggled,
            mUploadSpeedLimitSpinBox,
            &QSpinBox::setEnabled
        );
        speedLimitsGroupBoxLayout->addWidget(mUploadSpeedLimitSpinBox, 3, 1);

        speedLimitsGroupBoxLayout->setColumnMinimumWidth(0, 28 - speedLimitsGroupBoxLayout->spacing());

        speedPageLayout->addWidget(speedLimitsGroupBox);

        //: Alternative speed limits section
        auto alternativeSpeedLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Alternative Limits"), this);
        auto alternativeSpeedLimitsGroupBoxLayout = new QVBoxLayout(alternativeSpeedLimitsGroupBox);

        //: Check box label
        mEnableAlternativeSpeedLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Enable"), this);
        mEnableAlternativeSpeedLimitsGroupBox->setCheckable(true);
        auto enableAlternativeLimitsGroupBoxLayout = new QFormLayout(mEnableAlternativeSpeedLimitsGroupBox);
        enableAlternativeLimitsGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mAlternativeDownloadSpeedLimitSpinBox = new QSpinBox(this);
        mAlternativeDownloadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mAlternativeDownloadSpeedLimitSpinBox->setSuffix(suffix);
        enableAlternativeLimitsGroupBoxLayout->addRow(
            //: Download speed limit input field label
            qApp->translate("tremotesf", "Download:"),
            mAlternativeDownloadSpeedLimitSpinBox
        );
        mAlternativeUploadSpeedLimitSpinBox = new QSpinBox(this);
        mAlternativeUploadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mAlternativeUploadSpeedLimitSpinBox->setSuffix(suffix);
        enableAlternativeLimitsGroupBoxLayout->addRow(
            //: Upload speed limit input field label
            qApp->translate("tremotesf", "Upload:"),
            mAlternativeUploadSpeedLimitSpinBox
        );

        alternativeSpeedLimitsGroupBoxLayout->addWidget(mEnableAlternativeSpeedLimitsGroupBox);

        //: Title of alternative speed limit scheduling section
        mLimitScheduleGroupBox = new QGroupBox(qApp->translate("tremotesf", "Scheduled"), this);
        mLimitScheduleGroupBox->setCheckable(true);
        auto scheduleGroupBoxLayout = new QFormLayout(mLimitScheduleGroupBox);
        scheduleGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto scheduleTimeLayout = new QHBoxLayout();
        scheduleGroupBoxLayout->addRow(scheduleTimeLayout);
        mLimitScheduleBeginTimeEdit = new QTimeEdit(this);
        scheduleTimeLayout->addWidget(mLimitScheduleBeginTimeEdit, 1);
        //: Separates time range input fields. E.g. "to" inside "1:00 AM to 5:00 AM"
        scheduleTimeLayout->addWidget(new QLabel(qApp->translate("tremotesf", "to")));
        mLimitScheduleEndTimeEdit = new QTimeEdit(this);
        scheduleTimeLayout->addWidget(mLimitScheduleEndTimeEdit, 1);

        mLimitScheduleDaysComboBox = new QComboBox(this);
        mLimitScheduleDaysComboBox->addItem(
            qApp->translate("tremotesf", "Every day"),
            QVariant::fromValue(ServerSettingsData::AlternativeSpeedLimitsDays::All)
        );
        mLimitScheduleDaysComboBox->addItem(
            qApp->translate("tremotesf", "Weekdays"),
            QVariant::fromValue(ServerSettingsData::AlternativeSpeedLimitsDays::Weekdays)
        );
        mLimitScheduleDaysComboBox->addItem(
            qApp->translate("tremotesf", "Weekends"),
            QVariant::fromValue(ServerSettingsData::AlternativeSpeedLimitsDays::Weekends)
        );
        mLimitScheduleDaysComboBox->insertSeparator(mLimitScheduleDaysComboBox->count());
        {
            auto nextDay = [](Qt::DayOfWeek day) {
                if (day == Qt::Sunday) {
                    return Qt::Monday;
                }
                return static_cast<Qt::DayOfWeek>(day + 1);
            };

            auto daysFromQtDay = [](Qt::DayOfWeek day) {
                switch (day) {
                case Qt::Monday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Monday;
                case Qt::Tuesday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Tuesday;
                case Qt::Wednesday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Wednesday;
                case Qt::Thursday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Thursday;
                case Qt::Friday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Friday;
                case Qt::Saturday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Saturday;
                case Qt::Sunday:
                    return ServerSettingsData::AlternativeSpeedLimitsDays::Sunday;
                }
                return ServerSettingsData::AlternativeSpeedLimitsDays::All;
            };

            const QLocale locale;

            const Qt::DayOfWeek first = QLocale().firstDayOfWeek();
            mLimitScheduleDaysComboBox->addItem(locale.dayName(first), QVariant::fromValue(daysFromQtDay(first)));

            for (Qt::DayOfWeek day = nextDay(first); day != first; day = nextDay(day)) {
                mLimitScheduleDaysComboBox->addItem(locale.dayName(day), QVariant::fromValue(daysFromQtDay(day)));
            }
        }

        scheduleGroupBoxLayout->addRow(qApp->translate("tremotesf", "Days:"), mLimitScheduleDaysComboBox);

        alternativeSpeedLimitsGroupBoxLayout->addWidget(mLimitScheduleGroupBox);

        auto alternativeSpeedLimitsResizer = new KColumnResizer(this);
        alternativeSpeedLimitsResizer->addWidgetsFromLayout(enableAlternativeLimitsGroupBoxLayout);
        alternativeSpeedLimitsResizer->addWidgetsFromLayout(scheduleGroupBoxLayout);

        speedPageLayout->addWidget(alternativeSpeedLimitsGroupBox);

        speedPageLayout->addStretch();

        // Network page
        mNetworkPageWidget = new QWidget(this);
        KPageWidgetItem* networkPageItem = pageWidget->addPage(
            mNetworkPageWidget,
            //: "Network" server settings page
            qApp->translate("tremotesf", "Network")
        );
        networkPageItem->setIcon(QIcon::fromTheme("preferences-system-network"_l1));

        auto networkPageLayout = new QVBoxLayout(mNetworkPageWidget);

        //: Title of settings section related to peer connections
        auto connectionGroupBox = new QGroupBox(qApp->translate("tremotesf", "Connection"), this);
        auto connectionGroupBoxLayout = new QFormLayout(connectionGroupBox);
        connectionGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mPeerPortSpinBox = new QSpinBox(this);
        mPeerPortSpinBox->setMaximum(65535);
        connectionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Peer port:"), mPeerPortSpinBox);

        //: Check box label
        mRandomPortCheckBox = new QCheckBox(qApp->translate("tremotesf", "Random port on Transmission start"), this);
        connectionGroupBoxLayout->addRow(mRandomPortCheckBox);

        //: Check box label
        mPortForwardingCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable port forwarding"));
        connectionGroupBoxLayout->addRow(mPortForwardingCheckBox);

        mEncryptionComboBox = new QComboBox();
        for (const auto mode : encryptionModeComboBoxItems) {
            switch (mode) {
            case ServerSettingsData::EncryptionMode::Allowed:
                //: Encryption mode (allow/prefer/require)
                mEncryptionComboBox->addItem(qApp->translate("tremotesf", "Allow"));
                break;
            case ServerSettingsData::EncryptionMode::Preferred:
                //: Encryption mode (allow/prefer/require)
                mEncryptionComboBox->addItem(qApp->translate("tremotesf", "Prefer"));
                break;
            case ServerSettingsData::EncryptionMode::Required:
                //: Encryption mode (allow/prefer/require)
                mEncryptionComboBox->addItem(qApp->translate("tremotesf", "Require"));
                break;
            }
        }

        connectionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Encryption:"), mEncryptionComboBox);

        //: Check box label
        mUtpCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable μTP (Micro Transport Protocol)"), this);
        connectionGroupBoxLayout->addRow(mUtpCheckBox);

        //: Check box label
        mPexCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable PEX (Peer exchange)"), this);
        connectionGroupBoxLayout->addRow(mPexCheckBox);

        //: Check box label
        mDhtCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable DHT"), this);
        connectionGroupBoxLayout->addRow(mDhtCheckBox);

        //: Check box label
        mLpdCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable local peer discovery"), this);
        connectionGroupBoxLayout->addRow(mLpdCheckBox);

        networkPageLayout->addWidget(connectionGroupBox);

        auto peerLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Peer Limits"), this);
        auto peerLimitsGroupBoxLayout = new QFormLayout(peerLimitsGroupBox);
        peerLimitsGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mTorrentPeerLimitSpinBox = new QSpinBox(this);
        mTorrentPeerLimitSpinBox->setMaximum(std::numeric_limits<int>::max());
        peerLimitsGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "Maximum peers per torrent:"),
            mTorrentPeerLimitSpinBox
        );

        mGlobalPeerLimitSpinBox = new QSpinBox(this);
        mGlobalPeerLimitSpinBox->setMaximum(std::numeric_limits<int>::max());
        peerLimitsGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "Maximum peers globally:"),
            mGlobalPeerLimitSpinBox
        );

        networkPageLayout->addWidget(peerLimitsGroupBox);

        networkPageLayout->addStretch();

        auto networkPageResizer = new KColumnResizer(this);
        networkPageResizer->addWidgetsFromLayout(connectionGroupBoxLayout);
        networkPageResizer->addWidgetsFromLayout(peerLimitsGroupBoxLayout);

        layout->addWidget(pageWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ServerSettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &ServerSettingsDialog::reject);
        pageWidget->setPageFooter(dialogButtonBox);

        setMinimumSize(minimumSizeHint());
    }

    void ServerSettingsDialog::loadSettings() {
        const ServerSettings* settings = mRpc->serverSettings();

        mDownloadDirectoryWidget->updatePath(settings->data().downloadDirectory);
        mStartAddedTorrentsCheckBox->setChecked(settings->data().startAddedTorrents);
        mIncompleteFilesCheckBox->setChecked(settings->data().renameIncompleteFiles);
        mIncompleteDirectoryCheckBox->setChecked(settings->data().incompleteDirectoryEnabled);
        mIncompleteDirectoryWidget->updatePath(settings->data().incompleteDirectory);

        mRatioLimitCheckBox->setChecked(settings->data().ratioLimited);
        mRatioLimitSpinBox->setValue(settings->data().ratioLimit);
        mIdleSeedingLimitCheckBox->setChecked(settings->data().idleSeedingLimited);
        mIdleSeedingLimitSpinBox->setValue(settings->data().idleSeedingLimit);

        mMaximumActiveDownloadsCheckBox->setChecked(settings->data().downloadQueueEnabled);
        mMaximumActiveDownloadsSpinBox->setValue(settings->data().downloadQueueSize);
        mMaximumActiveUploadsCheckBox->setChecked(settings->data().seedQueueEnabled);
        mMaximumActiveUploadsSpinBox->setValue(settings->data().seedQueueSize);
        mIdleQueueLimitCheckBox->setChecked(settings->data().idleQueueLimited);
        mIdleQueueLimitSpinBox->setValue(settings->data().idleQueueLimit);

        mDownloadSpeedLimitCheckBox->setChecked(settings->data().downloadSpeedLimited);
        mDownloadSpeedLimitSpinBox->setValue(settings->data().downloadSpeedLimit);
        mUploadSpeedLimitCheckBox->setChecked(settings->data().uploadSpeedLimited);
        mUploadSpeedLimitSpinBox->setValue(settings->data().uploadSpeedLimit);
        mEnableAlternativeSpeedLimitsGroupBox->setChecked(settings->data().alternativeSpeedLimitsEnabled);
        mAlternativeDownloadSpeedLimitSpinBox->setValue(settings->data().alternativeDownloadSpeedLimit);
        mAlternativeUploadSpeedLimitSpinBox->setValue(settings->data().alternativeUploadSpeedLimit);
        mLimitScheduleGroupBox->setChecked(settings->data().alternativeSpeedLimitsScheduled);
        mLimitScheduleBeginTimeEdit->setTime(settings->data().alternativeSpeedLimitsBeginTime);
        mLimitScheduleEndTimeEdit->setTime(settings->data().alternativeSpeedLimitsEndTime);

        const auto days = settings->data().alternativeSpeedLimitsDays;
        for (int i = 0, max = mLimitScheduleDaysComboBox->count(); i < max; i++) {
            if (mLimitScheduleDaysComboBox->itemData(i).value<ServerSettingsData::AlternativeSpeedLimitsDays>() ==
                days) {
                mLimitScheduleDaysComboBox->setCurrentIndex(i);
                break;
            }
        }

        mPeerPortSpinBox->setValue(settings->data().peerPort);
        mRandomPortCheckBox->setChecked(settings->data().randomPortEnabled);
        mPortForwardingCheckBox->setChecked(settings->data().portForwardingEnabled);
        mEncryptionComboBox->setCurrentIndex(
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            indexOfCasted<int>(encryptionModeComboBoxItems, settings->data().encryptionMode).value()
        );
        mUtpCheckBox->setChecked(settings->data().utpEnabled);
        mPexCheckBox->setChecked(settings->data().pexEnabled);
        mDhtCheckBox->setChecked(settings->data().dhtEnabled);
        mLpdCheckBox->setChecked(settings->data().lpdEnabled);
        mTorrentPeerLimitSpinBox->setValue(settings->data().maximumPeersPerTorrent);
        mGlobalPeerLimitSpinBox->setValue(settings->data().maximumPeersGlobally);
    }
}
