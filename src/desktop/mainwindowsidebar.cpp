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

namespace tremotesf
{
    namespace
    {
        class StatusListWidget : public QListWidget
        {
            Q_OBJECT
        public:
            StatusListWidget(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent = nullptr)
                : QListWidget(parent),
                  mProxyModel(proxyModel),
                  mRpc(rpc),
                  mStats(new StatusFilterStats(mRpc, this))
            {
                setFrameShape(QFrame::NoFrame);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

                setIconSize(QSize(16, 16));

                using namespace desktoputils;
                addItem(new QListWidgetItem(QApplication::style()->standardIcon(QStyle::SP_DirIcon), QString(), this));
                addItem(new QListWidgetItem(QIcon(statusIconPath(ActiveIcon)), QString(), this));
                addItem(new QListWidgetItem(QIcon(statusIconPath(DownloadingIcon)), QString(), this));
                addItem(new QListWidgetItem(QIcon(statusIconPath(SeedingIcon)), QString(), this));
                addItem(new QListWidgetItem(QIcon(statusIconPath(PausedIcon)), QString(), this));
                addItem(new QListWidgetItem(QIcon(statusIconPath(CheckingIcon)), QString(), this));
                addItem(new QListWidgetItem(QIcon(statusIconPath(ErroredIcon)), QString(), this));
                setCurrentRow(0);

                updateItems();
                QObject::connect(mStats, &StatusFilterStats::updated, this, &StatusListWidget::updateItems);

                QObject::connect(this, &StatusListWidget::currentRowChanged, this, [this](int row) {
                    mProxyModel->setStatusFilter(static_cast<TorrentsProxyModel::StatusFilter>(row));
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
                item(TorrentsProxyModel::All)->setText(qApp->translate("tremotesf", "All (%L1)", "All torrents, %L1 - torrents count").arg(mRpc->torrentsCount()));

                //: Active torrents, %L1 - torrents count
                item(TorrentsProxyModel::Active)->setText(qApp->translate("tremotesf", "Active (%L1)").arg(mStats->activeTorrents()));

                //: Downloading torrents, %L1 - torrents count
                item(TorrentsProxyModel::Downloading)->setText(qApp->translate("tremotesf", "Downloading (%L1)").arg(mStats->downloadingTorrents()));

                //: Seeding torrents, %L1 - torrents count
                item(TorrentsProxyModel::Seeding)->setText(qApp->translate("tremotesf", "Seeding (%L1)").arg(mStats->seedingTorrents()));

                //: Paused torrents, %L1 - torrents count
                item(TorrentsProxyModel::Paused)->setText(qApp->translate("tremotesf", "Paused (%L1)").arg(mStats->pausedTorrents()));

                //: Checking torrents, %L1 - torrents count
                item(TorrentsProxyModel::Checking)->setText(qApp->translate("tremotesf", "Checking (%L1)").arg(mStats->checkingTorrents()));

                //: Errored torrents, %L1 - torrents count
                item(TorrentsProxyModel::Errored)->setText(qApp->translate("tremotesf", "Errored (%L1)").arg(mStats->erroredTorrents()));
            }

        protected:
            void showEvent(QShowEvent*) override
            {
                mProxyModel->setStatusFilter(static_cast<TorrentsProxyModel::StatusFilter>(currentRow()));
            }

            void hideEvent(QHideEvent*) override
            {
                mProxyModel->setStatusFilter(TorrentsProxyModel::All);
            }

        private:
            TorrentsProxyModel* mProxyModel;
            Rpc* mRpc;
            StatusFilterStats* mStats;
        };

        class BaseListView : public QListView
        {
            Q_OBJECT

        public:
            explicit BaseListView(TorrentsProxyModel* proxyModel, QWidget* parent = nullptr)
                : QListView(parent),
                  mTorrentsProxyModel(proxyModel)
            {
                setFrameShape(QFrame::NoFrame);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                setIconSize(QSize(16, 16));
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

        protected:
            TorrentsProxyModel* mTorrentsProxyModel;
        };

        class TrackersListView : public BaseListView
        {
            Q_OBJECT

        public:
            TrackersListView(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent = nullptr)
                : BaseListView(proxyModel, parent)
            {
                setItemDelegate(new CommonDelegate(this));
                setTextElideMode(Qt::ElideMiddle);
                auto model = new AllTrackersModel(rpc, mTorrentsProxyModel, this);
                setModel(model);
                setCurrentIndex(model->index(0));

                QObject::connect(model, &AllTrackersModel::rowsInserted, this, &TrackersListView::updateGeometry);
                QObject::connect(model, &AllTrackersModel::rowsRemoved, this, &TrackersListView::updateGeometry);

                QObject::connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const auto& current) {
                    mTorrentsProxyModel->setTracker(current.data(AllTrackersModel::TrackerRole).toString());
                });
            }

        protected:
            void showEvent(QShowEvent*) override
            {
                mTorrentsProxyModel->setTracker(currentIndex().data(AllTrackersModel::TrackerRole).toString());
            }

            void hideEvent(QHideEvent*) override
            {
                mTorrentsProxyModel->setTracker(QString());
            }
        };

        class DirectoriesListView : public BaseListView
        {
            Q_OBJECT

        public:
            DirectoriesListView(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent = nullptr)
                : BaseListView(proxyModel, parent)
            {
                setItemDelegate(new CommonDelegate(this));
                setTextElideMode(Qt::ElideMiddle);
                auto model = new DownloadDirectoriesModel(rpc, mTorrentsProxyModel, this);
                setModel(model);
                setCurrentIndex(model->index(0));

                QObject::connect(model, &DownloadDirectoriesModel::rowsInserted, this, &TrackersListView::updateGeometry);
                QObject::connect(model, &DownloadDirectoriesModel::rowsRemoved, this, &TrackersListView::updateGeometry);

                QObject::connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const auto& current) {
                    mTorrentsProxyModel->setDownloadDirectory(current.data(DownloadDirectoriesModel::DirectoryRole).toString());
                });
            }

        protected:
            void showEvent(QShowEvent*) override
            {
                mTorrentsProxyModel->setDownloadDirectory(currentIndex().data(DownloadDirectoriesModel::DirectoryRole).toString());
            }

            void hideEvent(QHideEvent*) override
            {
                mTorrentsProxyModel->setDownloadDirectory(QString());
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
        QObject::connect(statusCheckBox, &QCheckBox::toggled, statusListWidget, &StatusListWidget::setVisible);
        layout->addWidget(statusListWidget);

        auto directoriesCheckBox = new QCheckBox(qApp->translate("tremotesf", "Directories"), this);
        directoriesCheckBox->setChecked(true);
        directoriesCheckBox->setFont(checkBoxFont);
        layout->addWidget(directoriesCheckBox);

        auto directoriesListView = new DirectoriesListView(rpc, proxyModel, this);
        QObject::connect(directoriesCheckBox, &QCheckBox::toggled, directoriesListView, &TrackersListView::setVisible);
        layout->addWidget(directoriesListView);

        auto trackersCheckBox = new QCheckBox(qApp->translate("tremotesf", "Trackers"), this);
        trackersCheckBox->setChecked(true);
        trackersCheckBox->setFont(checkBoxFont);
        layout->addWidget(trackersCheckBox);

        auto trackersListView = new TrackersListView(rpc, proxyModel, this);
        QObject::connect(trackersCheckBox, &QCheckBox::toggled, trackersListView, &TrackersListView::setVisible);
        layout->addWidget(trackersListView);

        layout->addStretch();

        setWidget(contentWidget);
    }
}

#include "mainwindowsidebar.moc"
