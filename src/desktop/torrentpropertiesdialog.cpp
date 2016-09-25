/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "../baseproxymodel.h"
#include "../peersmodel.h"
#include "../rpc.h"
#include "../settings.h"
#include "../torrent.h"
#include "../utils.h"
#include "commondelegate.h"
#include "torrentfilesview.h"
#include "trackersviewwidget.h"

namespace tremotesf
{
    TorrentPropertiesDialog::TorrentPropertiesDialog(Torrent* torrent, Rpc* rpc, QWidget* parent)
        : QDialog(parent),
          mTorrent(torrent),
          mRpc(rpc)
    {
        auto layout = new QVBoxLayout(this);

        auto messageWidget = new KMessageWidget(this);
        messageWidget->setCloseButtonVisible(false);
        messageWidget->setMessageType(KMessageWidget::Warning);
        messageWidget->hide();
        layout->addWidget(messageWidget);

        auto tabWidget = new QTabWidget(this);
        setupDetailsTab(tabWidget);
        tabWidget->addTab(new TorrentFilesView(mTorrent), qApp->translate("tremotesf", "Files"));
        tabWidget->addTab(new TrackersViewWidget(mTorrent), qApp->translate("tremotesf", "Trackers"));
        setupPeersTab(tabWidget);
        setupLimitsTab(tabWidget);
        layout->addWidget(tabWidget);

        auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
        QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &TorrentPropertiesDialog::reject);
        layout->addWidget(dialogButtonBox);

        QObject::connect(mTorrent, &Torrent::destroyed, this, [=]() {
            if (mRpc->status() == Rpc::Disconnected) {
                messageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
            } else {
                messageWidget->setText(qApp->translate("tremotesf", "Torrent Removed"));
            }
            messageWidget->animatedShow();
        });

