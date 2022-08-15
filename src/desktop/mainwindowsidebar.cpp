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
#include "../statusfiltersmodel.h"
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

                QPalette palette{};
                palette.setColor(QPalette::Base, QColor(Qt::transparent));
                setPalette(palette);
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

        class StatusFiltersListView final : public BaseListView
        {
            Q_OBJECT

        public:
            StatusFiltersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, parent)
            {
                init(new StatusFiltersModel(this), torrentsModel, rpc);
                QObject::connect(torrentsModel, &TorrentsProxyModel::statusFilterChanged, this, [=] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override
            {
                mTorrentsProxyModel->setStatusFilter(index.data(StatusFiltersModel::FilterRole).value<TorrentsProxyModel::StatusFilter>());
            }

            void setTorrentsModelFilterEnabled(bool enabled) override
            {
                mTorrentsProxyModel->setStatusFilterEnabled(enabled);
            }
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

        auto* const searchShortcut = new QShortcut(QKeySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::Key_F)), this);
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

        auto statusFiltersListView = new StatusFiltersListView(rpc, proxyModel, this);
        QObject::connect(statusCheckBox, &QCheckBox::toggled, statusFiltersListView, &StatusFiltersListView::setFilterEnabled);
        layout->addWidget(statusFiltersListView);
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
