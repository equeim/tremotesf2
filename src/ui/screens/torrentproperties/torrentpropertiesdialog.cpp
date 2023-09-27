// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentpropertiesdialog.h"

#include <limits>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <KColumnResizer>
#include <KMessageWidget>

#include "rpc/pathutils.h"
#include "stdutils.h"
#include "rpc/torrent.h"
#include "ui/itemmodels/baseproxymodel.h"
#include "ui/itemmodels/stringlistmodel.h"
#include "ui/widgets/torrentfilesview.h"
#include "ui/widgets/commondelegate.h"
#include "rpc/rpc.h"
#include "desktoputils.h"
#include "settings.h"
#include "utils.h"
#include "peersmodel.h"
#include "torrentfilesmodel.h"
#include "trackersviewwidget.h"

namespace tremotesf {
    namespace {
        constexpr TorrentData::Priority priorityComboBoxItems[] = {
            TorrentData::Priority::High, TorrentData::Priority::Normal, TorrentData::Priority::Low};

        constexpr TorrentData::RatioLimitMode ratioLimitComboBoxItems[] = {
            TorrentData::RatioLimitMode::Global,
            TorrentData::RatioLimitMode::Unlimited,
            TorrentData::RatioLimitMode::Single};

        constexpr TorrentData::IdleSeedingLimitMode idleSeedingLimitComboBoxItems[] = {
            TorrentData::IdleSeedingLimitMode::Global,
            TorrentData::IdleSeedingLimitMode::Unlimited,
            TorrentData::IdleSeedingLimitMode::Single};
    }

    TorrentPropertiesDialog::TorrentPropertiesDialog(Torrent* torrent, Rpc* rpc, QWidget* parent)
        : QDialog(parent),
          mTorrent(torrent),
          mRpc(rpc),
          mMessageWidget(new KMessageWidget(this)),
          mTabWidget(new QTabWidget(this)),
          mFilesModel(new TorrentFilesModel(mTorrent, mRpc, this)),
          mTrackersViewWidget(new TrackersViewWidget(mTorrent, mRpc, this)),
          mPeersView(nullptr),
          mPeersModel(nullptr),
          mWebSeedersModel(nullptr) {
        auto layout = new QVBoxLayout(this);

        mMessageWidget->setCloseButtonVisible(false);
        mMessageWidget->setMessageType(KMessageWidget::Warning);
        mMessageWidget->hide();
        layout->addWidget(mMessageWidget);

        setupDetailsTab();
        //: Torrent properties dialog tab
        mTabWidget->addTab(new TorrentFilesView(mFilesModel, mRpc), qApp->translate("tremotesf", "Files"));
        //: Torrent properties dialog tab
        mTabWidget->addTab(mTrackersViewWidget, qApp->translate("tremotesf", "Trackers"));
        setupPeersTab();
        setupWebSeedersTab();
        setupLimitsTab();

        layout->addWidget(mTabWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TorrentPropertiesDialog::reject);
        layout->addWidget(dialogButtonBox);

        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);

        const QString torrentHash(mTorrent->data().hashString);
        QObject::connect(mRpc, &Rpc::torrentsUpdated, this, [=, this] {
            setTorrent(mRpc->torrentByHash(torrentHash));
        });

        onTorrentChanged();

