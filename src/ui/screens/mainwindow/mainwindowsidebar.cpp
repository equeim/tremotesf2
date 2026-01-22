// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindowsidebar.h"

#include <QApplication>
#include <QCheckBox>
#include <QFrame>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QListView>
#include <QVBoxLayout>
#include <QShortcut>

#include "rpc/rpc.h"
#include "rpc/serversettings.h"
#include "ui/itemmodels/baseproxymodel.h"
#include "ui/stylehelpers.h"
#include "ui/widgets/tooltipwhenelideddelegate.h"
#include "alltrackersmodel.h"
#include "downloaddirectoriesmodel.h"
#include "downloaddirectorydelegate.h"
#include "labelsmodel.h"
#include "statusfiltersmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    namespace {
        class BaseListView : public QListView {
            Q_OBJECT

        public:
            explicit BaseListView(
                TorrentsProxyModel* torrentsProxyModel,
                BaseTorrentsFiltersSettingsModel* model,
                QAbstractItemDelegate* delegate,
                QWidget* parent
            )
                : QListView(parent), mTorrentsProxyModel(torrentsProxyModel), mModel(model) {
                mModel->setParent(this);
                setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                setIconSize(QSize(16, 16));
                if (delegate) {
                    delegate->setParent(this);
                    setItemDelegate(delegate);
                } else {
                    setItemDelegate(new TooltipWhenElidedDelegate(this));
                }
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                setTextElideMode(Qt::ElideMiddle);
                setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                makeScrollAreaTransparent(this);
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
            void init(Rpc* rpc, std::optional<int> sortByRole = {}) {
                QAbstractItemModel* actualModel{};
                if (sortByRole.has_value()) {
                    mProxyModel = new BaseProxyModel(mModel, *sortByRole, std::nullopt, this);
                    mProxyModel->sort(0);
                    actualModel = mProxyModel;
                } else {
                    actualModel = mModel;
                }

                setModel(actualModel);

                QObject::connect(
                    mModel,
                    &BaseTorrentsFiltersSettingsModel::populatedChanged,
                    this,
                    &BaseListView::updateCurrentIndex
                );

                QObject::connect(
                    selectionModel(),
                    &QItemSelectionModel::currentChanged,
                    this,
                    [this](const QModelIndex& current) {
                        if (mModel->isPopulated()) {
                            setTorrentsModelFilterFromIndex(current);
                        }
                    }
                );

                QObject::connect(
                    actualModel,
                    &BaseTorrentsFiltersSettingsModel::rowsInserted,
                    this,
                    &BaseListView::updateGeometry
                );
                QObject::connect(
                    actualModel,
                    &BaseTorrentsFiltersSettingsModel::rowsRemoved,
                    this,
                    &BaseListView::updateGeometry
                );

                mModel->setTorrentsProxyModel(mTorrentsProxyModel);
                mModel->setRpc(rpc);

                if (mModel->isPopulated()) {
                    updateCurrentIndex();
                }
            }

            void updateCurrentIndex() {
                auto index = mModel->indexForTorrentsProxyModelFilter();
                if (index.isValid() && mProxyModel) {
                    index = mProxyModel->mapFromSource(index);
                }
                if (!index.isValid() && model()->rowCount() > 0) {
                    index = model()->index(0, 0);
                }
                if (index.isValid()) {
                    setCurrentIndex(index);
                }
            }

            virtual void setTorrentsModelFilterFromIndex(const QModelIndex& index) = 0;
            virtual void setTorrentsModelFilterEnabled(bool enabled) = 0;

            TorrentsProxyModel* mTorrentsProxyModel;
            BaseTorrentsFiltersSettingsModel* mModel;
            BaseProxyModel* mProxyModel{};
        };

        class StatusFiltersListView final : public BaseListView {
            Q_OBJECT

        public:
            StatusFiltersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, new StatusFiltersModel(), nullptr, parent) {
                init(rpc);
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

        class LabelsListView final : public BaseListView {
            Q_OBJECT

        public:
            LabelsListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, new LabelsModel(), nullptr, parent) {
                init(rpc, LabelsModel::LabelRole);
                QObject::connect(torrentsModel, &TorrentsProxyModel::labelFilterChanged, this, [=, this] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override {
                mTorrentsProxyModel->setLabelFilter(index.data(LabelsModel::LabelRole).toString());
            }

            void setTorrentsModelFilterEnabled(bool enabled) override {
                mTorrentsProxyModel->setLabelFilterEnabled(enabled);
            }
        };

        class TrackersListView final : public BaseListView {
            Q_OBJECT

        public:
            TrackersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, new AllTrackersModel(), nullptr, parent) {
                init(rpc, AllTrackersModel::TrackerRole);
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
                : BaseListView(torrentsModel, new DownloadDirectoriesModel(), new DownloadDirectoryDelegate(), parent) {
                init(rpc, static_cast<int>(DownloadDirectoriesModel::Role::Directory));
                QObject::connect(torrentsModel, &TorrentsProxyModel::downloadDirectoryFilterChanged, this, [=, this] {
                    updateCurrentIndex();
                });
            }

        protected:
            void setTorrentsModelFilterFromIndex(const QModelIndex& index) override {
                mTorrentsProxyModel->setDownloadDirectoryFilter(
                    index.data(static_cast<int>(DownloadDirectoriesModel::Role::Directory)).toString()
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

        auto* const searchShortcut = new QShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_F), this);
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

        //: Title of torrents label filters list
        auto labelsCheckBox = new QCheckBox(qApp->translate("tremotesf", "Labels"), this);
        labelsCheckBox->setChecked(proxyModel->isLabelFilterEnabled());
        labelsCheckBox->setFont(checkBoxFont);
        layout->addWidget(labelsCheckBox);

        auto labelsListView = new LabelsListView(rpc, proxyModel, this);
        QObject::connect(labelsCheckBox, &QCheckBox::toggled, labelsListView, &LabelsListView::setFilterEnabled);
        layout->addWidget(labelsListView);

        QObject::connect(rpc, &Rpc::connectedChanged, this, [=] {
            const bool serverSupportsLabels =
                rpc->isConnected() ? rpc->serverSettings()->data().hasLabelsProperty() : true;
            if (serverSupportsLabels) {
                labelsCheckBox->setVisible(true);
                labelsListView->setVisible(labelsCheckBox->isChecked());
            } else {
                labelsCheckBox->setVisible(false);
                labelsListView->setVisible(false);
            }
        });

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
