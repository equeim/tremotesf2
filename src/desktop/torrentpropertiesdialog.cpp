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
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <KColumnResizer>
#include <KMessageWidget>

#include "../libtremotesf/torrent.h"
#include "../baseproxymodel.h"
#include "../peersmodel.h"
#include "../settings.h"
#include "../torrentfilesmodel.h"
#include "../trackersmodel.h"
#include "../trpc.h"
#include "../utils.h"
#include "commondelegate.h"
#include "torrentfilesview.h"
#include "trackersviewwidget.h"

namespace tremotesf
{
    using libtremotesf::Torrent;
    using libtremotesf::TorrentData;

    namespace
    {
        const TorrentData::Priority priorityComboBoxItems[] = {
            TorrentData::HighPriority,
            TorrentData::NormalPriority,
            TorrentData::LowPriority
        };

        const TorrentData::RatioLimitMode ratioLimitComboBoxItems[] = {
            TorrentData::GlobalRatioLimit,
            TorrentData::UnlimitedRatio,
            TorrentData::SingleRatioLimit
        };

        const TorrentData::IdleSeedingLimitMode idleSeedingLimitComboBoxItems[] = {
            TorrentData::GlobalIdleSeedingLimit,
            TorrentData::UnlimitedIdleSeeding,
            TorrentData::SingleIdleSeedingLimit
        };
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
          mPeersModel(nullptr)
    {
        auto layout = new QVBoxLayout(this);

        mMessageWidget->setCloseButtonVisible(false);
        mMessageWidget->setMessageType(KMessageWidget::Warning);
        mMessageWidget->hide();
        layout->addWidget(mMessageWidget);

        setupDetailsTab();
        mTabWidget->addTab(new TorrentFilesView(mFilesModel, mRpc), qApp->translate("tremotesf", "Files"));
        mTabWidget->addTab(mTrackersViewWidget, qApp->translate("tremotesf", "Trackers"));
        setupPeersTab();
        setupLimitsTab();

        layout->addWidget(mTabWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TorrentPropertiesDialog::reject);
        layout->addWidget(dialogButtonBox);

        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);

        const QString torrentHash(mTorrent->hashString());
        QObject::connect(mRpc, &Rpc::torrentsUpdated, this, [=] {
            setTorrent(mRpc->torrentByHash(torrentHash));
        });

        setMinimumSize(minimumSizeHint());
        restoreGeometry(Settings::instance()->torrentPropertiesDialogGeometry());
    }

    TorrentPropertiesDialog::~TorrentPropertiesDialog()
    {
        Settings::instance()->setPeersViewHeaderState(mPeersView->header()->saveState());
        Settings::instance()->setTorrentPropertiesDialogGeometry(saveGeometry());
    }

    QSize TorrentPropertiesDialog::sizeHint() const
    {
        return minimumSizeHint();
    }

    void TorrentPropertiesDialog::setupDetailsTab()
    {
        auto detailsTab = new QWidget(this);
        mTabWidget->addTab(detailsTab, qApp->translate("tremotesf", "Details"));

        auto detailsTabLayout = new QVBoxLayout(detailsTab);

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
        auto lastActivityLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Last activity:"), lastActivityLabel);
        detailsTabLayout->addWidget(activityGroupBox);

        auto infoGroupBox = new QGroupBox(qApp->translate("tremotesf", "Information"), this);
        auto infoGroupBoxLayout = new QFormLayout(infoGroupBox);
        auto totalSizeLabel = new QLabel(this);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Total size:"), totalSizeLabel);
        auto locationLabel = new QLabel(this);
        locationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Location:"), locationLabel);
        auto hashLabel = new QLabel(mTorrent->hashString(), this);
        hashLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Hash:"), hashLabel);
        auto creatorLabel = new QLabel(this);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Created by:"), creatorLabel);
        auto creationDateLabel = new QLabel(this);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Created on:"), creationDateLabel);
        auto commentTextEdit = new QPlainTextEdit(this);
        commentTextEdit->setReadOnly(true);
        infoGroupBoxLayout->addRow(qApp->translate("tremotesf", "Comment:"), commentTextEdit);
        detailsTabLayout->addWidget(infoGroupBox);

        auto resizer = new KColumnResizer(this);
        resizer->addWidgetsFromLayout(activityGroupBoxLayout);
        resizer->addWidgetsFromLayout(infoGroupBoxLayout);

        mUpdateDetailsTab = [=] {
            setWindowTitle(mTorrent->name());

            //: e.g. 100 MiB of 200 MiB (50%)
            completedLabel->setText(qApp->translate("tremotesf", "%1 of %2 (%3)")
                                        .arg(Utils::formatByteSize(mTorrent->completedSize()),
                                             Utils::formatByteSize(mTorrent->sizeWhenDone()),
                                             Utils::formatProgress(mTorrent->percentDone())));
            downloadedLabel->setText(Utils::formatByteSize(mTorrent->totalDownloaded()));
            uploadedLabel->setText(Utils::formatByteSize(mTorrent->totalUploaded()));
            ratioLabel->setText(Utils::formatRatio(mTorrent->ratio()));
            downloadSpeedLabel->setText(Utils::formatByteSpeed(mTorrent->downloadSpeed()));
            uploadSpeedLabel->setText(Utils::formatByteSpeed(mTorrent->uploadSpeed()));
            etaLabel->setText(Utils::formatEta(mTorrent->eta()));

            const QLocale locale;
            seedersLabel->setText(locale.toString(mTorrent->seeders()));
            leechersLabel->setText(locale.toString(mTorrent->leechers()));

            lastActivityLabel->setText(mTorrent->activityDate().toString());

            totalSizeLabel->setText(Utils::formatByteSize(mTorrent->totalSize()));
            locationLabel->setText(mTorrent->downloadDirectory());
            creatorLabel->setText(mTorrent->creator());
            creationDateLabel->setText(mTorrent->creationDate().toString());
            if (mTorrent->comment() != commentTextEdit->toPlainText()) {
                commentTextEdit->setPlainText(mTorrent->comment());
            }
        };

        mUpdateDetailsTab();
        QObject::connect(mTorrent, &Torrent::changed, this, mUpdateDetailsTab);
    }

