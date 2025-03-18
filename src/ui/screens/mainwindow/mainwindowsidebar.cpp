// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
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
#include "ui/widgets/commondelegate.h"
#include "alltrackersmodel.h"
#include "downloaddirectoriesmodel.h"
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
                QWidget* parent = nullptr
            )
                : QListView(parent), mTorrentsProxyModel(torrentsProxyModel), mModel(model) {
                mModel->setParent(this);
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
            void init(Rpc* rpc, std::optional<int> sortByRole = {}) {
                QAbstractItemModel* actualModel{};
                if (sortByRole.has_value()) {
                    mProxyModel = new QSortFilterProxyModel(this);
                    mProxyModel->setSourceModel(mModel);
                    mProxyModel->setSortRole(*sortByRole);
                    mProxyModel->sort(0);
                    actualModel = mProxyModel;
                } else {
                    actualModel = mModel;
                }

                setModel(actualModel);

                QObject::connect(mModel, &BaseTorrentsFiltersSettingsModel::populatedChanged, this, [this] {
                    if (mModel->isPopulated()) {
                        updateCurrentIndex();
                    }
                });

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
                const auto index = mModel->indexForTorrentsProxyModelFilter();
                if (mProxyModel) {
                    setCurrentIndex(mProxyModel->mapFromSource(index));
                } else {
                    setCurrentIndex(index);
                }
            }

            virtual void setTorrentsModelFilterFromIndex(const QModelIndex& index) = 0;
            virtual void setTorrentsModelFilterEnabled(bool enabled) = 0;

            TorrentsProxyModel* mTorrentsProxyModel;
            BaseTorrentsFiltersSettingsModel* mModel;
            QSortFilterProxyModel* mProxyModel{};
        };

        class StatusFiltersListView final : public BaseListView {
            Q_OBJECT

        public:
            StatusFiltersListView(Rpc* rpc, TorrentsProxyModel* torrentsModel, QWidget* parent = nullptr)
                : BaseListView(torrentsModel, new StatusFiltersModel(), parent) {
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
                : BaseListView(torrentsModel, new LabelsModel(), parent) {
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
                : BaseListView(torrentsModel, new AllTrackersModel(), parent) {
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
                : BaseListView(torrentsModel, new DownloadDirectoriesModel(), parent) {
                init(rpc, DownloadDirectoriesModel::DirectoryRole);
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
