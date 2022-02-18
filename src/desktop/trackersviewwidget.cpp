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

#include "trackersviewwidget.h"

#include <QCoreApplication>
#include <QCursor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "../libtremotesf/torrent.h"
#include "../libtremotesf/tracker.h"
#include "../baseproxymodel.h"
#include "../settings.h"
#include "../trackersmodel.h"
#include "../trpc.h"
#include "../utils.h"
#include "basetreeview.h"
#include "textinputdialog.h"

namespace tremotesf
{
    namespace
    {
        class EnterEatingTreeView : public BaseTreeView
        {
            Q_OBJECT
        public:
            explicit EnterEatingTreeView(QWidget* parent = nullptr)
                : BaseTreeView(parent)
            {
            }

        protected:
            void keyPressEvent(QKeyEvent* event) override
            {
                BaseTreeView::keyPressEvent(event);
                switch (event->key()) {
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    event->accept();
                }
            }
        };
    }

    TrackersViewWidget::TrackersViewWidget(libtremotesf::Torrent* torrent, Rpc* rpc, QWidget* parent)
        : QWidget(parent),
          mTorrent(torrent),
          mRpc(rpc),
          mModel(new TrackersModel(torrent, this)),
          mProxyModel(new BaseProxyModel(mModel, TrackersModel::SortRole, this)),
          mTrackersView(new EnterEatingTreeView(this))
    {
        auto layout = new QHBoxLayout(this);

        mTrackersView->setContextMenuPolicy(Qt::CustomContextMenu);
        mTrackersView->setModel(mProxyModel);
        mTrackersView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTrackersView->setRootIsDecorated(false);
        mTrackersView->header()->restoreState(Settings::instance()->trackersViewHeaderState());
        QObject::connect(mTrackersView, &EnterEatingTreeView::activated, this, &TrackersViewWidget::showEditDialogs);

        auto removeAction = new QAction(QIcon::fromTheme(QLatin1String("list-remove")), qApp->translate("tremotesf", "&Remove"), this);
        removeAction->setShortcut(QKeySequence::Delete);
        mTrackersView->addAction(removeAction);
        QObject::connect(removeAction, &QAction::triggered, this, &TrackersViewWidget::removeTrackers);

        QObject::connect(mTrackersView, &EnterEatingTreeView::customContextMenuRequested, this, [=](auto pos) {
            if (mTrackersView->indexAt(pos).isValid()) {
                QMenu contextMenu;
                QAction* editAction = contextMenu.addAction(QIcon::fromTheme(QLatin1String("document-properties")), qApp->translate("tremotesf", "&Edit..."));
                QObject::connect(editAction, &QAction::triggered, this, &TrackersViewWidget::showEditDialogs);
                contextMenu.addAction(removeAction);
                contextMenu.exec(QCursor::pos());
            }
        });

        layout->addWidget(mTrackersView);

        auto buttonsLayout = new QVBoxLayout();
        layout->addLayout(buttonsLayout);
        auto addTrackersButton = new QPushButton(QIcon::fromTheme(QLatin1String("list-add")), qApp->translate("tremotesf", "Add..."), this);
        QObject::connect(addTrackersButton, &QPushButton::clicked, this, &TrackersViewWidget::addTrackers);
        buttonsLayout->addWidget(addTrackersButton);
        auto editButton = new QPushButton(QIcon::fromTheme(QLatin1String("document-properties")), qApp->translate("tremotesf", "Edit..."), this);
        QObject::connect(editButton, &QPushButton::clicked, this, &TrackersViewWidget::showEditDialogs);
        editButton->setEnabled(false);
        buttonsLayout->addWidget(editButton);
        auto removeButton = new QPushButton(QIcon::fromTheme(QLatin1String("list-remove")), qApp->translate("tremotesf", "Remove"), this);
        removeButton->setEnabled(false);
        QObject::connect(removeButton, &QPushButton::clicked, this, &TrackersViewWidget::removeTrackers);
        buttonsLayout->addWidget(removeButton);
        auto reannounceButton = new QPushButton(QIcon::fromTheme(QLatin1String("view-refresh")), qApp->translate("tremotesf", "Reanno&unce"), this);
        QObject::connect(reannounceButton, &QPushButton::clicked, this, [=] {
            mRpc->reannounceTorrents({mTorrent->id()});
        });
        buttonsLayout->addWidget(reannounceButton);
        buttonsLayout->addStretch();

        QObject::connect(mTrackersView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=] {
            const bool hasSelection = mTrackersView->selectionModel()->hasSelection();
            editButton->setEnabled(hasSelection);
            removeButton->setEnabled(hasSelection);
        });
    }

    TrackersViewWidget::~TrackersViewWidget()
    {
        Settings::instance()->setTrackersViewHeaderState(mTrackersView->header()->saveState());
    }

    void TrackersViewWidget::setTorrent(libtremotesf::Torrent* torrent)
    {
        mTorrent = torrent;
        mModel->setTorrent(torrent);
    }

    void TrackersViewWidget::addTrackers()
    {
        auto dialog = new TextInputDialog(qApp->translate("tremotesf", "Add Trackers"),
                                          qApp->translate("tremotesf", "Trackers announce URLs:"),
                                          QString(),
                                          qApp->translate("tremotesf", "Add"),
                                          true,
                                          this);
        QObject::connect(dialog, &TextInputDialog::accepted, this, [=] {
            const auto lines = dialog->text().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            mTorrent->addTrackers(lines);
        });
        dialog->show();
    }

    void TrackersViewWidget::showEditDialogs()
    {
        const QModelIndexList indexes(mTrackersView->selectionModel()->selectedRows());
        for (const QModelIndex& index : indexes) {
            const libtremotesf::Tracker& tracker = mModel->trackerAtIndex(mProxyModel->sourceIndex(index));
            const int id = tracker.id();
            auto dialog = new TextInputDialog(qApp->translate("tremotesf", "Edit Tracker"),
                                              qApp->translate("tremotesf", "Tracker announce URL:"),
                                              tracker.announce(),
                                              QString(),
                                              false,
                                              this);
            QObject::connect(dialog, &TextInputDialog::accepted, this, [=] {
                mTorrent->setTracker(id, dialog->text());
            });
            dialog->show();
        }
    }

    void TrackersViewWidget::removeTrackers()
    {
        if (!mTrackersView->selectionModel()->hasSelection()) {
            return;
        }

        QMessageBox dialog(this);
        dialog.setIcon(QMessageBox::Warning);

        dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialog.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Remove"));
        dialog.setDefaultButton(QMessageBox::Cancel);

        const QVariantList ids(mModel->idsFromIndexes(mProxyModel->sourceIndexes(mTrackersView->selectionModel()->selectedRows())));
        if (ids.size() == 1) {
            dialog.setWindowTitle(qApp->translate("tremotesf", "Remove Tracker"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to remove this tracker?"));
        } else {
            dialog.setWindowTitle(qApp->translate("tremotesf", "Remove Trackers"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to remove %Ln selected trackers?", nullptr, ids.size()));
        }

        if (dialog.exec() == QMessageBox::Ok) {
            mTorrent->removeTrackers(ids);
        }
    }
}

#include "trackersviewwidget.moc"
