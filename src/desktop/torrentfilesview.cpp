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

#include "torrentfilesview.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QCursor>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>

#include "../localtorrentfilesmodel.h"
#include "../settings.h"
#include "../torrentfilesmodel.h"
#include "../torrentfilesproxymodel.h"
#include "commondelegate.h"

namespace tremotesf
{
    TorrentFilesView::TorrentFilesView(LocalTorrentFilesModel* model, QWidget* parent)
        : BaseTreeView(parent),
          mLocalFile(true),
          mModel(model),
          mProxyModel(new TorrentFilesProxyModel(mModel, LocalTorrentFilesModel::SortRole, this))
    {
        init();
        expand(mProxyModel->index(0, 0));
        if (!header()->restoreState(Settings::instance()->localTorrentFilesViewHeaderState())) {
            sortByColumn(LocalTorrentFilesModel::NameColumn, Qt::AscendingOrder);
        }
    }

    TorrentFilesView::TorrentFilesView(TorrentFilesModel* model, QWidget* parent)
        : BaseTreeView(parent),
          mLocalFile(false),
          mModel(model),
          mProxyModel(new TorrentFilesProxyModel(mModel, TorrentFilesModel::SortRole, this))
    {
        init();
        setItemDelegate(new CommonDelegate(TorrentFilesModel::ProgressBarColumn, TorrentFilesModel::SortRole, this));
        if (!header()->restoreState(Settings::instance()->torrentFilesViewHeaderState())) {
            sortByColumn(TorrentFilesModel::NameColumn, Qt::AscendingOrder);
        }
    }

    TorrentFilesView::~TorrentFilesView()
    {
        if (mLocalFile) {
            Settings::instance()->setLocalTorrentFilesViewHeaderState(header()->saveState());
        } else {
            Settings::instance()->setTorrentFilesViewHeaderState(header()->saveState());
        }
    }

    void TorrentFilesView::init()
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setModel(mProxyModel);
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        QObject::connect(mModel, &BaseTorrentFilesModel::modelReset, this, [=]() {
            if (mModel->rowCount(mModel->index(0, 0)) == 0) {
                setRootIsDecorated(false);
            } else {
                expand(mProxyModel->index(0, 0));
            }
        });

        QObject::connect(this, &TorrentFilesView::customContextMenuRequested, this, &TorrentFilesView::showContextMenu);
    }

    void TorrentFilesView::showContextMenu(const QPoint& pos)
    {
        if (!indexAt(pos).isValid()) {
            return;
        }

        const QModelIndexList sourceIndexes(mProxyModel->sourceIndexes(selectionModel()->selectedRows()));

        QMenu contextMenu;

        QAction* downloadAction = contextMenu.addAction(qApp->translate("tremotesf", "Download", "File menu item"));
        QObject::connect(downloadAction, &QAction::triggered, this, [=, &sourceIndexes]() {
            mModel->setFilesWanted(sourceIndexes, true);
        });

        QAction* notDownloadAction = contextMenu.addAction(qApp->translate("tremotesf", "Not Download"));
        QObject::connect(notDownloadAction, &QAction::triggered, this, [=, &sourceIndexes]() {
            mModel->setFilesWanted(sourceIndexes, false);
        });

        contextMenu.addSeparator();

        QMenu* priorityMenu = contextMenu.addMenu(qApp->translate("tremotesf", "Priority"));
        QActionGroup priorityGroup(this);
        priorityGroup.setExclusive(true);

        QAction* highPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "High"));
        highPriorityAction->setCheckable(true);
        QObject::connect(highPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) {
                mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntryEnums::HighPriority);
            }
        });

        QAction* normalPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "Normal"));
        normalPriorityAction->setCheckable(true);
        QObject::connect(normalPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) {
                mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntryEnums::NormalPriority);
            }
        });

        QAction* lowPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "Low"));
        lowPriorityAction->setCheckable(true);
        QObject::connect(lowPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) {
                mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntryEnums::LowPriority);
            }
        });

        QAction* mixedPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "Mixed"));
        mixedPriorityAction->setCheckable(true);
        mixedPriorityAction->setChecked(true);
        mixedPriorityAction->setVisible(false);

        priorityMenu->addActions(priorityGroup.actions());

        if (sourceIndexes.size() == 1) {
            auto entry = static_cast<const TorrentFilesModelEntry*>(sourceIndexes.first().internalPointer());
            if (entry->wantedState() == TorrentFilesModelEntryEnums::Wanted) {
                downloadAction->setEnabled(false);
            } else if (entry->wantedState() == TorrentFilesModelEntryEnums::Unwanted) {
                notDownloadAction->setEnabled(false);
            }

            switch (entry->priority()) {
            case TorrentFilesModelEntryEnums::LowPriority:
                lowPriorityAction->setChecked(true);
                break;
            case TorrentFilesModelEntryEnums::NormalPriority:
                normalPriorityAction->setChecked(true);
                break;
            case TorrentFilesModelEntryEnums::HighPriority:
                highPriorityAction->setChecked(true);
                break;
            case TorrentFilesModelEntryEnums::MixedPriority:
                mixedPriorityAction->setVisible(true);
            }
        }

        contextMenu.exec(QCursor::pos());
    }
}
