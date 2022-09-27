// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serversettingsdialog.h"

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

#include "libtremotesf/serversettings.h"
#include "libtremotesf/stdutils.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/ui/widgets/remotedirectoryselectionwidget.h"

namespace tremotesf
{
    namespace
    {
        const libtremotesf::ServerSettingsData::EncryptionMode encryptionModeComboBoxItems[] = {
            libtremotesf::ServerSettingsData::AllowedEncryption,
            libtremotesf::ServerSettingsData::PreferredEncryption,
            libtremotesf::ServerSettingsData::RequiredEncryption
        };
    }

    ServerSettingsDialog::ServerSettingsDialog(const Rpc* rpc, QWidget* parent)
        : QDialog(parent),
          mRpc(rpc)
    {
        setWindowTitle(qApp->translate("tremotesf", "Server Options"));

        setupUi();

        QObject::connect(mRpc, &Rpc::connectedChanged, this, [=] {
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
        });

        loadSettings();
    }

    QSize ServerSettingsDialog::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(700, 550));
    }

    void ServerSettingsDialog::accept()
    {
        libtremotesf::ServerSettings* settings = mRpc->serverSettings();

        settings->setSaveOnSet(false);

        settings->setDownloadDirectory(mDownloadDirectoryWidget->text());
        settings->setStartAddedTorrents(mStartAddedTorrentsCheckBox->isChecked());
        //settings->setTrashTorrentFiles(mTrashTorrentFilesCheckBox->isChecked());
        settings->setRenameIncompleteFiles(mIncompleteFilesCheckBox->isChecked());
        settings->setIncompleteDirectoryEnabled(mIncompleteDirectoryCheckBox->isChecked());
        settings->setIncompleteDirectory(mIncompleteDirectoryWidget->text());

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

        settings->setAlternativeSpeedLimitsDays(static_cast<libtremotesf::ServerSettingsData::AlternativeSpeedLimitsDays>(mLimitScheduleDaysComboBox->currentData().toInt()));

        settings->setPeerPort(mPeerPortSpinBox->value());
        settings->setRandomPortEnabled(mRandomPortCheckBox->isChecked());
        settings->setPortForwardingEnabled(mPortForwardingCheckBox->isChecked());
        settings->setEncryptionMode(encryptionModeComboBoxItems[mEncryptionComboBox->currentIndex()]);
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

    void ServerSettingsDialog::setupUi()
    {
        //
        // Creating layout
        //

        auto layout = new QVBoxLayout(this);

        mDisconnectedMessageWidget = new KMessageWidget(qApp->translate("tremotesf", "Disconnected"), this);
        mDisconnectedMessageWidget->setCloseButtonVisible(false);
        mDisconnectedMessageWidget->setMessageType(KMessageWidget::Warning);
        mDisconnectedMessageWidget->setVisible(false);
        layout->addWidget(mDisconnectedMessageWidget);

        auto pageWidget = new KPageWidget(this);

        // Downloading page
        mDownloadingPageWidget = new QWidget(this);

        KPageWidgetItem* downloadingPageItem = pageWidget->addPage(mDownloadingPageWidget, qApp->translate("tremotesf", "Downloading", "Noun"));
        downloadingPageItem->setIcon(QIcon::fromTheme(QLatin1String("folder-download")));

        auto downloadingPageLayout = new QFormLayout(mDownloadingPageWidget);

        mDownloadDirectoryWidget = new RemoteDirectorySelectionWidget(QString(), mRpc, false, this);
        downloadingPageLayout->addRow(qApp->translate("tremotesf", "Download directory:"), mDownloadDirectoryWidget);

        mStartAddedTorrentsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Start added torrents"), this);
        downloadingPageLayout->addRow(mStartAddedTorrentsCheckBox);

        /*mTrashTorrentFilesCheckBox = new QCheckBox(qApp->translate("tremotesf", "Trash .torrent files"), this);
        downloadingPageLayout->addRow(mTrashTorrentFilesCheckBox);*/

        mIncompleteFilesCheckBox = new QCheckBox(qApp->translate("tremotesf", "Append \".part\" to names of incomplete files"), this);
        downloadingPageLayout->addRow(mIncompleteFilesCheckBox);

        mIncompleteDirectoryCheckBox = new QCheckBox(qApp->translate("tremotesf", "Directory for incomplete files:"), this);
        downloadingPageLayout->addRow(mIncompleteDirectoryCheckBox);

        auto incompleteDirectoryWidgetLayout = new QHBoxLayout();
        downloadingPageLayout->addRow(incompleteDirectoryWidgetLayout);
        mIncompleteDirectoryWidget = new RemoteDirectorySelectionWidget(QString(), mRpc, false, this);
        mIncompleteDirectoryWidget->setEnabled(false);
        QObject::connect(mIncompleteDirectoryCheckBox, &QCheckBox::toggled, mIncompleteDirectoryWidget, &FileSelectionWidget::setEnabled);
        //downloadingPageLayout->addRow(mIncompleteDirectoryCheckBox, mIncompleteDirectoryWidget);
        incompleteDirectoryWidgetLayout->addSpacing(28);
        incompleteDirectoryWidgetLayout->addWidget(mIncompleteDirectoryWidget);

        // Seeding page
        mSeedingPageWidget = new QWidget(this);
        KPageWidgetItem* seedingPageItem = pageWidget->addPage(mSeedingPageWidget, qApp->translate("tremotesf", "Seeding", "Noun"));
        seedingPageItem->setIcon(QIcon::fromTheme(QLatin1String("network-server")));

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
        //: Minutes
        mIdleSeedingLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));
        QObject::connect(mIdleSeedingLimitCheckBox, &QCheckBox::toggled, mIdleSeedingLimitSpinBox, &QSpinBox::setEnabled);
        seedingPageLayout->addWidget(mIdleSeedingLimitSpinBox, 3, 1, Qt::AlignTop);

        seedingPageLayout->setRowStretch(3, 1);
        seedingPageLayout->setColumnStretch(1, 1);
        seedingPageLayout->setColumnMinimumWidth(0, 28 - seedingPageLayout->spacing());

        // Queue page
        mQueuePageWidget = new QWidget(this);
        KPageWidgetItem* queuePageItem = pageWidget->addPage(mQueuePageWidget, qApp->translate("tremotesf", "Queue"));
        queuePageItem->setIcon(QIcon::fromTheme(QLatin1String("applications-utilities")));

        auto queuePageLayout = new QGridLayout(mQueuePageWidget);

        mMaximumActiveDownloadsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Maximum active downloads:"), this);
        queuePageLayout->addWidget(mMaximumActiveDownloadsCheckBox, 0, 0, 1, 2, Qt::AlignTop);

        mMaximumActiveDownloadsSpinBox = new QSpinBox(this);
        mMaximumActiveDownloadsSpinBox->setEnabled(false);
        mMaximumActiveDownloadsSpinBox->setMaximum(std::numeric_limits<int>::max());
        QObject::connect(mMaximumActiveDownloadsCheckBox, &QCheckBox::toggled, mMaximumActiveDownloadsSpinBox, &QSpinBox::setEnabled);
        queuePageLayout->addWidget(mMaximumActiveDownloadsSpinBox, 1, 1, Qt::AlignTop);

        mMaximumActiveUploadsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Maximum active uploads:"), this);
        queuePageLayout->addWidget(mMaximumActiveUploadsCheckBox, 2, 0, 1, 2, Qt::AlignTop);

        mMaximumActiveUploadsSpinBox = new QSpinBox(this);
        mMaximumActiveUploadsSpinBox->setEnabled(false);
        mMaximumActiveUploadsSpinBox->setMaximum(std::numeric_limits<int>::max());
        QObject::connect(mMaximumActiveUploadsCheckBox, &QCheckBox::toggled, mMaximumActiveUploadsSpinBox, &QSpinBox::setEnabled);
        queuePageLayout->addWidget(mMaximumActiveUploadsSpinBox, 3, 1, Qt::AlignTop);

        mIdleQueueLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Ignore queue position if idle for:"), this);
        queuePageLayout->addWidget(mIdleQueueLimitCheckBox, 4, 0, 1, 2, Qt::AlignTop);

        mIdleQueueLimitSpinBox = new QSpinBox(this);
        mIdleQueueLimitSpinBox->setEnabled(false);
        mIdleQueueLimitSpinBox->setMaximum(9999);
        //: Minutes
        mIdleQueueLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));
        QObject::connect(mIdleQueueLimitCheckBox, &QCheckBox::toggled, mIdleQueueLimitSpinBox, &QSpinBox::setEnabled);
        queuePageLayout->addWidget(mIdleQueueLimitSpinBox, 5, 1, Qt::AlignTop);

        queuePageLayout->setRowStretch(5, 1);
        queuePageLayout->setColumnStretch(1, 1);
        queuePageLayout->setColumnMinimumWidth(0, 28 - queuePageLayout->spacing());

        // Speed page
        mSpeedPageWidget = new QWidget(this);
        KPageWidgetItem* speedPageItem = pageWidget->addPage(mSpeedPageWidget, qApp->translate("tremotesf", "Speed"));
        speedPageItem->setIcon(QIcon::fromTheme(QLatin1String("preferences-system-time")));

        auto speedPageLayout = new QVBoxLayout(mSpeedPageWidget);

        auto speedLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Limits"), this);
        auto speedLimitsGroupBoxLayout = new QGridLayout(speedLimitsGroupBox);

        const int maxSpeedLimit = static_cast<int>(std::numeric_limits<uint>::max() / 1024);
        //: In this context, 'k' prefix means SI prefix, i.e kB = 1000 bytes
        const QString suffix(qApp->translate("tremotesf", " kB/s"));

        //: Noun
        mDownloadSpeedLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Download:"), this);
        speedLimitsGroupBoxLayout->addWidget(mDownloadSpeedLimitCheckBox, 0, 0, 1, 2);

        mDownloadSpeedLimitSpinBox = new QSpinBox(this);
        mDownloadSpeedLimitSpinBox->setEnabled(false);
        mDownloadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mDownloadSpeedLimitSpinBox->setSuffix(suffix);
        QObject::connect(mDownloadSpeedLimitCheckBox, &QCheckBox::toggled, mDownloadSpeedLimitSpinBox, &QSpinBox::setEnabled);
        speedLimitsGroupBoxLayout->addWidget(mDownloadSpeedLimitSpinBox, 1, 1);

        //: Noun
        mUploadSpeedLimitCheckBox = new QCheckBox(qApp->translate("tremotesf", "Upload:"), this);
        speedLimitsGroupBoxLayout->addWidget(mUploadSpeedLimitCheckBox, 2, 0, 1, 2);

        mUploadSpeedLimitSpinBox = new QSpinBox(this);
        mUploadSpeedLimitSpinBox->setEnabled(false);
        mUploadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mUploadSpeedLimitSpinBox->setSuffix(suffix);
        QObject::connect(mUploadSpeedLimitCheckBox, &QCheckBox::toggled, mUploadSpeedLimitSpinBox, &QSpinBox::setEnabled);
        speedLimitsGroupBoxLayout->addWidget(mUploadSpeedLimitSpinBox, 3, 1);

        speedLimitsGroupBoxLayout->setColumnMinimumWidth(0, 28 - speedLimitsGroupBoxLayout->spacing());

        speedPageLayout->addWidget(speedLimitsGroupBox);

        auto alternativeSpeedLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Alternative Limits"), this);
        auto alternativeSpeedLimitsGroupBoxLayout = new QVBoxLayout(alternativeSpeedLimitsGroupBox);

        mEnableAlternativeSpeedLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Enable"), this);
        mEnableAlternativeSpeedLimitsGroupBox->setCheckable(true);
        auto enableAlternativeLimitsGroupBoxLayout = new QFormLayout(mEnableAlternativeSpeedLimitsGroupBox);
        enableAlternativeLimitsGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mAlternativeDownloadSpeedLimitSpinBox = new QSpinBox(this);
        mAlternativeDownloadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mAlternativeDownloadSpeedLimitSpinBox->setSuffix(suffix);
        enableAlternativeLimitsGroupBoxLayout->addRow(qApp->translate("tremotesf", "Download:"), mAlternativeDownloadSpeedLimitSpinBox);
        mAlternativeUploadSpeedLimitSpinBox = new QSpinBox(this);
        mAlternativeUploadSpeedLimitSpinBox->setMaximum(maxSpeedLimit);
        mAlternativeUploadSpeedLimitSpinBox->setSuffix(suffix);
        enableAlternativeLimitsGroupBoxLayout->addRow(qApp->translate("tremotesf", "Upload:"), mAlternativeUploadSpeedLimitSpinBox);

        alternativeSpeedLimitsGroupBoxLayout->addWidget(mEnableAlternativeSpeedLimitsGroupBox);

        mLimitScheduleGroupBox = new QGroupBox(qApp->translate("tremotesf", "Scheduled"), this);
        mLimitScheduleGroupBox->setCheckable(true);
        auto scheduleGroupBoxLayout = new QFormLayout(mLimitScheduleGroupBox);
        scheduleGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto scheduleTimeLayout = new QHBoxLayout();
        scheduleGroupBoxLayout->addRow(scheduleTimeLayout);
        mLimitScheduleBeginTimeEdit = new QTimeEdit(this);
        scheduleTimeLayout->addWidget(mLimitScheduleBeginTimeEdit, 1);
        //: e.g. inside "From 1:00 AM to 5:00 AM"
        scheduleTimeLayout->addWidget(new QLabel(qApp->translate("tremotesf", "to")));
        mLimitScheduleEndTimeEdit = new QTimeEdit(this);
        scheduleTimeLayout->addWidget(mLimitScheduleEndTimeEdit, 1);

        using libtremotesf::ServerSettingsData;

        mLimitScheduleDaysComboBox = new QComboBox(this);
        mLimitScheduleDaysComboBox->addItem(qApp->translate("tremotesf", "Every day"), ServerSettingsData::All);
        mLimitScheduleDaysComboBox->addItem(qApp->translate("tremotesf", "Weekdays"), ServerSettingsData::Weekdays);
        mLimitScheduleDaysComboBox->addItem(qApp->translate("tremotesf", "Weekends"), ServerSettingsData::Weekends);
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
                    return ServerSettingsData::Monday;
                case Qt::Tuesday:
                    return ServerSettingsData::Tuesday;
                case Qt::Wednesday:
                    return ServerSettingsData::Wednesday;
                case Qt::Thursday:
                    return ServerSettingsData::Thursday;
                case Qt::Friday:
                    return ServerSettingsData::Friday;
                case Qt::Saturday:
                    return ServerSettingsData::Saturday;
                case Qt::Sunday:
                    return ServerSettingsData::Sunday;
                }
                return ServerSettingsData::All;
            };

            const QLocale locale;

            const Qt::DayOfWeek first = QLocale().firstDayOfWeek();
            mLimitScheduleDaysComboBox->addItem(locale.dayName(first), daysFromQtDay(first));

            for (Qt::DayOfWeek day = nextDay(first); day != first; day = nextDay(day)) {
                mLimitScheduleDaysComboBox->addItem(locale.dayName(day), daysFromQtDay(day));
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
        KPageWidgetItem* networkPageItem = pageWidget->addPage(mNetworkPageWidget, qApp->translate("tremotesf", "Network"));
        networkPageItem->setIcon(QIcon::fromTheme(QLatin1String("preferences-system-network")));

        auto networkPageLayout = new QVBoxLayout(mNetworkPageWidget);

        auto connectionGroupBox = new QGroupBox(qApp->translate("tremotesf", "Connection"), this);
        auto connectionGroupBoxLayout = new QFormLayout(connectionGroupBox);
        connectionGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mPeerPortSpinBox = new QSpinBox(this);
        mPeerPortSpinBox->setMaximum(65535);
        connectionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Peer port:"), mPeerPortSpinBox);

        mRandomPortCheckBox = new QCheckBox(qApp->translate("tremotesf", "Random port on Transmission start"), this);
        connectionGroupBoxLayout->addRow(mRandomPortCheckBox);

        mPortForwardingCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable port forwarding"));
        connectionGroupBoxLayout->addRow(mPortForwardingCheckBox);

        mEncryptionComboBox = new QComboBox();
        for (ServerSettingsData::EncryptionMode mode : encryptionModeComboBoxItems) {
            switch (mode) {
            case ServerSettingsData::AllowedEncryption:
                //: Encryption mode
                mEncryptionComboBox->addItem(qApp->translate("tremotesf", "Allow"));
                break;
            case ServerSettingsData::PreferredEncryption:
                //: Encryption mode
                mEncryptionComboBox->addItem(qApp->translate("tremotesf", "Prefer"));
                break;
            case ServerSettingsData::RequiredEncryption:
                //: Encryption mode
                mEncryptionComboBox->addItem(qApp->translate("tremotesf", "Require"));
                break;
            }
        }

        connectionGroupBoxLayout->addRow(qApp->translate("tremotesf", "Encryption:"), mEncryptionComboBox);

        mUtpCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable uTP"), this);
        connectionGroupBoxLayout->addRow(mUtpCheckBox);

        mPexCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable PEX"), this);
        connectionGroupBoxLayout->addRow(mPexCheckBox);

        mDhtCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable DHT"), this);
        connectionGroupBoxLayout->addRow(mDhtCheckBox);

        mLpdCheckBox = new QCheckBox(qApp->translate("tremotesf", "Enable LPD"), this);
        connectionGroupBoxLayout->addRow(mLpdCheckBox);

        networkPageLayout->addWidget(connectionGroupBox);

        auto peerLimitsGroupBox = new QGroupBox(qApp->translate("tremotesf", "Peer Limits"), this);
        auto peerLimitsGroupBoxLayout = new QFormLayout(peerLimitsGroupBox);
        peerLimitsGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        mTorrentPeerLimitSpinBox = new QSpinBox(this);
        mTorrentPeerLimitSpinBox->setMaximum(std::numeric_limits<int>::max());
        peerLimitsGroupBoxLayout->addRow(qApp->translate("tremotesf", "Maximum peers per torrent:"), mTorrentPeerLimitSpinBox);

        mGlobalPeerLimitSpinBox = new QSpinBox(this);
        mGlobalPeerLimitSpinBox->setMaximum(std::numeric_limits<int>::max());
        peerLimitsGroupBoxLayout->addRow(qApp->translate("tremotesf", "Maximum peers globally:"), mGlobalPeerLimitSpinBox);

        networkPageLayout->addWidget(peerLimitsGroupBox);

        networkPageLayout->addStretch();

        auto networkPageResizer = new KColumnResizer(this);
        networkPageResizer->addWidgetsFromLayout(connectionGroupBoxLayout);
        networkPageResizer->addWidgetsFromLayout(peerLimitsGroupBoxLayout);

        layout->addWidget(pageWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ServerSettingsDialog::accept);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &ServerSettingsDialog::reject);
        layout->addWidget(dialogButtonBox);

        setMinimumSize(minimumSizeHint());
    }

    void ServerSettingsDialog::loadSettings()
    {
        const libtremotesf::ServerSettings* settings = mRpc->serverSettings();

        mDownloadDirectoryWidget->setText(settings->downloadDirectory());
        mStartAddedTorrentsCheckBox->setChecked(settings->startAddedTorrents());
        //mTrashTorrentFilesCheckBox->setChecked(settings->trashTorrentFiles());
        mIncompleteFilesCheckBox->setChecked(settings->renameIncompleteFiles());
        mIncompleteDirectoryCheckBox->setChecked(settings->isIncompleteDirectoryEnabled());
        mIncompleteDirectoryWidget->setText(settings->incompleteDirectory());

        mRatioLimitCheckBox->setChecked(settings->isRatioLimited());
        mRatioLimitSpinBox->setValue(settings->ratioLimit());
        mIdleSeedingLimitCheckBox->setChecked(settings->isIdleSeedingLimited());
        mIdleSeedingLimitSpinBox->setValue(settings->idleSeedingLimit());

        mMaximumActiveDownloadsCheckBox->setChecked(settings->isDownloadQueueEnabled());
        mMaximumActiveDownloadsSpinBox->setValue(settings->downloadQueueSize());
        mMaximumActiveUploadsCheckBox->setChecked(settings->isSeedQueueEnabled());
        mMaximumActiveUploadsSpinBox->setValue(settings->seedQueueSize());
        mIdleQueueLimitCheckBox->setChecked(settings->isIdleQueueLimited());
        mIdleQueueLimitSpinBox->setValue(settings->idleQueueLimit());

        mDownloadSpeedLimitCheckBox->setChecked(settings->isDownloadSpeedLimited());
        mDownloadSpeedLimitSpinBox->setValue(settings->downloadSpeedLimit());
        mUploadSpeedLimitCheckBox->setChecked(settings->isUploadSpeedLimited());
        mUploadSpeedLimitSpinBox->setValue(settings->uploadSpeedLimit());
        mEnableAlternativeSpeedLimitsGroupBox->setChecked(settings->isAlternativeSpeedLimitsEnabled());
        mAlternativeDownloadSpeedLimitSpinBox->setValue(settings->alternativeDownloadSpeedLimit());
        mAlternativeUploadSpeedLimitSpinBox->setValue(settings->alternativeUploadSpeedLimit());
        mLimitScheduleGroupBox->setChecked(settings->isAlternativeSpeedLimitsScheduled());
        mLimitScheduleBeginTimeEdit->setTime(settings->alternativeSpeedLimitsBeginTime());
        mLimitScheduleEndTimeEdit->setTime(settings->alternativeSpeedLimitsEndTime());

        const libtremotesf::ServerSettingsData::AlternativeSpeedLimitsDays days = settings->alternativeSpeedLimitsDays();
        for (int i = 0, max = mLimitScheduleDaysComboBox->count(); i < max; i++) {
            if (mLimitScheduleDaysComboBox->itemData(i).toInt() == days) {
                mLimitScheduleDaysComboBox->setCurrentIndex(i);
                break;
            }
        }

        mPeerPortSpinBox->setValue(settings->peerPort());
        mRandomPortCheckBox->setChecked(settings->isRandomPortEnabled());
        mPortForwardingCheckBox->setChecked(settings->isPortForwardingEnabled());
        mEncryptionComboBox->setCurrentIndex(index_of_i(encryptionModeComboBoxItems, settings->encryptionMode()));
        mUtpCheckBox->setChecked(settings->isUtpEnabled());
        mPexCheckBox->setChecked(settings->isPexEnabled());
        mDhtCheckBox->setChecked(settings->isDhtEnabled());
        mLpdCheckBox->setChecked(settings->isLpdEnabled());
        mTorrentPeerLimitSpinBox->setValue(settings->maximumPeersPerTorrent());
        mGlobalPeerLimitSpinBox->setValue(settings->maximumPeersGlobally());
    }
}