    void TorrentPropertiesDialog::setupPeersTab()
    {
        mPeersModel = new PeersModel(mTorrent, this);
        auto peersProxyModel = new BaseProxyModel(mPeersModel, PeersModel::SortRole, this);

        auto peersTab = new QWidget(this);
        auto peersTabLayout = new QVBoxLayout(peersTab);

        mPeersView = new BaseTreeView(this);
        mPeersView->setItemDelegate(new CommonDelegate(PeersModel::ProgressBarColumn, PeersModel::SortRole, -1, this));
        mPeersView->setModel(peersProxyModel);
        mPeersView->setRootIsDecorated(false);
        mPeersView->header()->restoreState(Settings::instance()->peersViewHeaderState());

        peersTabLayout->addWidget(mPeersView);

        mTabWidget->addTab(peersTab, qApp->translate("tremotesf", "Peers"));
    }

    void TorrentPropertiesDialog::setupLimitsTab()
    {
        auto limitsTab = new QWidget(this);
        mTabWidget->addTab(limitsTab, qApp->translate("tremotesf", "Limits"));

        auto limitsTabLayout = new QVBoxLayout(limitsTab);

        //
        // Speed group box
        //
        auto speedGroupBox = new QGroupBox(qApp->translate("tremotesf", "Speed"), this);
        auto speedGroupBoxLayout = new QFormLayout(speedGroupBox);
        speedGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        speedGroupBoxLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        auto globalLimitsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Honor global limits"), this);
        speedGroupBoxLayout->addRow(globalLimitsCheckBox);

        const int maxSpeedLimit = std::numeric_limits<uint>::max() / 1024;

        auto downloadSpeedCheckBox = new QCheckBox(qApp->translate("tremotesf", "Download:"), this);
        speedGroupBoxLayout->addRow(downloadSpeedCheckBox);

        auto downloadSpeedSpinBoxLayout = new QHBoxLayout();
        speedGroupBoxLayout->addRow(downloadSpeedSpinBoxLayout);
        auto downloadSpeedSpinBox = new QSpinBox(this);
        downloadSpeedSpinBox->setEnabled(false);
        downloadSpeedSpinBox->setMaximum(maxSpeedLimit);
        //: In this context, 'k' prefix means SI prefix, i.e kB = 1000 bytes
        downloadSpeedSpinBox->setSuffix(qApp->translate("tremotesf", " kB/s"));

        QObject::connect(downloadSpeedCheckBox, &QCheckBox::toggled, downloadSpeedSpinBox, &QSpinBox::setEnabled);

        downloadSpeedSpinBoxLayout->addSpacing(28);
        downloadSpeedSpinBoxLayout->addWidget(downloadSpeedSpinBox);

        auto uploadSpeedCheckBox = new QCheckBox(qApp->translate("tremotesf", "Upload:"), this);
        speedGroupBoxLayout->addRow(uploadSpeedCheckBox);

        auto uploadSpeedSpinBoxLayout = new QHBoxLayout();
        speedGroupBoxLayout->addRow(uploadSpeedSpinBoxLayout);
        auto uploadSpeedSpinBox = new QSpinBox(this);
        uploadSpeedSpinBox->setEnabled(false);
        uploadSpeedSpinBox->setMaximum(maxSpeedLimit);
        //: In this context, 'k' prefix means SI prefix, i.e kB = 1000 bytes
        uploadSpeedSpinBox->setSuffix(qApp->translate("tremotesf", " kB/s"));

        QObject::connect(uploadSpeedCheckBox, &QCheckBox::toggled, uploadSpeedSpinBox, &QSpinBox::setEnabled);

        uploadSpeedSpinBoxLayout->addSpacing(28);
        uploadSpeedSpinBoxLayout->addWidget(uploadSpeedSpinBox);

        auto priorityComboBox = new QComboBox(this);
        for (TorrentData::Priority priority : priorityComboBoxItems) {
            switch (priority) {
            case TorrentData::HighPriority:
                //: Priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "High"));
                break;
            case TorrentData::NormalPriority:
                //: Priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "Normal"));
                break;
            case TorrentData::LowPriority:
                //: Priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "Low"));
                break;
            }
        }
        speedGroupBoxLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), priorityComboBox);

        limitsTabLayout->addWidget(speedGroupBox);

        //
        // Seeding group box
        //
        auto seedingGroupBox = new QGroupBox(qApp->translate("tremotesf", "Seeding", "Noun"), this);
        auto seedingGroupBoxLayout = new QFormLayout(seedingGroupBox);
        seedingGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto ratioLimitLayout = new QHBoxLayout();
        seedingGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio limit mode:"), ratioLimitLayout);
        ratioLimitLayout->setContentsMargins(0, 0, 0, 0);
        auto ratioLimitComboBox = new QComboBox(this);
        for (TorrentData::RatioLimitMode mode : ratioLimitComboBoxItems) {
            switch (mode) {
            case TorrentData::GlobalRatioLimit:
                ratioLimitComboBox->addItem(qApp->translate("tremotesf", "Use global settings"));
                break;
            case TorrentData::SingleRatioLimit:
                ratioLimitComboBox->addItem(qApp->translate("tremotesf", "Stop seeding at ratio:"));
                break;
            case TorrentData::UnlimitedRatio:
                ratioLimitComboBox->addItem(qApp->translate("tremotesf", "Seed regardless of ratio"));
                break;
            }
        }

        ratioLimitLayout->addWidget(ratioLimitComboBox);
        auto ratioLimitSpinBox = new QDoubleSpinBox(this);
        ratioLimitSpinBox->setMaximum(10000.0);
        ratioLimitSpinBox->setSingleStep(0.1);

        ratioLimitSpinBox->setVisible(false);
        QObject::connect(ratioLimitComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
                             if (index == index_of_i(ratioLimitComboBoxItems, TorrentData::SingleRatioLimit)) {
                                 ratioLimitSpinBox->show();
                             } else {
                                 ratioLimitSpinBox->hide();
                             }
                         });
        ratioLimitLayout->addWidget(ratioLimitSpinBox);

        auto idleSeedingLimitLayout = new QHBoxLayout();
        seedingGroupBoxLayout->addRow(qApp->translate("tremotesf", "Idle seeding mode:"), idleSeedingLimitLayout);
        idleSeedingLimitLayout->setContentsMargins(0, 0, 0, 0);
        auto idleSeedingLimitComboBox = new QComboBox(this);
        for (TorrentData::IdleSeedingLimitMode mode : idleSeedingLimitComboBoxItems) {
            switch (mode) {
            case TorrentData::GlobalIdleSeedingLimit:
                idleSeedingLimitComboBox->addItem(qApp->translate("tremotesf", "Use global settings"));
                break;
            case TorrentData::SingleIdleSeedingLimit:
                idleSeedingLimitComboBox->addItem(qApp->translate("tremotesf", "Stop seeding if idle for:"));
                break;
            case TorrentData::UnlimitedIdleSeeding:
                idleSeedingLimitComboBox->addItem(qApp->translate("tremotesf", "Seed regardless of activity"));
                break;
            }
        }

        idleSeedingLimitLayout->addWidget(idleSeedingLimitComboBox);
        auto idleSeedingLimitSpinBox = new QSpinBox(this);
        idleSeedingLimitSpinBox->setMaximum(9999);
        //: Minutes
        idleSeedingLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));

        idleSeedingLimitSpinBox->setVisible(false);
        QObject::connect(idleSeedingLimitComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
                             if (index == index_of_i(idleSeedingLimitComboBoxItems, TorrentData::SingleIdleSeedingLimit)) {
                                 idleSeedingLimitSpinBox->show();
                             } else {
                                 idleSeedingLimitSpinBox->hide();
                             }
                         });
        idleSeedingLimitLayout->addWidget(idleSeedingLimitSpinBox);

        limitsTabLayout->addWidget(seedingGroupBox);

        //
        // Peers group box
        //
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

        mUpdateLimitsTab = [=] {
            globalLimitsCheckBox->setChecked(mTorrent->honorSessionLimits());
            QObject::connect(globalLimitsCheckBox, &QCheckBox::toggled, mTorrent, &Torrent::setHonorSessionLimits);

            downloadSpeedCheckBox->setChecked(mTorrent->isDownloadSpeedLimited());
            QObject::connect(downloadSpeedCheckBox, &QCheckBox::toggled, mTorrent, &Torrent::setDownloadSpeedLimited);

            downloadSpeedSpinBox->setValue(mTorrent->downloadSpeedLimit());
            QObject::connect(downloadSpeedSpinBox,
                             static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                             mTorrent,
                             &Torrent::setDownloadSpeedLimit);

            uploadSpeedCheckBox->setChecked(mTorrent->isUploadSpeedLimited());
            QObject::connect(uploadSpeedCheckBox, &QCheckBox::toggled, mTorrent, &Torrent::setUploadSpeedLimited);

            uploadSpeedSpinBox->setValue(mTorrent->uploadSpeedLimit());
            QObject::connect(uploadSpeedSpinBox,
                             static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                             mTorrent,
                             &Torrent::setUploadSpeedLimit);

            priorityComboBox->setCurrentIndex(index_of_i(priorityComboBoxItems, mTorrent->bandwidthPriority()));
            QObject::connect(priorityComboBox,
                             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                             mTorrent,
                             [=](int index) {
                                 mTorrent->setBandwidthPriority(priorityComboBoxItems[index]);
                             });

            ratioLimitComboBox->setCurrentIndex(index_of_i(ratioLimitComboBoxItems, mTorrent->ratioLimitMode()));
            QObject::connect(ratioLimitComboBox,
                             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                             mTorrent,
                             [=](int index) {
                                 mTorrent->setRatioLimitMode(ratioLimitComboBoxItems[index]);
                             });

            ratioLimitSpinBox->setValue(mTorrent->ratioLimit());
            QObject::connect(ratioLimitSpinBox,
                             static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                             mTorrent,
                             &Torrent::setRatioLimit);

            idleSeedingLimitComboBox->setCurrentIndex(index_of_i(idleSeedingLimitComboBoxItems, mTorrent->idleSeedingLimitMode()));
            QObject::connect(idleSeedingLimitComboBox,
                             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                             mTorrent,
                             [=](int index) {
                                 mTorrent->setIdleSeedingLimitMode(idleSeedingLimitComboBoxItems[index]);
                             });

            idleSeedingLimitSpinBox->setValue(mTorrent->idleSeedingLimit());
            QObject::connect(idleSeedingLimitSpinBox,
                             static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                             mTorrent,
                             &Torrent::setIdleSeedingLimit);

            peersLimitsSpinBox->setValue(mTorrent->peersLimit());
            QObject::connect(peersLimitsSpinBox,
                             static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                             mTorrent,
                             &Torrent::setPeersLimit);
        };

        mUpdateLimitsTab();
    }

    void TorrentPropertiesDialog::setTorrent(Torrent* torrent)
    {
        if (torrent != mTorrent) {
            mTorrent = torrent;

            if (mTorrent) {
                mMessageWidget->animatedHide();

                for (int i = 0, count = mTabWidget->count(); i < count; i++) {
                    mTabWidget->widget(i)->setEnabled(true);
                }

                QObject::connect(mTorrent, &Torrent::changed, this, mUpdateDetailsTab);
                mUpdateDetailsTab();

                mUpdateLimitsTab();
            } else {
                if (mRpc->connectionState() == Rpc::ConnectionState::Disconnected) {
                    mMessageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
                } else {
                    mMessageWidget->setText(qApp->translate("tremotesf", "Torrent Removed"));
                }
                mMessageWidget->animatedShow();

                for (int i = 0, count = mTabWidget->count(); i < count; i++) {
                    mTabWidget->widget(i)->setEnabled(false);
                }
            }

            mFilesModel->setTorrent(mTorrent);
            mTrackersViewWidget->setTorrent(mTorrent);
            mPeersModel->setTorrent(mTorrent);
        }
    }
}
