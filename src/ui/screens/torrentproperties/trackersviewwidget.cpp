// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trackersviewwidget.h"

#include <array>

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "rpc/torrent.h"
#include "rpc/tracker.h"
#include "stdutils.h"
#include "ui/itemmodels/baseproxymodel.h"
#include "ui/widgets/basetreeview.h"
#include "ui/widgets/textinputdialog.h"
#include "rpc/rpc.h"
#include "settings.h"
#include "trackersmodel.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        class EnterEatingTreeView final : public BaseTreeView {
            Q_OBJECT

        public:
            explicit EnterEatingTreeView(QWidget* parent = nullptr) : BaseTreeView(parent) {}

        protected:
            void keyPressEvent(QKeyEvent* event) override {
                BaseTreeView::keyPressEvent(event);
                switch (event->key()) {
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    event->accept();
                default:
                    break;
                }
            }
        };
    }

    TrackersViewWidget::TrackersViewWidget(Rpc* rpc, QWidget* parent)
        : QWidget(parent),
          mRpc(rpc),
          mModel(new TrackersModel(this)),
          mProxyModel(new BaseProxyModel(
              mModel, TrackersModel::SortRole, static_cast<int>(TrackersModel::Column::Announce), this
          )),
          mTrackersView(new EnterEatingTreeView(this)) {
        auto layout = new QHBoxLayout(this);

        mTrackersView->setContextMenuPolicy(Qt::CustomContextMenu);
        mTrackersView->setModel(mProxyModel);
        mTrackersView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTrackersView->setRootIsDecorated(false);
        mTrackersView->header()->restoreState(Settings::instance()->get_trackersViewHeaderState());
        QObject::connect(mTrackersView, &EnterEatingTreeView::activated, this, &TrackersViewWidget::showEditDialogs);

        auto removeAction = new QAction(
            QIcon::fromTheme("list-remove"_L1),
            //: Tracker's context menu item
            qApp->translate("tremotesf", "&Remove"),
            this
        );
        removeAction->setShortcut(QKeySequence::Delete);
        mTrackersView->addAction(removeAction);
        QObject::connect(removeAction, &QAction::triggered, this, &TrackersViewWidget::removeTrackers);

        QObject::connect(mTrackersView, &EnterEatingTreeView::customContextMenuRequested, this, [=, this](QPoint pos) {
            if (mTrackersView->indexAt(pos).isValid()) {
                QMenu contextMenu;
                QAction* editAction = contextMenu.addAction(
                    QIcon::fromTheme("document-properties"_L1),
                    //: Tracker's context menu item
                    qApp->translate("tremotesf", "&Edit...")
                );
                QObject::connect(editAction, &QAction::triggered, this, &TrackersViewWidget::showEditDialogs);
                contextMenu.addAction(removeAction);
                contextMenu.exec(mTrackersView->viewport()->mapToGlobal(pos));
            }
        });

        layout->addWidget(mTrackersView);

        auto buttonsLayout = new QVBoxLayout();
        layout->addLayout(buttonsLayout);
        auto addTrackersButton = new QPushButton(
            QIcon::fromTheme("list-add"_L1),
            //: Button
            qApp->translate("tremotesf", "Add..."),
            this
        );
        QObject::connect(addTrackersButton, &QPushButton::clicked, this, &TrackersViewWidget::addTrackers);
        buttonsLayout->addWidget(addTrackersButton);
        auto editButton = new QPushButton(
            QIcon::fromTheme("document-properties"_L1),
            //: Button
            qApp->translate("tremotesf", "Edit..."),
            this
        );
        QObject::connect(editButton, &QPushButton::clicked, this, &TrackersViewWidget::showEditDialogs);
        editButton->setEnabled(false);
        buttonsLayout->addWidget(editButton);
        auto removeButton = new QPushButton(
            QIcon::fromTheme("list-remove"_L1),
            //: Button
            qApp->translate("tremotesf", "Remove"),
            this
        );
        removeButton->setEnabled(false);
        QObject::connect(removeButton, &QPushButton::clicked, this, &TrackersViewWidget::removeTrackers);
        buttonsLayout->addWidget(removeButton);
        auto reannounceButton = new QPushButton(
            QIcon::fromTheme("view-refresh"_L1),
            //: Button
            qApp->translate("tremotesf", "Reanno&unce"),
            this
        );
        QObject::connect(reannounceButton, &QPushButton::clicked, this, [=, this] {
            mRpc->reannounceTorrents(std::array{mTorrent->data().id});
        });
        buttonsLayout->addWidget(reannounceButton);
        buttonsLayout->addStretch();

        QObject::connect(mTrackersView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=, this] {
            const bool hasSelection = mTrackersView->selectionModel()->hasSelection();
            editButton->setEnabled(hasSelection);
            removeButton->setEnabled(hasSelection);
        });
    }

    void TrackersViewWidget::setTorrent(Torrent* torrent, bool oldTorrentDestroyed) {
        mTorrent = torrent;
        mModel->setTorrent(torrent, oldTorrentDestroyed);
    }

    void TrackersViewWidget::saveState() {
        Settings::instance()->set_trackersViewHeaderState(mTrackersView->header()->saveState());
    }

    void TrackersViewWidget::addTrackers() {
        auto dialog = new TextInputDialog(
            //: Dialog title
            qApp->translate("tremotesf", "Add Trackers"),
            qApp->translate("tremotesf", "Trackers announce URLs:"),
            QString(),
            //: Dialog confirmation button
            qApp->translate("tremotesf", "Add"),
            true,
            this
        );
        QObject::connect(dialog, &TextInputDialog::accepted, this, [=, this] {
            auto lines = dialog->text().split('\n', Qt::SkipEmptyParts);
            mTorrent->addTrackers(toContainer<std::vector>(lines | std::views::transform([](QString& announceUrl) {
                                                               return std::set{std::move(announceUrl)};
                                                           })));
        });
        dialog->show();
    }

    void TrackersViewWidget::showEditDialogs() {
        const QModelIndexList indexes(mTrackersView->selectionModel()->selectedRows());
        for (const QModelIndex& index : indexes) {
            const Tracker& tracker = mModel->trackerAtIndex(mProxyModel->mapToSource(index));
            const int id = tracker.id();
            auto dialog = new TextInputDialog(
                //: Dialog title
                qApp->translate("tremotesf", "Edit Tracker"),
                qApp->translate("tremotesf", "Tracker announce URL:"),
                tracker.announce(),
                QString(),
                false,
                this
            );
            QObject::connect(dialog, &TextInputDialog::accepted, this, [=, this] {
                mTorrent->setTracker(id, dialog->text());
            });
            dialog->show();
        }
    }

    void TrackersViewWidget::removeTrackers() {
        if (!mTrackersView->selectionModel()->hasSelection()) {
            return;
        }

        QMessageBox dialog(this);
        dialog.setIcon(QMessageBox::Warning);

        dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        //: Dialog confirmation button
        dialog.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Remove"));
        dialog.setDefaultButton(QMessageBox::Cancel);

        const auto ids =
            mModel->idsFromIndexes(mProxyModel->sourceIndexes(mTrackersView->selectionModel()->selectedRows()));
        if (ids.size() == 1) {
            //: Dialog title
            dialog.setWindowTitle(qApp->translate("tremotesf", "Remove Tracker"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to remove this tracker?"));
        } else {
            //: Dialog title
            dialog.setWindowTitle(qApp->translate("tremotesf", "Remove Trackers"));
            // Don't put static_cast in qApp->translate() - lupdate doesn't like it
            const auto count = static_cast<int>(ids.size());
            //: %Ln is number of trackers selected for deletion
            dialog.setText(
                qApp->translate("tremotesf", "Are you sure you want to remove %Ln selected trackers?", nullptr, count)
            );
        }

        if (dialog.exec() == QMessageBox::Ok) {
            mTorrent->removeTrackers(ids);
        }
    }
}

#include "trackersviewwidget.moc"
