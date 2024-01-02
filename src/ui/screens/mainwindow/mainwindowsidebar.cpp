// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

#include "rpc/rpc.h"
#include "ui/widgets/commondelegate.h"
#include "alltrackersmodel.h"
#include "downloaddirectoriesmodel.h"
#include "statusfiltersmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    namespace {
        class BaseListView : public QListView {
            Q_OBJECT

        public:
            explicit BaseListView(TorrentsProxyModel* torrentsProxyModel, QWidget* parent = nullptr)
                : QListView(parent), mTorrentsProxyModel(torrentsProxyModel) {
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

            [[nodiscard]] QSize minimumSizeHint() const override { return {8, 0}; }

            [[nodiscard]] QSize sizeHint() const override {
                int height = 0;
                for (int i = 0, max = model()->rowCount(); i < max; ++i) {
                    height += sizeHintForRow(i);
                    height += spacing();
                }
                height += spacing();

                return {sizeHintForColumn(0), height};
            }

            void setFilterEnabled(bool enabled) {
                setVisible(enabled);
                setTorrentsModelFilterEnabled(enabled);
            }

        protected:
            void init(BaseTorrentsFiltersSettingsModel* model, TorrentsProxyModel* torrentsProxyModel, Rpc* rpc) {
                setModel(model);

                QObject::connect(model, &BaseTorrentsFiltersSettingsModel::populatedChanged, this, [=, this] {
                    if (model->isPopulated()) {
                        updateCurrentIndex();
                    }
                });

                QObject::connect(
                    selectionModel(),
                    &QItemSelectionModel::currentChanged,
                    this,
                    [=, this](const QModelIndex& current) {
                        if (model->isPopulated()) {
                            setTorrentsModelFilterFromIndex(current);
                        }
                    }
                );

                QObject::connect(
                    model,
                    &BaseTorrentsFiltersSettingsModel::rowsInserted,
                    this,
                    &BaseListView::updateGeometry
                );
                QObject::connect(
                    model,
                    &BaseTorrentsFiltersSettingsModel::rowsRemoved,
                    this,
                    &BaseListView::updateGeometry
                );

                model->setTorrentsProxyModel(torrentsProxyModel);
                model->setRpc(rpc);

                if (model->isPopulated()) {
                    updateCurrentIndex();
                }
            }

            void updateCurrentIndex() {
                setCurrentIndex(
                    static_cast<BaseTorrentsFiltersSettingsModel*>(model())->indexForTorrentsProxyModelFilter()
                );
            }

            virtual void setTorrentsModelFilterFromIndex(const QModelIndex& index) = 0;
            virtual void setTorrentsModelFilterEnabled(bool enabled) = 0;

            TorrentsProxyModel* mTorrentsProxyModel;
        };

        class StatusFiltersListView final : public BaseListView {
            Q_OBJECT

        public:
            StatusFiltersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, parent) {
                init(new StatusFiltersModel(this), torrentsModel, rpc);
                QObject::connect(torrentsModel, &TorrentsProxyModel::statusFilterChanged, this, [=, this] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override {
                mTorrentsProxyModel->setStatusFilter(
                    index.data(StatusFiltersModel::FilterRole).value<TorrentsProxyModel::StatusFilter>()
                );
            }

            void setTorrentsModelFilterEnabled(bool enabled) override {
                mTorrentsProxyModel->setStatusFilterEnabled(enabled);
            }
        };

        class TrackersListView final : public BaseListView {
            Q_OBJECT

        public:
            TrackersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, parent) {
                init(new AllTrackersModel(this), torrentsModel, rpc);
                QObject::connect(torrentsModel, &TorrentsProxyModel::trackerFilterChanged, this, [=, this] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override {
                mTorrentsProxyModel->setTrackerFilter(index.data(AllTrackersModel::TrackerRole).toString());
            }

            void setTorrentsModelFilterEnabled(bool enabled) override {
                mTorrentsProxyModel->setTrackerFilterEnabled(enabled);
            }
        };

        class DirectoriesListView final : public BaseListView {
            Q_OBJECT

        public:
            DirectoriesListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, parent) {
                init(new DownloadDirectoriesModel(this), torrentsModel, rpc);
                QObject::connect(torrentsModel, &TorrentsProxyModel::downloadDirectoryFilterChanged, this, [=, this] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override {
                mTorrentsProxyModel->setDownloadDirectoryFilter(
                    index.data(DownloadDirectoriesModel::DirectoryRole).toString()
                );
            }

            void setTorrentsModelFilterEnabled(bool enabled) override {
                mTorrentsProxyModel->setDownloadDirectoryFilterEnabled(enabled);
            }
        };
    }

    MainWindowSideBar::MainWindowSideBar(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent)
        : QScrollArea(parent) {
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setWidgetResizable(true);

        auto contentWidget = new QWidget(this);
        auto layout = new QVBoxLayout(contentWidget);

        auto searchLineEdit = new QLineEdit(this);
        searchLineEdit->setClearButtonEnabled(true);
        //: Search field placeholder
        searchLineEdit->setPlaceholderText(qApp->translate("tremotesf", "Search..."));
        QObject::connect(searchLineEdit, &QLineEdit::textChanged, this, [proxyModel](const auto& text) {
            proxyModel->setSearchString(text);
        });
        layout->addWidget(searchLineEdit);

        auto* const searchShortcut = new QShortcut(
#if QT_VERSION_MAJOR >= 6
            QKeyCombination(Qt::ControlModifier, Qt::Key_F),
#else
            QKeySequence(static_cast<int>(Qt::ControlModifier) | static_cast<int>(Qt::Key_F)),
#endif
            this
        );
        QObject::connect(searchShortcut, &QShortcut::activated, this, [searchLineEdit] {
            searchLineEdit->selectAll();
            searchLineEdit->setFocus();
        });

        QFont checkBoxFont(QApplication::font());
        checkBoxFont.setBold(true);
        checkBoxFont.setCapitalization(QFont::AllUppercase);

        //: Title of torrents status filters list
        auto statusCheckBox = new QCheckBox(qApp->translate("tremotesf", "Status"), this);
        statusCheckBox->setChecked(true);
        statusCheckBox->setFont(checkBoxFont);
        layout->addWidget(statusCheckBox);

        auto statusFiltersListView = new StatusFiltersListView(rpc, proxyModel, this);
        QObject::connect(
            statusCheckBox,
            &QCheckBox::toggled,
            statusFiltersListView,
            &StatusFiltersListView::setFilterEnabled
        );
        layout->addWidget(statusFiltersListView);
        statusCheckBox->setChecked(proxyModel->isStatusFilterEnabled());

        //: Title of torrents download directory filters list
        auto directoriesCheckBox = new QCheckBox(qApp->translate("tremotesf", "Directories"), this);
        directoriesCheckBox->setChecked(true);
        directoriesCheckBox->setFont(checkBoxFont);
        layout->addWidget(directoriesCheckBox);

        auto directoriesListView = new DirectoriesListView(rpc, proxyModel, this);
        QObject::connect(
            directoriesCheckBox,
            &QCheckBox::toggled,
            directoriesListView,
            &DirectoriesListView::setFilterEnabled
        );
        layout->addWidget(directoriesListView);
        directoriesCheckBox->setChecked(proxyModel->isDownloadDirectoryFilterEnabled());

        //: Title of torrents tracker filters list
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