        setMinimumSize(minimumSizeHint());
        restoreGeometry(Settings::instance()->torrentPropertiesDialogGeometry());
    }

    TorrentPropertiesDialog::~TorrentPropertiesDialog() {
        Settings::instance()->setPeersViewHeaderState(mPeersView->header()->saveState());
        Settings::instance()->setTorrentPropertiesDialogGeometry(saveGeometry());
    }

    QSize TorrentPropertiesDialog::sizeHint() const { return minimumSizeHint(); }

    void TorrentPropertiesDialog::setupDetailsTab() {
        auto detailsTab = new QWidget(this);
        //: Torrent's properties dialog tab
        mTabWidget->addTab(detailsTab, qApp->translate("tremotesf", "Details"));

        auto detailsTabLayout = new QVBoxLayout(detailsTab);

        //: Torrent's details tab section
        auto activityGroupBox = new QGroupBox(qApp->translate("tremotesf", "Activity"), this);
        auto activityGroupBoxLayout = new QFormLayout(activityGroupBox);
        auto completedLabel = new QLabel(this);
        //: Torrent's completed size
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Completed:"), completedLabel);
        auto downloadedLabel = new QLabel(this);
        //: Torrent's downloaded size
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Downloaded:"), downloadedLabel);
        auto uploadedLabel = new QLabel(this);
        //: Torrent's uploaded size
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Uploaded:"), uploadedLabel);
        auto ratioLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio:"), ratioLabel);
        auto downloadSpeedLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Download speed:"), downloadSpeedLabel);
        auto uploadSpeedLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Upload speed:"), uploadSpeedLabel);
        auto etaLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "ETA:"), etaLabel);
        auto seedersLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Seeders:"), seedersLabel);
        auto leechersLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Leechers:"), leechersLabel);
        auto peersSendingToUsLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "Peers we are downloading from:"),
            peersSendingToUsLabel
        );
        auto webSeedersSendingToUsLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "Web seeders we are downloading from:"),
            webSeedersSendingToUsLabel
        );
        auto peersGettingFromUsLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(
            qApp->translate("tremotesf", "Peers we are uploading to:"),
            peersGettingFromUsLabel
        );
        auto lastActivityLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Last activity:"), lastActivityLabel);
        detailsTabLayout->addWidget(activityGroupBox);

        //: Torrent's details tab section
        auto infoGroupBox = new QGroupBox(qApp->translate("tremotesf", "Information"), this);
        auto infoGroupBoxLayout = new QFormLayout(infoGroupBox);
        auto totalSizeLabel = new QLabel(this);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Total size:"), totalSizeLabel);
        auto locationLabel = new QLabel(this);
        locationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //: Torrent's download directory
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Location:"), locationLabel);
        auto hashLabel = new QLabel(mTorrent->data().hashString, this);
        hashLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //: Torrent's hash string
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Hash:"), hashLabel);
        auto creatorLabel = new QLabel(this);
        //: Program that created torrent file
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Created by:"), creatorLabel);
        auto creationDateLabel = new QLabel(this);
        //: Date/time when torrent was created
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Created on:"), creationDateLabel);
        auto commentTextEdit = new QTextBrowser(this);
        commentTextEdit->setOpenExternalLinks(true);
        //: Torrent's comment text
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Comment:"), commentTextEdit);
        detailsTabLayout->addWidget(infoGroupBox);

        auto resizer = new KColumnResizer(this);
        resizer->addWidgetsFromLayout(activityGroupBoxLayout);
        resizer->addWidgetsFromLayout(infoGroupBoxLayout);

        mUpdateDetailsTab = [=, this] {
            setWindowTitle(mTorrent->data().name);

            //: Torrent's completion size, e.g. 100 MiB of 200 MiB (50%). %1 is completed size, %2 is size, %3 is progress in percents
            completedLabel->setText(qApp->translate("tremotesf", "%1 of %2 (%3)")
                                        .arg(
                                            Utils::formatByteSize(mTorrent->data().completedSize),
                                            Utils::formatByteSize(mTorrent->data().sizeWhenDone),
                                            Utils::formatProgress(mTorrent->data().percentDone)
                                        ));
            downloadedLabel->setText(Utils::formatByteSize(mTorrent->data().totalDownloaded));
            uploadedLabel->setText(Utils::formatByteSize(mTorrent->data().totalUploaded));
            ratioLabel->setText(Utils::formatRatio(mTorrent->data().ratio));
            downloadSpeedLabel->setText(Utils::formatByteSpeed(mTorrent->data().downloadSpeed));
            uploadSpeedLabel->setText(Utils::formatByteSpeed(mTorrent->data().uploadSpeed));
            etaLabel->setText(Utils::formatEta(mTorrent->data().eta));

            const QLocale locale{};
            seedersLabel->setText(locale.toString(mTorrent->data().totalSeedersFromTrackersCount));
            leechersLabel->setText(locale.toString(mTorrent->data().totalLeechersFromTrackersCount));
            peersSendingToUsLabel->setText(locale.toString(mTorrent->data().peersSendingToUsCount));
            webSeedersSendingToUsLabel->setText(locale.toString(mTorrent->data().webSeedersSendingToUsCount));
            peersGettingFromUsLabel->setText(locale.toString(mTorrent->data().peersGettingFromUsCount));

            lastActivityLabel->setText(mTorrent->data().activityDate.toLocalTime().toString());

            totalSizeLabel->setText(Utils::formatByteSize(mTorrent->data().totalSize));
            locationLabel->setText(
                toNativeSeparators(mTorrent->data().downloadDirectory, mRpc->serverSettings()->data().pathOs)
            );
            creatorLabel->setText(mTorrent->data().creator);
            creationDateLabel->setText(mTorrent->data().creationDate.toLocalTime().toString());
            if (mTorrent->data().comment != commentTextEdit->toPlainText()) {
                commentTextEdit->document()->setPlainText(mTorrent->data().comment);
                desktoputils::findLinksAndAddAnchors(commentTextEdit->document());
            }
        };
    }

    void TorrentPropertiesDialog::setupPeersTab() {
        mPeersModel = new PeersModel(mTorrent, this);
        auto peersProxyModel =
            new BaseProxyModel(mPeersModel, PeersModel::SortRole, static_cast<int>(PeersModel::Column::Address), this);

        auto peersTab = new QWidget(this);
        auto peersTabLayout = new QVBoxLayout(peersTab);

        mPeersView = new BaseTreeView(this);
        mPeersView->setItemDelegate(
            new CommonDelegate(static_cast<int>(PeersModel::Column::ProgressBar), PeersModel::SortRole, -1, this)
        );
        mPeersView->setModel(peersProxyModel);
        mPeersView->setRootIsDecorated(false);
        mPeersView->header()->restoreState(Settings::instance()->peersViewHeaderState());

        peersTabLayout->addWidget(mPeersView);
        //: Torrent's properties dialog tab
        mTabWidget->addTab(peersTab, qApp->translate("tremotesf", "Peers"));
    }

    void TorrentPropertiesDialog::setupWebSeedersTab() {
        //: Web seeders list column title
        mWebSeedersModel = new StringListModel(qApp->translate("tremotesf", "Web seeder"), this);
        auto webSeedersProxyModel = new BaseProxyModel(mWebSeedersModel, Qt::DisplayRole, std::nullopt, this);

        auto webSeedersTab = new QWidget(this);
        auto webSeedersTabLayout = new QVBoxLayout(webSeedersTab);

        auto webSeedersView = new BaseTreeView(this);
        webSeedersView->header()->setContextMenuPolicy(Qt::DefaultContextMenu);
        webSeedersView->setModel(webSeedersProxyModel);
        webSeedersView->setRootIsDecorated(false);

        webSeedersTabLayout->addWidget(webSeedersView);

        //: Torrent's properties dialog tab
        mTabWidget->addTab(webSeedersTab, qApp->translate("tremotesf", "Web seeders"));
    }

    void TorrentPropertiesDialog::setupLimitsTab() {
        auto limitsTab = new QWidget(this);
        //: Torrent's properties dialog tab
        mTabWidget->addTab(limitsTab, qApp->translate("tremotesf", "Limits"));

        auto limitsTabLayout = new QVBoxLayout(limitsTab);

        //
        // Speed group box
        //
        //: Torrent's limits tab section
        auto speedGroupBox = new QGroupBox(qApp->translate("tremotesf", "Speed"), this);
        auto speedGroupBoxLayout = new QFormLayout(speedGroupBox);
        speedGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        speedGroupBoxLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        //: Check box label
        auto globalLimitsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Honor global limits"), this);
        speedGroupBoxLayout->addRow(globalLimitsCheckBox);

        const int maxSpeedLimit = static_cast<int>(std::numeric_limits<uint>::max() / 1024);

        //: Download speed limit input field label
        auto downloadSpeedCheckBox = new QCheckBox(qApp->translate("tremotesf", "Download:"), this);
        speedGroupBoxLayout->addRow(downloadSpeedCheckBox);

        auto downloadSpeedSpinBoxLayout = new QHBoxLayout();
        speedGroupBoxLayout->addRow(downloadSpeedSpinBoxLayout);
        auto downloadSpeedSpinBox = new QSpinBox(this);
        downloadSpeedSpinBox->setEnabled(false);
        downloadSpeedSpinBox->setMaximum(maxSpeedLimit);
        //: Suffix that is added to input field with download/upload speed limit, e.g. "5000 kB/s". 'k' prefix means SI prefix, i.e kB = 1000 bytes
        downloadSpeedSpinBox->setSuffix(qApp->translate("tremotesf", " kB/s"));

        QObject::connect(downloadSpeedCheckBox, &QCheckBox::toggled, downloadSpeedSpinBox, &QSpinBox::setEnabled);

        downloadSpeedSpinBoxLayout->addSpacing(28);
        downloadSpeedSpinBoxLayout->addWidget(downloadSpeedSpinBox);

        //: Upload speed limit input field label
        auto uploadSpeedCheckBox = new QCheckBox(qApp->translate("tremotesf", "Upload:"), this);
        speedGroupBoxLayout->addRow(uploadSpeedCheckBox);

        auto uploadSpeedSpinBoxLayout = new QHBoxLayout();
        speedGroupBoxLayout->addRow(uploadSpeedSpinBoxLayout);
        auto uploadSpeedSpinBox = new QSpinBox(this);
        uploadSpeedSpinBox->setEnabled(false);
        uploadSpeedSpinBox->setMaximum(maxSpeedLimit);
        //: Suffix that is added to input field with download/upload speed limit, e.g. "5000 kB/s". 'k' prefix means SI prefix, i.e kB = 1000 bytes
        uploadSpeedSpinBox->setSuffix(qApp->translate("tremotesf", " kB/s"));

        QObject::connect(uploadSpeedCheckBox, &QCheckBox::toggled, uploadSpeedSpinBox, &QSpinBox::setEnabled);

        uploadSpeedSpinBoxLayout->addSpacing(28);
        uploadSpeedSpinBoxLayout->addWidget(uploadSpeedSpinBox);

        auto priorityComboBox = new QComboBox(this);
        for (const TorrentData::Priority priority : priorityComboBoxItems) {
            switch (priority) {
            case TorrentData::Priority::High:
                //: Torrent's loading priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "High"));
                break;
            case TorrentData::Priority::Normal:
                //: Torrent's loading priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "Normal"));
                break;
            case TorrentData::Priority::Low:
                //: Torrent's loading priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "Low"));
                break;
            }
        }
        speedGroupBoxLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), priorityComboBox);

        limitsTabLayout->addWidget(speedGroupBox);

        //
        // Seeding group box
        //
        //: Torrent's limits tab section
        auto seedingGroupBox = new QGroupBox(qApp->translate("tremotesf", "Seeding", "Options section"), this);
        auto seedingGroupBoxLayout = new QFormLayout(seedingGroupBox);
        seedingGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto ratioLimitLayout = new QHBoxLayout();
        seedingGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio limit mode:"), ratioLimitLayout);
        ratioLimitLayout->setContentsMargins(0, 0, 0, 0);
        auto ratioLimitComboBox = new QComboBox(this);
        for (const TorrentData::RatioLimitMode mode : ratioLimitComboBoxItems) {
            switch (mode) {
            case TorrentData::RatioLimitMode::Global:
                //: Seeding ratio limit mode (global settings/stop at ratio/unlimited)
                ratioLimitComboBox->addItem(qApp->translate("tremotesf", "Use global settings"));
                break;
            case TorrentData::RatioLimitMode::Single:
                //: Seeding ratio limit mode (global settings/stop at ratio/unlimited)
                ratioLimitComboBox->addItem(qApp->translate("tremotesf", "Stop seeding at ratio:"));
                break;
            case TorrentData::RatioLimitMode::Unlimited:
                //: Seeding ratio limit mode (global settings/stop at ratio/unlimited)
                ratioLimitComboBox->addItem(qApp->translate("tremotesf", "Seed regardless of ratio"));
                break;
            }
        }

        ratioLimitLayout->addWidget(ratioLimitComboBox);
        auto ratioLimitSpinBox = new QDoubleSpinBox(this);
        ratioLimitSpinBox->setMaximum(10000.0);
        ratioLimitSpinBox->setSingleStep(0.1);

        ratioLimitSpinBox->setVisible(false);
        QObject::connect(
            ratioLimitComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [ratioLimitSpinBox](int index) {
                if (index == indexOfCasted<int>(ratioLimitComboBoxItems, TorrentData::RatioLimitMode::Single)) {
                    ratioLimitSpinBox->show();
                } else {
                    ratioLimitSpinBox->hide();
                }
            }
        );
        ratioLimitLayout->addWidget(ratioLimitSpinBox);

        auto idleSeedingLimitLayout = new QHBoxLayout();
        seedingGroupBoxLayout->addRow(qApp->translate("tremotesf", "Idle seeding mode:"), idleSeedingLimitLayout);
        idleSeedingLimitLayout->setContentsMargins(0, 0, 0, 0);
        auto idleSeedingLimitComboBox = new QComboBox(this);
        for (const TorrentData::IdleSeedingLimitMode mode : idleSeedingLimitComboBoxItems) {
            switch (mode) {
            case TorrentData::IdleSeedingLimitMode::Global:
                //: Seeding idle limit mode (global settings/stop if idle for/unlimited)
                idleSeedingLimitComboBox->addItem(qApp->translate("tremotesf", "Use global settings"));
                break;
            case TorrentData::IdleSeedingLimitMode::Single:
                //: Seeding idle limit mode (global settings/stop if idle for/unlimited)
                idleSeedingLimitComboBox->addItem(qApp->translate("tremotesf", "Stop seeding if idle for:"));
                break;
            case TorrentData::IdleSeedingLimitMode::Unlimited:
                //: Seeding idle limit mode (global settings/stop if idle for/unlimited)
                idleSeedingLimitComboBox->addItem(qApp->translate("tremotesf", "Seed regardless of activity"));
                break;
            }
        }

        idleSeedingLimitLayout->addWidget(idleSeedingLimitComboBox);
        auto idleSeedingLimitSpinBox = new QSpinBox(this);
        idleSeedingLimitSpinBox->setMaximum(9999);
        //: Suffix that is added to input field with number of minuts, e.g. "5 min"
        idleSeedingLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));

        idleSeedingLimitSpinBox->setVisible(false);
        QObject::connect(
            idleSeedingLimitComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [idleSeedingLimitSpinBox](int index) {
                if (index ==
                    indexOfCasted<int>(idleSeedingLimitComboBoxItems, TorrentData::IdleSeedingLimitMode::Single)) {
                    idleSeedingLimitSpinBox->show();
                } else {
                    idleSeedingLimitSpinBox->hide();
                }
            }
        );
        idleSeedingLimitLayout->addWidget(idleSeedingLimitSpinBox);

        limitsTabLayout->addWidget(seedingGroupBox);

        //
        // Peers group box
        //
        //: Torrent's limits tab section
        auto peersGroupBox = new QGroupBox(qApp->translate("tremotesf", "Peers"), this);
        auto peersGroupBoxLayout = new QFormLayout(peersGroupBox);
        peersGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto peersLimitsSpinBox = new QSpinBox(this);
        peersLimitsSpinBox->setMaximum(9999);
        peersGroupBoxLayout->addRow(qApp->translate("tremotesf", "Maximum peers:"), peersLimitsSpinBox);

        limitsTabLayout->addWidget(peersGroupBox);

        limitsTabLayout->addStretch();

        auto resizer = new KColumnResizer(this);
        resizer->addWidgetsFromLayout(speedGroupBoxLayout);
        resizer->addWidgetsFromLayout(seedingGroupBoxLayout);
        resizer->addWidgetsFromLayout(peersGroupBoxLayout);

        QObject::connect(globalLimitsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
            if (!mUpdatingLimits && mTorrent) {
                mTorrent->setHonorSessionLimits(checked);
            }
        });
        QObject::connect(downloadSpeedCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
            if (!mUpdatingLimits && mTorrent) {
                mTorrent->setDownloadSpeedLimited(checked);
            }
        });
        QObject::connect(
            downloadSpeedSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int limit) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setDownloadSpeedLimit(limit);
                }
            }
        );
        QObject::connect(uploadSpeedCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
            if (!mUpdatingLimits && mTorrent) {
                mTorrent->setUploadSpeedLimited(checked);
            }
        });
        QObject::connect(
            uploadSpeedSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int limit) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setUploadSpeedLimit(limit);
                }
            }
        );
        QObject::connect(
            priorityComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setBandwidthPriority(priorityComboBoxItems[index]);
                }
            }
        );
        QObject::connect(
            ratioLimitComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setRatioLimitMode(ratioLimitComboBoxItems[index]);
                }
            }
        );
        QObject::connect(
            ratioLimitSpinBox,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,
            [this](double limit) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setRatioLimit(limit);
                }
            }
        );
        QObject::connect(
            idleSeedingLimitComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setIdleSeedingLimitMode(idleSeedingLimitComboBoxItems[index]);
                }
            }
        );
        QObject::connect(
            idleSeedingLimitSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int limit) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setIdleSeedingLimit(limit);
                }
            }
        );
        QObject::connect(
            peersLimitsSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            [this](int limit) {
                if (!mUpdatingLimits && mTorrent) {
                    mTorrent->setPeersLimit(limit);
                }
            }
        );

        mUpdateLimitsTab = [=, this] {
            mUpdatingLimits = true;

            globalLimitsCheckBox->setChecked(mTorrent->data().honorSessionLimits);
            downloadSpeedCheckBox->setChecked(mTorrent->data().downloadSpeedLimited);
            downloadSpeedSpinBox->setValue(mTorrent->data().downloadSpeedLimit);
            uploadSpeedCheckBox->setChecked(mTorrent->data().uploadSpeedLimited);
            uploadSpeedSpinBox->setValue(mTorrent->data().uploadSpeedLimit);
            priorityComboBox->setCurrentIndex(
                indexOfCasted<int>(priorityComboBoxItems, mTorrent->data().bandwidthPriority).value()
            );
            ratioLimitComboBox->setCurrentIndex(
                indexOfCasted<int>(ratioLimitComboBoxItems, mTorrent->data().ratioLimitMode).value()
            );
            ratioLimitSpinBox->setValue(mTorrent->data().ratioLimit);
            idleSeedingLimitComboBox->setCurrentIndex(
                indexOfCasted<int>(idleSeedingLimitComboBoxItems, mTorrent->data().idleSeedingLimitMode).value()
            );
            idleSeedingLimitSpinBox->setValue(mTorrent->data().idleSeedingLimit);
            peersLimitsSpinBox->setValue(mTorrent->data().peersLimit);

            mUpdatingLimits = false;
        };
    }

    void TorrentPropertiesDialog::setTorrent(Torrent* torrent) {
        if (torrent != mTorrent) {
            if (mTorrent) {
                QObject::disconnect(mTorrent, nullptr, this, nullptr);
            }
            mTorrent = torrent;
            onTorrentChanged();
        }
    }

    void TorrentPropertiesDialog::onTorrentChanged() {
        if (mTorrent) {
            mMessageWidget->animatedHide();

            for (int i = 0, count = mTabWidget->count(); i < count; i++) {
                mTabWidget->widget(i)->setEnabled(true);
            }

            mUpdateDetailsTab();
            mWebSeedersModel->setStringList(mTorrent->data().webSeeders);
            mUpdateLimitsTab();

            QObject::connect(mTorrent, &Torrent::changed, this, [this] {
                mUpdateDetailsTab();
                mWebSeedersModel->setStringList(mTorrent->data().webSeeders);
                mUpdateLimitsTab();
            });
        } else {
            if (mRpc->connectionState() == Rpc::ConnectionState::Disconnected) {
                //: Message that appears when disconnected from server
                mMessageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
            } else {
                //: Message that appears when torrent is removed
                mMessageWidget->setText(qApp->translate("tremotesf", "Torrent Removed"));
            }
            mMessageWidget->animatedShow();
            for (int i = 0, count = mTabWidget->count(); i < count; i++) {
                mTabWidget->widget(i)->setEnabled(false);
            }
            mWebSeedersModel->setStringList({});
        }

        mFilesModel->setTorrent(mTorrent);
        mTrackersViewWidget->setTorrent(mTorrent);
        mPeersModel->setTorrent(mTorrent);
    }
}
