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

#include "torrentfilesview.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QCursor>
#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>

#include "../libtremotesf/serversettings.h"

#include "../localtorrentfilesmodel.h"
#include "../settings.h"
#include "../torrentfilesmodel.h"
#include "../torrentfilesproxymodel.h"
#include "../trpc.h"
#include "../utils.h"

#include "commondelegate.h"
#include "textinputdialog.h"

namespace tremotesf
{
    TorrentFilesView::TorrentFilesView(LocalTorrentFilesModel* model, Rpc* rpc, QWidget* parent)
        : BaseTreeView(parent),
          mLocalFile(true),
          mModel(model),
          mProxyModel(new TorrentFilesProxyModel(mModel, LocalTorrentFilesModel::SortRole, this)),
          mRpc(rpc)
    {
        init();
        expand(mProxyModel->index(0, 0));
        if (!header()->restoreState(Settings::instance()->localTorrentFilesViewHeaderState())) {
            sortByColumn(LocalTorrentFilesModel::NameColumn, Qt::AscendingOrder);
        }
    }

    TorrentFilesView::TorrentFilesView(TorrentFilesModel* model,
                                       Rpc* rpc,
                                       QWidget* parent)
        : BaseTreeView(parent),
          mLocalFile(false),
          mModel(model),
          mProxyModel(new TorrentFilesProxyModel(mModel, TorrentFilesModel::SortRole, this)),
          mRpc(rpc)
    {
        init();
        setItemDelegate(new CommonDelegate(TorrentFilesModel::ProgressBarColumn, TorrentFilesModel::SortRole, this));
        if (!header()->restoreState(Settings::instance()->torrentFilesViewHeaderState())) {
            sortByColumn(TorrentFilesModel::NameColumn, Qt::AscendingOrder);
        }

        QObject::connect(this, &TorrentFilesView::activated, this, [=](const QModelIndex& index) {
            const QModelIndex sourceIndex(mProxyModel->sourceIndex(index));
            const TorrentFilesModelEntry* entry = static_cast<const TorrentFilesModelEntry*>(mProxyModel->sourceIndex(index).internalPointer());
            if (!entry->isDirectory() &&
                    mRpc->isTorrentLocalMounted(static_cast<const TorrentFilesModel*>(mModel)->torrent()) &&
                    entry->wantedState() != TorrentFilesModelEntry::Unwanted) {
                Utils::openFile(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(sourceIndex), this);
            }
        });
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

        if (!mLocalFile) {
            bool show = true;
            for (const QModelIndex& index : sourceIndexes) {
                if (static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->wantedState() == TorrentFilesModelEntry::Unwanted) {
                    show = false;
                    break;
                }
            }
            if (show) {
                bool disableOpen = false;
                bool disableBoth = false;

                libtremotesf::Torrent* torrent = static_cast<const TorrentFilesModel*>(mModel)->torrent();
                if (mRpc->isTorrentLocalMounted(torrent)) {
                    if (sourceIndexes.size() == 1 && !sourceIndexes.first().parent().internalPointer()) {
                        disableOpen = true;
                    } else {
                        for (const QModelIndex& index : sourceIndexes) {
                            if (!QFileInfo::exists(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(index))) {
                                disableBoth = true;
                                break;
                            }
                        }
                    }
                } else {
                    disableBoth = true;
                }

                QAction* openAction = contextMenu.addAction(QIcon::fromTheme(QLatin1String("document-open")), qApp->translate("tremotesf", "&Open"));
                openAction->setEnabled(!disableBoth && !disableOpen);
                QObject::connect(openAction, &QAction::triggered, this, [=, &sourceIndexes]() {
                    for (const QModelIndex& index : sourceIndexes) {
                        Utils::openFile(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(index), this);
                    }
                });

                QAction* showInFileManagerAction = contextMenu.addAction(QIcon::fromTheme(QLatin1String("go-jump")), qApp->translate("tremotesf", "Show In &File Manager"));
                showInFileManagerAction->setEnabled(!disableBoth);
                QObject::connect(showInFileManagerAction, &QAction::triggered, this, [=, &sourceIndexes]() {
                    QStringList files;
                    files.reserve(sourceIndexes.size());
                    for (const QModelIndex& index : sourceIndexes) {
                        files.push_back(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(index));
                    }
                    Utils::selectFilesInFileManager(files, this);
                });
            }
        }

        contextMenu.addSeparator();

        QAction* downloadAction = contextMenu.addAction(qApp->translate("tremotesf", "&Download", "File menu item, verb"));
        QObject::connect(downloadAction, &QAction::triggered, this, [=, &sourceIndexes]() {
            mModel->setFilesWanted(sourceIndexes, true);
        });

        QAction* notDownloadAction = contextMenu.addAction(qApp->translate("tremotesf", "&Not Download"));
        QObject::connect(notDownloadAction, &QAction::triggered, this, [=, &sourceIndexes]() {
            mModel->setFilesWanted(sourceIndexes, false);
        });

        contextMenu.addSeparator();

        QMenu* priorityMenu = contextMenu.addMenu(qApp->translate("tremotesf", "&Priority"));
        QActionGroup priorityGroup(this);
        priorityGroup.setExclusive(true);

        //: Priority
        QAction* highPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "&High"));
        highPriorityAction->setCheckable(true);
        QObject::connect(highPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) {
                mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntry::HighPriority);
            }
        });

        //: Priority
        QAction* normalPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "&Normal"));
        normalPriorityAction->setCheckable(true);
        QObject::connect(normalPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) {
                mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntry::NormalPriority);
            }
        });

        //: Priority
        QAction* lowPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "&Low"));
        lowPriorityAction->setCheckable(true);
        QObject::connect(lowPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) {
                mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntry::LowPriority);
            }
        });

        QAction* mixedPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "Mixed"));
        mixedPriorityAction->setCheckable(true);
        mixedPriorityAction->setChecked(true);
        mixedPriorityAction->setVisible(false);

        priorityMenu->addActions(priorityGroup.actions());

        if (sourceIndexes.size() == 1) {
            auto entry = static_cast<const TorrentFilesModelEntry*>(sourceIndexes.first().internalPointer());
            if (entry->wantedState() == TorrentFilesModelEntry::Wanted) {
                downloadAction->setEnabled(false);
            } else if (entry->wantedState() == TorrentFilesModelEntry::Unwanted) {
                notDownloadAction->setEnabled(false);
            }

            switch (entry->priority()) {
            case TorrentFilesModelEntry::LowPriority:
                lowPriorityAction->setChecked(true);
                break;
            case TorrentFilesModelEntry::NormalPriority:
                normalPriorityAction->setChecked(true);
                break;
            case TorrentFilesModelEntry::HighPriority:
                highPriorityAction->setChecked(true);
                break;
            case TorrentFilesModelEntry::MixedPriority:
                mixedPriorityAction->setVisible(true);
            }
        }

        if (mRpc->serverSettings()->canRenameFiles()) {
            contextMenu.addSeparator();
            QAction* renameAction = contextMenu.addAction(qApp->translate("tremotesf", "&Rename"));
            renameAction->setEnabled(sourceIndexes.size() == 1);
            QObject::connect(renameAction, &QAction::triggered, this, [=]() {
                const QModelIndex& index = sourceIndexes.first();
                auto entry = static_cast<const TorrentFilesModelEntry*>(index.internalPointer());
                auto dialog = new TextInputDialog(qApp->translate("tremotesf", "Rename"),
                                                  qApp->translate("tremotesf", "File name:"),
                                                  entry->name(),
                                                  qApp->translate("tremotesf", "Rename"),
                                                  this);
                QObject::connect(dialog, &TextInputDialog::accepted, this, [=]() {
                    mModel->renameFile(index, dialog->text());
                });
                dialog->show();
            });
        }

        contextMenu.exec(QCursor::pos());
    }
}
