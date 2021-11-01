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

#include "mainwindowsidebar.h"

#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaEnum>
#include <QStyle>
#include <QVBoxLayout>
#include <QShortcut>

#include "../alltrackersmodel.h"
#include "../downloaddirectoriesmodel.h"
#include "../statusfilterstats.h"
#include "../torrentsproxymodel.h"
#include "../trpc.h"
#include "../utils.h"
#include "commondelegate.h"
#include "desktoputils.h"
#include "settings.h"

namespace tremotesf
{
    namespace
    {
        class StatusListWidget final : public QListWidget
        {
            Q_OBJECT
        public:
            StatusListWidget(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent = nullptr)
                : QListWidget(parent),
                  mTorrentsProxyModel(proxyModel),
                  mRpc(rpc),
                  mStats(new StatusFilterStats(mRpc, this))
            {
                setFrameShape(QFrame::NoFrame);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                setIconSize(QSize(16, 16));

                using namespace desktoputils;
                addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_DirIcon), {}, this, statusFilterToItemType(TorrentsProxyModel::All)));
                addItem(new QListWidgetItem(QIcon(statusIconPath(ActiveIcon)), {}, this, statusFilterToItemType(TorrentsProxyModel::Active)));
                addItem(new QListWidgetItem(QIcon(statusIconPath(DownloadingIcon)), {}, this, statusFilterToItemType(TorrentsProxyModel::Downloading)));
                addItem(new QListWidgetItem(QIcon(statusIconPath(SeedingIcon)), {}, this, statusFilterToItemType(TorrentsProxyModel::Seeding)));
                addItem(new QListWidgetItem(QIcon(statusIconPath(PausedIcon)), {}, this, statusFilterToItemType(TorrentsProxyModel::Paused)));
                addItem(new QListWidgetItem(QIcon(statusIconPath(CheckingIcon)), {}, this, statusFilterToItemType(TorrentsProxyModel::Checking)));
                addItem(new QListWidgetItem(QIcon(statusIconPath(ErroredIcon)), {}, this, statusFilterToItemType(TorrentsProxyModel::Errored)));
                setCurrentItem(itemForStatusFilter(mTorrentsProxyModel->statusFilter()));

                updateItems();
                QObject::connect(mStats, &StatusFilterStats::updated, this, &StatusListWidget::updateItems);

