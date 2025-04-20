// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentpropertieswidget.h"

#include <fmt/format.h>
#include <limits>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QLocale>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <KMessageWidget>
#include <KColumnResizer>

#include "desktoputils.h"
#include "formatutils.h"
#include "peersmodel.h"
#include "settings.h"
#include "stdutils.h"
#include "torrentfilesmodel.h"
#include "trackersviewwidget.h"
#include "rpc/pathutils.h"
#include "rpc/rpc.h"
#include "rpc/serversettings.h"
#include "rpc/torrent.h"
#include "ui/itemmodels/baseproxymodel.h"
#include "ui/itemmodels/stringlistmodel.h"
#include "ui/stylehelpers.h"
#include "ui/widgets/progressbardelegate.h"
#include "ui/widgets/tooltipwhenelideddelegate.h"
#include "ui/widgets/torrentfilesview.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        constexpr TorrentData::Priority priorityComboBoxItems[] = {
            TorrentData::Priority::High, TorrentData::Priority::Normal, TorrentData::Priority::Low
        };

        constexpr TorrentData::RatioLimitMode ratioLimitComboBoxItems[] = {
            TorrentData::RatioLimitMode::Global,
            TorrentData::RatioLimitMode::Unlimited,
            TorrentData::RatioLimitMode::Single
        };

        constexpr TorrentData::IdleSeedingLimitMode idleSeedingLimitComboBoxItems[] = {
            TorrentData::IdleSeedingLimitMode::Global,
            TorrentData::IdleSeedingLimitMode::Unlimited,
            TorrentData::IdleSeedingLimitMode::Single
        };
    }

    TorrentPropertiesWidget::TorrentPropertiesWidget(Rpc* rpc, bool horizontalDetails, QWidget* parent)
        : QTabWidget(parent),
          mRpc(rpc),
          mFilesModel(new TorrentFilesModel(mRpc, this)),
          mFilesView(new TorrentFilesView(mFilesModel, mRpc, this)),
          mTrackersViewWidget(new TrackersViewWidget(mRpc, this)) {
        setEnabled(false);

        setupDetailsTab(horizontalDetails);

        auto filesTab = new QWidget(this);
        auto filesTabLayout = new QVBoxLayout(filesTab);
        filesTabLayout->addWidget(mFilesView);
        overrideBreezeFramelessScrollAreaHeuristic(mFilesView, true);
        //: Torrent properties dialog tab
        addTab(filesTab, qApp->translate("tremotesf", "Files"));

        //: Torrent properties dialog tab
        addTab(mTrackersViewWidget, qApp->translate("tremotesf", "Trackers"));

        setupPeersTab();
        setupWebSeedersTab();
        setupLimitsTab();
    }

    void TorrentPropertiesWidget::saveState() {
        Settings::instance()->set_peersViewHeaderState(mPeersView->header()->saveState());
        mFilesView->saveState();
        mTrackersViewWidget->saveState();
    }

    void TorrentPropertiesWidget::setupDetailsTab(bool horizontal) {
        auto detailsTab = new QScrollArea(this);
        detailsTab->setWidgetResizable(true);
        makeScrollAreaTransparent(detailsTab);

        if (!horizontal) {
            detailsTab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        //: Torrent's properties dialog tab
        addTab(detailsTab, qApp->translate("tremotesf", "Details"));

        auto detailsTabScrollContent = new QWidget(detailsTab);
        detailsTab->setWidget(detailsTabScrollContent);

        QBoxLayout* detailsTabLayout{};
        if (horizontal) {
            detailsTabLayout = new QHBoxLayout(detailsTabScrollContent);
        } else {
            detailsTabLayout = new QVBoxLayout(detailsTabScrollContent);
        }

        //: Torrent's details tab section
        auto activityGroupBox = new QGroupBox(qApp->translate("tremotesf", "Activity"), this);
        auto activityGroupBoxLayout = new QFormLayout(activityGroupBox);
        activityGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
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
        infoGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        auto totalSizeLabel = new QLabel(this);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Total size:"), totalSizeLabel);
        auto locationLabel = new QLabel(this);
        locationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        //: Torrent's download directory
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Location:"), locationLabel);
        auto hashLabel = new QLabel(this);
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
        commentTextEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        //: Torrent's comment text
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Comment:"), commentTextEdit);

        auto labelsModel = new StringListModel({}, QIcon::fromTheme("tag"_L1), this);
        auto labelsProxyModel = new BaseProxyModel(labelsModel, Qt::DisplayRole, std::nullopt, this);
        auto labelsView = new QListView(this);
        labelsView->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        labelsView->setFlow(QListView::LeftToRight);
        labelsView->setWrapping(true);
        labelsView->setResizeMode(QListView::Adjust);
        labelsView->setIconSize(QSize(16, 16));
        labelsView->setModel(labelsProxyModel);
        //: Torrent's labels
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Labels:"), labelsView);
        auto labelsLabel = infoGroupBoxLayout->labelForField(labelsView);

        detailsTabLayout->addWidget(infoGroupBox);

        if (!horizontal) {
            auto resizer = new KColumnResizer(this);
            resizer->addWidgetsFromLayout(activityGroupBoxLayout);
            resizer->addWidgetsFromLayout(infoGroupBoxLayout);
        }

        mUpdateDetailsTab = [=, this] {
            if (mTorrent) {
                //: Torrent's completion size, e.g. 100 MiB of 200 MiB (50%). %1 is completed size, %2 is size, %3 is progress in percents
                completedLabel->setText(qApp->translate("tremotesf", "%1 of %2 (%3)")
                                            .arg(
                                                formatutils::formatByteSize(mTorrent->data().completedSize),
                                                formatutils::formatByteSize(mTorrent->data().sizeWhenDone),
                                                formatutils::formatProgress(mTorrent->data().percentDone)
                                            ));
                downloadedLabel->setText(formatutils::formatByteSize(mTorrent->data().totalDownloaded));
                uploadedLabel->setText(formatutils::formatByteSize(mTorrent->data().totalUploaded));
                ratioLabel->setText(formatutils::formatRatio(mTorrent->data().ratio));
                downloadSpeedLabel->setText(formatutils::formatByteSpeed(mTorrent->data().downloadSpeed));
                uploadSpeedLabel->setText(formatutils::formatByteSpeed(mTorrent->data().uploadSpeed));
                etaLabel->setText(formatutils::formatEta(mTorrent->data().eta));

                const QLocale locale{};
                seedersLabel->setText(locale.toString(mTorrent->data().totalSeedersFromTrackersCount));
                leechersLabel->setText(locale.toString(mTorrent->data().totalLeechersFromTrackersCount));
                peersSendingToUsLabel->setText(locale.toString(mTorrent->data().peersSendingToUsCount));
                webSeedersSendingToUsLabel->setText(locale.toString(mTorrent->data().webSeedersSendingToUsCount));
                peersGettingFromUsLabel->setText(locale.toString(mTorrent->data().peersGettingFromUsCount));

                lastActivityLabel->setText(
                    formatutils::formatDateTime(mTorrent->data().activityDate.toLocalTime(), QLocale::LongFormat)
                );

                totalSizeLabel->setText(formatutils::formatByteSize(mTorrent->data().totalSize));
                locationLabel->setText(
                    toNativeSeparators(mTorrent->data().downloadDirectory, mRpc->serverSettings()->data().pathOs)
                );
                hashLabel->setText(mTorrent->data().hashString);
                creatorLabel->setText(mTorrent->data().creator);
                creationDateLabel->setText(
                    formatutils::formatDateTime(mTorrent->data().creationDate.toLocalTime(), QLocale::LongFormat)
                );
                if (mTorrent->data().comment != commentTextEdit->toPlainText()) {
                    commentTextEdit->document()->setPlainText(mTorrent->data().comment);
                    desktoputils::findLinksAndAddAnchors(commentTextEdit->document());
                }
                labelsModel->setStringList(mTorrent->data().labels);
            } else {
                completedLabel->clear();
                downloadedLabel->clear();
                uploadedLabel->clear();
                ratioLabel->clear();
                downloadSpeedLabel->clear();
                uploadSpeedLabel->clear();
                etaLabel->clear();
                seedersLabel->clear();
                leechersLabel->clear();
                peersSendingToUsLabel->clear();
                webSeedersSendingToUsLabel->clear();
                peersGettingFromUsLabel->clear();
                lastActivityLabel->clear();
                totalSizeLabel->clear();
                locationLabel->clear();
                hashLabel->clear();
                creatorLabel->clear();
                creationDateLabel->clear();
                commentTextEdit->clear();
                labelsModel->setStringList({});
            }
            const bool labelsViewVisible = labelsModel->rowCount() > 0;
            labelsView->setVisible(labelsViewVisible);
            labelsLabel->setVisible(labelsViewVisible);
        };

        QObject::connect(Settings::instance(), &Settings::displayRelativeTimeChanged, this, mUpdateDetailsTab);
    }

    void TorrentPropertiesWidget::setupPeersTab() {
        mPeersModel = new PeersModel(this);
        auto peersProxyModel =
            new BaseProxyModel(mPeersModel, PeersModel::SortRole, static_cast<int>(PeersModel::Column::Address), this);

        auto peersTab = new QWidget(this);
        auto peersTabLayout = new QVBoxLayout(peersTab);

        mPeersView = new BaseTreeView(this);
        mPeersView->setItemDelegate(new TooltipWhenElidedDelegate(this));
        mPeersView->setItemDelegateForColumn(
            static_cast<int>(PeersModel::Column::ProgressBar),
            new ProgressBarDelegate(PeersModel::SortRole, this)
        );
        mPeersView->setModel(peersProxyModel);
        mPeersView->setRootIsDecorated(false);
        mPeersView->header()->restoreState(Settings::instance()->get_peersViewHeaderState());
        overrideBreezeFramelessScrollAreaHeuristic(mPeersView, true);

        peersTabLayout->addWidget(mPeersView);
        //: Torrent's properties dialog tab
        addTab(peersTab, qApp->translate("tremotesf", "Peers"));
    }

    void TorrentPropertiesWidget::setupWebSeedersTab() {
        //: Web seeders list column title
        mWebSeedersModel = new StringListModel(qApp->translate("tremotesf", "Web seeder"), {}, this);
        auto webSeedersProxyModel = new BaseProxyModel(mWebSeedersModel, Qt::DisplayRole, std::nullopt, this);

        auto webSeedersTab = new QWidget(this);
        auto webSeedersTabLayout = new QVBoxLayout(webSeedersTab);

        auto webSeedersView = new BaseTreeView(this);
        webSeedersView->header()->setContextMenuPolicy(Qt::DefaultContextMenu);
        webSeedersView->setModel(webSeedersProxyModel);
        webSeedersView->setRootIsDecorated(false);
        overrideBreezeFramelessScrollAreaHeuristic(webSeedersView, true);

        webSeedersTabLayout->addWidget(webSeedersView);

        //: Torrent's properties dialog tab
        addTab(webSeedersTab, qApp->translate("tremotesf", "Web seeders"));
    }

    void TorrentPropertiesWidget::setupLimitsTab() {
        auto limitsTab = new QScrollArea(this);
        limitsTab->setWidgetResizable(true);
        limitsTab->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        makeScrollAreaTransparent(limitsTab);
        //: Torrent's properties dialog tab
        addTab(limitsTab, qApp->translate("tremotesf", "Limits"));

        auto limitsTabScrollContent = new QWidget(limitsTab);
        limitsTab->setWidget(limitsTabScrollContent);

        auto limitsTabLayout = new QVBoxLayout(limitsTabScrollContent);

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
                if (index
                    == indexOfCasted<int>(idleSeedingLimitComboBoxItems, TorrentData::IdleSeedingLimitMode::Single)) {
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

            if (mTorrent) {
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
            } else {
                globalLimitsCheckBox->setChecked(false);
                downloadSpeedCheckBox->setChecked(false);
                downloadSpeedSpinBox->clear();
                uploadSpeedCheckBox->setChecked(false);
                uploadSpeedSpinBox->clear();
                priorityComboBox->setCurrentIndex(0);
                ratioLimitComboBox->setCurrentIndex(0);
                ratioLimitSpinBox->clear();
                idleSeedingLimitComboBox->setCurrentIndex(0);
                idleSeedingLimitSpinBox->clear();
                peersLimitsSpinBox->clear();
            }

            mUpdatingLimits = false;
        };
    }

    void TorrentPropertiesWidget::setTorrent(Torrent* torrent) { setTorrent(torrent, false); }

    void TorrentPropertiesWidget::setTorrent(Torrent* torrent, bool oldTorrentDestroyed) {
        if (torrent == mTorrent) {
            return;
        }
        if (mTorrent && !oldTorrentDestroyed) {
            QObject::disconnect(mTorrent, nullptr, this, nullptr);
        }

        const bool hadTorrent = mTorrent != nullptr;
        mTorrent = torrent;

        const auto update = [this] {
            mUpdateDetailsTab();
            if (mTorrent) {
                mWebSeedersModel->setStringList(mTorrent->data().webSeeders);
            } else {
                mWebSeedersModel->setStringList({});
            }
            mUpdateLimitsTab();
        };
        update();
        if (mTorrent) {
            QObject::connect(mTorrent, &Torrent::changed, this, update);
            QObject::connect(mTorrent, &QObject::destroyed, this, [this] { setTorrent(nullptr, true); });
        }
        mFilesModel->setTorrent(mTorrent, oldTorrentDestroyed);
        mTrackersViewWidget->setTorrent(mTorrent, oldTorrentDestroyed);
        mPeersModel->setTorrent(mTorrent, oldTorrentDestroyed);

        const auto onHasTorrentChanged = [this](bool hasTorrent) {
            setEnabled(hasTorrent);
            emit hasTorrentChanged(hasTorrent);
        };
        if (hadTorrent && !mTorrent) {
            onHasTorrentChanged(false);
        } else if (!hadTorrent && mTorrent) {
            onHasTorrentChanged(true);
        }
    }
}