        dialogButtonBox->button(QDialogButtonBox::Close)->setDefault(true);
    }

    TorrentPropertiesDialog::~TorrentPropertiesDialog()
    {
        Settings::instance()->setPeersViewHeaderState(mPeersView->header()->saveState());
    }

    QSize TorrentPropertiesDialog::sizeHint() const
    {
        return layout()->totalMinimumSize();
    }

    void TorrentPropertiesDialog::setupDetailsTab(QTabWidget* tabWidget)
    {
        auto detailsTab = new QWidget(this);
        tabWidget->addTab(detailsTab, qApp->translate("tremotesf", "Details"));

        auto detailsTabLayout = new QVBoxLayout(detailsTab);

        auto activityGroupBox = new QGroupBox(qApp->translate("tremotesf", "Activity"), this);
        auto activityGroupBoxLayout = new QFormLayout(activityGroupBox);
        auto completedLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Completed:"), completedLabel);
        auto downloadedLabel = new QLabel(this);
        activityGroupBoxLayout->addRow(qApp->translate("tremotesf", "Downloaded:"), downloadedLabel);
        auto uploadedLabel = new QLabel(this);
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

        auto update = [=]() {
            setWindowTitle(mTorrent->name());

            completedLabel->setText(qApp->translate("tremotesf", "%1 of %2 (%3)")
                                    .arg(Utils::formatByteSize(mTorrent->completedSize()))
                                    .arg(Utils::formatByteSize(mTorrent->sizeWhenDone()))
                                    .arg(Utils::formatProgress(mTorrent->percentDone())));
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

        update();
        QObject::connect(mTorrent, &Torrent::updated, this, update);
    }

    void TorrentPropertiesDialog::setupPeersTab(QTabWidget* tabWidget)
    {
        auto peersModel = new PeersModel(mTorrent, this);
        auto peersProxyModel = new BaseProxyModel(peersModel, PeersModel::SortRole, this);

        auto peersTab = new QWidget(this);
        auto peersTabLayout = new QVBoxLayout(peersTab);

        mPeersView = new BaseTreeView(this);
        mPeersView->setItemDelegate(new CommonDelegate(PeersModel::ProgressBarColumn, PeersModel::SortRole, this));
        mPeersView->setModel(peersProxyModel);
        mPeersView->setRootIsDecorated(false);
        mPeersView->header()->restoreState(Settings::instance()->peersViewHeaderState());

        peersTabLayout->addWidget(mPeersView);

        tabWidget->addTab(peersTab, qApp->translate("tremotesf", "Peers"));
    }

    void TorrentPropertiesDialog::setupLimitsTab(QTabWidget* tabWidget)
    {
        auto limitsTab = new QWidget(this);
        tabWidget->addTab(limitsTab, qApp->translate("tremotesf", "Limits"));

        auto limitsTabLayout = new QVBoxLayout(limitsTab);

        //
        // Speed group box
        //
        auto speedGroupBox = new QGroupBox(qApp->translate("tremotesf", "Speed"), this);
        auto speedGroupBoxLayout = new QFormLayout(speedGroupBox);
        speedGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        speedGroupBoxLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        auto globalLimitsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Honor global limits"), this);
        globalLimitsCheckBox->setChecked(mTorrent->honorSessionLimits());
        QObject::connect(globalLimitsCheckBox, &QCheckBox::toggled, mTorrent, &Torrent::setHonorSessionLimits);
        speedGroupBoxLayout->addRow(globalLimitsCheckBox);

        const int maxSpeedLimit = std::numeric_limits<uint>::max() / 1024;

        auto downloadSpeedCheckBox = new QCheckBox(qApp->translate("tremotesf", "Download:"), this);
        downloadSpeedCheckBox->setChecked(mTorrent->isDownloadSpeedLimited());
        QObject::connect(downloadSpeedCheckBox, &QCheckBox::toggled, mTorrent, &Torrent::setDownloadSpeedLimited);
        speedGroupBoxLayout->addRow(downloadSpeedCheckBox);

        auto downloadSpeedSpinBoxLayout = new QHBoxLayout();
        speedGroupBoxLayout->addRow(downloadSpeedSpinBoxLayout);
        auto downloadSpeedSpinBox = new QSpinBox(this);
        downloadSpeedSpinBox->setEnabled(downloadSpeedCheckBox->isChecked());
        downloadSpeedSpinBox->setMaximum(maxSpeedLimit);
        downloadSpeedSpinBox->setSuffix(qApp->translate("tremotesf", " KiB/s"));
        downloadSpeedSpinBox->setValue(mTorrent->downloadSpeedLimit());
        QObject::connect(downloadSpeedCheckBox, &QCheckBox::toggled, downloadSpeedSpinBox, &QSpinBox::setEnabled);
        QObject::connect(downloadSpeedSpinBox,
                         static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                         mTorrent,
                         &Torrent::setDownloadSpeedLimit);
        downloadSpeedSpinBoxLayout->addSpacing(28);
        downloadSpeedSpinBoxLayout->addWidget(downloadSpeedSpinBox);

        auto uploadSpeedCheckBox = new QCheckBox(qApp->translate("tremotesf", "Upload:"), this);
        uploadSpeedCheckBox->setChecked(mTorrent->isUploadSpeedLimited());
        QObject::connect(uploadSpeedCheckBox, &QCheckBox::toggled, mTorrent, &Torrent::setUploadSpeedLimited);
        speedGroupBoxLayout->addRow(uploadSpeedCheckBox);

        auto uploadSpeedSpinBoxLayout = new QHBoxLayout();
        speedGroupBoxLayout->addRow(uploadSpeedSpinBoxLayout);
        auto uploadSpeedSpinBox = new QSpinBox(this);
        uploadSpeedSpinBox->setEnabled(uploadSpeedCheckBox->isChecked());
        uploadSpeedSpinBox->setMaximum(maxSpeedLimit);
        uploadSpeedSpinBox->setSuffix(qApp->translate("tremotesf", " KiB/s"));
        uploadSpeedSpinBox->setValue(mTorrent->uploadSpeedLimit());
        QObject::connect(uploadSpeedCheckBox, &QCheckBox::toggled, uploadSpeedSpinBox, &QSpinBox::setEnabled);
        QObject::connect(uploadSpeedSpinBox,
                         static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                         mTorrent,
                         &Torrent::setUploadSpeedLimit);
        uploadSpeedSpinBoxLayout->addSpacing(28);
        uploadSpeedSpinBoxLayout->addWidget(uploadSpeedSpinBox);

        auto priorityComboBox = new QComboBox(this);
        priorityComboBox->addItems({qApp->translate("tremotesf", "High"),
                                    qApp->translate("tremotesf", "Normal"),
                                    qApp->translate("tremotesf", "Low")});
        priorityComboBox->setCurrentIndex(1 - mTorrent->bandwidthPriority());
        QObject::connect(priorityComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
            mTorrent->setBandwidthPriority(static_cast<Torrent::Priority>(1 - index));
        });
        speedGroupBoxLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), priorityComboBox);

        limitsTabLayout->addWidget(speedGroupBox);

        //
        // Seeding group box
        //
        auto seedingGroupBox = new QGroupBox(qApp->translate("tremotesf", "Seeding"), this);
        auto seedingGroupBoxLayout = new QFormLayout(seedingGroupBox);
        seedingGroupBoxLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

        auto ratioLimitLayout = new QHBoxLayout();
        seedingGroupBoxLayout->addRow(qApp->translate("tremotesf", "Ratio limit mode:"), ratioLimitLayout);
        ratioLimitLayout->setContentsMargins(0, 0, 0, 0);
        auto ratioLimitComboBox = new QComboBox(this);
        ratioLimitComboBox->addItems({qApp->translate("tremotesf", "Use global settings"),
                                      qApp->translate("tremotesf", "Seed regardless of ratio"),
                                      qApp->translate("tremotesf", "Stop seeding at ratio:")});
        switch (mTorrent->ratioLimitMode()) {
        case Torrent::GlobalRatioLimit:
            ratioLimitComboBox->setCurrentIndex(0);
            break;
        case Torrent::SingleRatioLimit:
            ratioLimitComboBox->setCurrentIndex(2);
            break;
        case Torrent::UnlimitedRatio:
            ratioLimitComboBox->setCurrentIndex(1);
        }
        //ratioLimitComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QObject::connect(ratioLimitComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
            switch (index) {
            case 0:
                mTorrent->setRatioLimitMode(Torrent::GlobalRatioLimit);
                break;
            case 1:
                mTorrent->setRatioLimitMode(Torrent::UnlimitedRatio);
                break;
            case 2:
                mTorrent->setRatioLimitMode(Torrent::SingleRatioLimit);
            }
        });
        ratioLimitLayout->addWidget(ratioLimitComboBox);
        auto ratioLimitSpinBox = new QDoubleSpinBox(this);
        ratioLimitSpinBox->setMaximum(10000.0);
        ratioLimitSpinBox->setSingleStep(0.1);
        ratioLimitSpinBox->setValue(mTorrent->ratioLimit());
        ratioLimitSpinBox->setVisible(ratioLimitComboBox->currentIndex() == 2);
        QObject::connect(ratioLimitSpinBox,
                         static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                         mTorrent,
                         &Torrent::setRatioLimit);
        QObject::connect(ratioLimitComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
            if (index == 2) {
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
        idleSeedingLimitComboBox->addItems({qApp->translate("tremotesf", "Use global settings"),
                                            qApp->translate("tremotesf", "Seed regardless of activity"),
                                            qApp->translate("tremotesf", "Stop seeding if idle for:")});
        switch (mTorrent->idleSeedingLimitMode()) {
        case Torrent::GlobalIdleSeedingLimit:
            idleSeedingLimitComboBox->setCurrentIndex(0);
            break;
        case Torrent::SingleIdleSeedingLimit:
            idleSeedingLimitComboBox->setCurrentIndex(2);
            break;
        case Torrent::UnlimitedIdleSeeding:
            idleSeedingLimitComboBox->setCurrentIndex(1);
        }
        //idleSeedingLimitComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QObject::connect(idleSeedingLimitComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
            switch (index) {
            case 0:
                mTorrent->setIdleSeedingLimitMode(Torrent::GlobalIdleSeedingLimit);
                break;
            case 1:
                mTorrent->setIdleSeedingLimitMode(Torrent::UnlimitedIdleSeeding);
                break;
            case 2:
                mTorrent->setIdleSeedingLimitMode(Torrent::SingleIdleSeedingLimit);
            }
        });
        idleSeedingLimitLayout->addWidget(idleSeedingLimitComboBox);
        auto idleSeedingLimitSpinBox = new QSpinBox(this);
        idleSeedingLimitSpinBox->setMaximum(9999);
        idleSeedingLimitSpinBox->setSuffix(qApp->translate("tremotesf", " min"));
        idleSeedingLimitSpinBox->setValue(mTorrent->idleSeedingLimit());
        idleSeedingLimitSpinBox->setVisible(idleSeedingLimitComboBox->currentIndex() == 2);
        QObject::connect(idleSeedingLimitSpinBox,
                         static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                         mTorrent,
                         &Torrent::setIdleSeedingLimit);
        QObject::connect(idleSeedingLimitComboBox,
                         static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                         this,
                         [=](int index) {
            if (index == 2) {
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
        //peersLimitsSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        peersLimitsSpinBox->setMaximum(9999);
        peersLimitsSpinBox->setValue(mTorrent->peersLimit());
        QObject::connect(peersLimitsSpinBox,
                         static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                         mTorrent,
                         &Torrent::setPeersLimit);
        peersGroupBoxLayout->addRow(qApp->translate("tremotesf", "Maximum peers:"), peersLimitsSpinBox);

        limitsTabLayout->addWidget(peersGroupBox);

        limitsTabLayout->addStretch();

        auto resizer = new KColumnResizer(this);
        resizer->addWidgetsFromLayout(speedGroupBoxLayout);
        resizer->addWidgetsFromLayout(seedingGroupBoxLayout);
        resizer->addWidgetsFromLayout(peersGroupBoxLayout);
    }
}