                QObject::connect(this, &StatusListWidget::currentItemChanged, this, [this](QListWidgetItem* current, auto) {
                    mTorrentsProxyModel->setStatusFilter(itemTypeToStatusFilter(current->type()));
                });
            }

            QSize minimumSizeHint() const override
            {
                return QSize(8, 0);
            }

            QSize sizeHint() const override
            {
                int height = 0;
                for (int i = 0, max = count(); i < max; ++i) {
                    height += sizeHintForRow(i);
                    height += spacing();
                }
                height += spacing();

                return QSize(sizeHintForColumn(0), height);
            }

            void updateItems()
            {
                itemForStatusFilter(TorrentsProxyModel::All)->setText(qApp->translate("tremotesf", "All (%L1)", "All torrents, %L1 - torrents count").arg(mRpc->torrentsCount()));

                //: Active torrents, %L1 - torrents count
                itemForStatusFilter(TorrentsProxyModel::Active)->setText(qApp->translate("tremotesf", "Active (%L1)").arg(mStats->activeTorrents()));

                //: Downloading torrents, %L1 - torrents count
                itemForStatusFilter(TorrentsProxyModel::Downloading)->setText(qApp->translate("tremotesf", "Downloading (%L1)").arg(mStats->downloadingTorrents()));

                //: Seeding torrents, %L1 - torrents count
                itemForStatusFilter(TorrentsProxyModel::Seeding)->setText(qApp->translate("tremotesf", "Seeding (%L1)").arg(mStats->seedingTorrents()));

                //: Paused torrents, %L1 - torrents count
                itemForStatusFilter(TorrentsProxyModel::Paused)->setText(qApp->translate("tremotesf", "Paused (%L1)").arg(mStats->pausedTorrents()));

                //: Checking torrents, %L1 - torrents count
                itemForStatusFilter(TorrentsProxyModel::Checking)->setText(qApp->translate("tremotesf", "Checking (%L1)").arg(mStats->checkingTorrents()));

                //: Errored torrents, %L1 - torrents count
                itemForStatusFilter(TorrentsProxyModel::Errored)->setText(qApp->translate("tremotesf", "Errored (%L1)").arg(mStats->erroredTorrents()));
            }

            void setVisibleExplicitly(bool visible)
            {
                setVisible(visible);
                mTorrentsProxyModel->setStatusFilterEnabled(visible);
            }

        private:
            static int statusFilterToItemType(TorrentsProxyModel::StatusFilter filter) {
                return static_cast<int>(QListWidgetItem::UserType) + static_cast<int>(filter);
            }

            static TorrentsProxyModel::StatusFilter itemTypeToStatusFilter(int type) {
                return static_cast<TorrentsProxyModel::StatusFilter>(type - static_cast<int>(QListWidgetItem::UserType));
            }

            QListWidgetItem* itemForStatusFilter(TorrentsProxyModel::StatusFilter filter) {
                const int type = statusFilterToItemType(filter);
                for (int i = 0, max = count(); i < max; ++i) {
                    auto item = this->item(i);
                    if (item->type() == type) {
                        return item;
                    }
                }
                qFatal("No item in list for status filter %s", QMetaEnum::fromType<TorrentsProxyModel::StatusFilter>().valueToKey(filter));
                return nullptr;
            }

            TorrentsProxyModel* mTorrentsProxyModel;
            Rpc* mRpc;
            StatusFilterStats* mStats;
        };

        class BaseListView : public QListView
        {
            Q_OBJECT

        public:
            explicit BaseListView(TorrentsProxyModel* torrentsProxyModel, QWidget* parent = nullptr)
                : QListView(parent),
                  mTorrentsProxyModel(torrentsProxyModel)
            {
                setFrameShape(QFrame::NoFrame);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setIconSize(QSize(16, 16));
                setItemDelegate(new CommonDelegate(this));
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                setTextElideMode(Qt::ElideMiddle);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }

            QSize minimumSizeHint() const override
            {
                return QSize(8, 0);
            }

            QSize sizeHint() const override
            {
                int height = 0;
                for (int i = 0, max = model()->rowCount(); i < max; ++i) {
                    height += sizeHintForRow(i);
                    height += spacing();
                }
                height += spacing();

                return QSize(sizeHintForColumn(0), height);
            }

            void setFilterEnabled(bool enabled)
            {
                setVisible(enabled);
                setTorrentsModelFilterEnabled(enabled);
            }

        protected:
            void init(BaseTorrentsFiltersSettingsModel* model, TorrentsProxyModel* torrentsProxyModel, Rpc* rpc)
            {
                setModel(model);

                QObject::connect(model, &BaseTorrentsFiltersSettingsModel::populatedChanged, this, [=] {
                    if (model->isPopulated()) {
                        updateCurrentIndex();
                    }
                });

                QObject::connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const QModelIndex& current) {
                    if (model->isPopulated()) {
                        setTorrentsModelFilterFromIndex(current);
                    }
                });

                QObject::connect(model, &BaseTorrentsFiltersSettingsModel::rowsInserted, this, &BaseListView::updateGeometry);
                QObject::connect(model, &BaseTorrentsFiltersSettingsModel::rowsRemoved, this, &BaseListView::updateGeometry);

                model->setTorrentsProxyModel(torrentsProxyModel);
                model->setRpc(rpc);

                if (model->isPopulated()) {
                    updateCurrentIndex();
                }
            }

            void updateCurrentIndex()
            {
                setCurrentIndex(static_cast<BaseTorrentsFiltersSettingsModel*>(model())->indexForTorrentsProxyModelFilter());
            }

            virtual void setTorrentsModelFilterFromIndex(const QModelIndex& index) = 0;
            virtual void setTorrentsModelFilterEnabled(bool enabled) = 0;

            TorrentsProxyModel* mTorrentsProxyModel;
        };

        class TrackersListView final : public BaseListView
        {
            Q_OBJECT

        public:
            TrackersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, parent)
            {
                init(new AllTrackersModel(this), torrentsModel, rpc);
                QObject::connect(torrentsModel, &TorrentsProxyModel::trackerFilterChanged, this, [=] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override
            {
                mTorrentsProxyModel->setTrackerFilter(index.data(AllTrackersModel::TrackerRole).toString());
            }

            void setTorrentsModelFilterEnabled(bool enabled) override
            {
                mTorrentsProxyModel->setTrackerFilterEnabled(enabled);
            }
        };

        class DirectoriesListView final : public BaseListView
        {
            Q_OBJECT

        public:
            DirectoriesListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, parent)
            {
                init(new DownloadDirectoriesModel(this), torrentsModel, rpc);
                QObject::connect(torrentsModel, &TorrentsProxyModel::downloadDirectoryFilterChanged, this, [=] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override
            {
                mTorrentsProxyModel->setDownloadDirectoryFilter(index.data(DownloadDirectoriesModel::DirectoryRole).toString());
            }

            void setTorrentsModelFilterEnabled(bool enabled) override
            {
                mTorrentsProxyModel->setDownloadDirectoryFilterEnabled(enabled);
            }
        };
    }

    MainWindowSideBar::MainWindowSideBar(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent)
        : QScrollArea(parent)
    {
        setFrameShape(QFrame::NoFrame);
        setStyleSheet(QLatin1String("QFrame {background: transparent}"));
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setWidgetResizable(true);

        auto contentWidget = new QWidget(this);
        auto layout = new QVBoxLayout(contentWidget);

        auto searchLineEdit = new QLineEdit(this);
        searchLineEdit->setClearButtonEnabled(true);
        searchLineEdit->setPlaceholderText(qApp->translate("tremotesf", "Search..."));
        QObject::connect(searchLineEdit, &QLineEdit::textChanged, this, [proxyModel](const auto& text) {
            proxyModel->setSearchString(text);
        });
        layout->addWidget(searchLineEdit);

        auto* const searchShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
        QObject::connect(searchShortcut, &QShortcut::activated, this, [searchLineEdit] {
            searchLineEdit->setFocus();
        });

        QFont checkBoxFont(QApplication::font());
        checkBoxFont.setBold(true);
        checkBoxFont.setCapitalization(QFont::AllUppercase);

        auto statusCheckBox = new QCheckBox(qApp->translate("tremotesf", "Status"), this);
        statusCheckBox->setChecked(true);
        statusCheckBox->setFont(checkBoxFont);
        layout->addWidget(statusCheckBox);

        auto statusListWidget = new StatusListWidget(rpc, proxyModel, this);
        QObject::connect(statusCheckBox, &QCheckBox::toggled, statusListWidget, &StatusListWidget::setVisibleExplicitly);
        layout->addWidget(statusListWidget);
        statusCheckBox->setChecked(proxyModel->isStatusFilterEnabled());

        auto directoriesCheckBox = new QCheckBox(qApp->translate("tremotesf", "Directories"), this);
        directoriesCheckBox->setChecked(true);
        directoriesCheckBox->setFont(checkBoxFont);
        layout->addWidget(directoriesCheckBox);

        auto directoriesListView = new DirectoriesListView(rpc, proxyModel, this);
        QObject::connect(directoriesCheckBox, &QCheckBox::toggled, directoriesListView, &DirectoriesListView::setFilterEnabled);
        layout->addWidget(directoriesListView);
        directoriesCheckBox->setChecked(proxyModel->isDownloadDirectoryFilterEnabled());

        auto trackersCheckBox = new QCheckBox(qApp->translate("tremotesf", "Trackers"), this);
        trackersCheckBox->setChecked(true);
        trackersCheckBox->setFont(checkBoxFont);
        layout->addWidget(trackersCheckBox);

        auto trackersListView = new TrackersListView(rpc, proxyModel, this);
        QObject::connect(trackersCheckBox, &QCheckBox::toggled, trackersListView, &TrackersListView::setFilterEnabled);
        layout->addWidget(trackersListView);
        trackersCheckBox->setChecked(proxyModel->isTrackerFilterEnabled());

        layout->addStretch();

        setWidget(contentWidget);
    }
}

#include "mainwindowsidebar.moc"
