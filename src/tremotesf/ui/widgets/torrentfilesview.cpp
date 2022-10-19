// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfilesview.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QCursor>
#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>

#include "libtremotesf/serversettings.h"

#include "tremotesf/desktoputils.h"
#include "tremotesf/filemanagerlauncher.h"
#include "tremotesf/settings.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/ui/itemmodels/torrentfilesmodelentry.h"
#include "tremotesf/ui/itemmodels/torrentfilesproxymodel.h"
#include "tremotesf/ui/screens/addtorrent/localtorrentfilesmodel.h"
#include "tremotesf/ui/screens/torrentproperties/torrentfilesmodel.h"

#include "commondelegate.h"
#include "textinputdialog.h"

namespace tremotesf {
    TorrentFilesView::TorrentFilesView(LocalTorrentFilesModel* model, Rpc* rpc, QWidget* parent)
        : BaseTreeView(parent),
          mLocalFile(true),
          mModel(model),
          mProxyModel(new TorrentFilesProxyModel(mModel, LocalTorrentFilesModel::SortRole, this)),
          mRpc(rpc) {
        init();
        setItemDelegate(new CommonDelegate(this));
        if (!header()->restoreState(Settings::instance()->localTorrentFilesViewHeaderState())) {
            sortByColumn(static_cast<int>(LocalTorrentFilesModel::Column::Name), Qt::AscendingOrder);
        }
    }

    TorrentFilesView::TorrentFilesView(TorrentFilesModel* model, Rpc* rpc, QWidget* parent)
        : BaseTreeView(parent),
          mLocalFile(false),
          mModel(model),
          mProxyModel(new TorrentFilesProxyModel(mModel, TorrentFilesModel::SortRole, this)),
          mRpc(rpc) {
        init();
        setItemDelegate(new CommonDelegate(
            static_cast<int>(TorrentFilesModel::Column::ProgressBar),
            TorrentFilesModel::SortRole,
            -1,
            this
        ));
        if (!header()->restoreState(Settings::instance()->torrentFilesViewHeaderState())) {
            sortByColumn(static_cast<int>(TorrentFilesModel::Column::Name), Qt::AscendingOrder);
        }

        QObject::connect(this, &TorrentFilesView::activated, this, [=](const auto& index) {
            const QModelIndex sourceIndex(mProxyModel->sourceIndex(index));
            auto entry = static_cast<const TorrentFilesModelEntry*>(mProxyModel->sourceIndex(index).internalPointer());
            if (!entry->isDirectory() &&
                mRpc->isTorrentLocalMounted(static_cast<const TorrentFilesModel*>(mModel)->torrent()) &&
                entry->wantedState() != TorrentFilesModelEntry::Unwanted) {
                desktoputils::openFile(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(sourceIndex), this);
            }
        });
    }

    TorrentFilesView::~TorrentFilesView() {
        if (mLocalFile) {
            Settings::instance()->setLocalTorrentFilesViewHeaderState(header()->saveState());
        } else {
            Settings::instance()->setTorrentFilesViewHeaderState(header()->saveState());
        }
    }

    void TorrentFilesView::showFileRenameDialog(
        const QString& fileName, QWidget* parent, const std::function<void(const QString&)>& onAccepted
    ) {
        auto dialog = new TextInputDialog(
            qApp->translate("tremotesf", "Rename"),
            qApp->translate("tremotesf", "File name:"),
            fileName,
            qApp->translate("tremotesf", "Rename"),
            false,
            parent
        );
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(dialog, &QDialog::accepted, parent, [=] { onAccepted(dialog->text()); });
        dialog->show();
    }

    void TorrentFilesView::init() {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setModel(mProxyModel);
        setSelectionMode(QAbstractItemView::ExtendedSelection);

        QObject::connect(mModel, &BaseTorrentFilesModel::modelReset, this, &TorrentFilesView::onModelReset);
        QObject::connect(this, &TorrentFilesView::customContextMenuRequested, this, &TorrentFilesView::showContextMenu);

        onModelReset();
    }

    void TorrentFilesView::onModelReset() {
        if (mModel->rowCount() > 0) {
            if (mModel->rowCount(mModel->index(0, 0)) == 0) {
                setRootIsDecorated(false);
            } else {
                setRootIsDecorated(true);
                expand(mProxyModel->index(0, 0));
            }
        }
    }

    void TorrentFilesView::showContextMenu(QPoint pos) {
        if (!indexAt(pos).isValid()) { return; }

        const QModelIndexList sourceIndexes(mProxyModel->sourceIndexes(selectionModel()->selectedRows()));

        QMenu contextMenu;

        if (!mLocalFile) {
            bool show = true;
            for (const QModelIndex& index : sourceIndexes) {
                if (static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->wantedState() ==
                    TorrentFilesModelEntry::Unwanted) {
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
                            if (!QFileInfo::exists(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(index)
                                )) {
                                disableBoth = true;
                                break;
                            }
                        }
                    }
                } else {
                    disableBoth = true;
                }

                QAction* openAction =
                    contextMenu.addAction(QIcon::fromTheme("document-open"_l1), qApp->translate("tremotesf", "&Open"));
                openAction->setEnabled(!disableBoth && !disableOpen);
                QObject::connect(openAction, &QAction::triggered, this, [=, &sourceIndexes] {
                    for (const QModelIndex& index : sourceIndexes) {
                        desktoputils::openFile(
                            static_cast<const TorrentFilesModel*>(mModel)->localFilePath(index),
                            this
                        );
                    }
                });

                QAction* showInFileManagerAction = contextMenu.addAction(
                    QIcon::fromTheme("go-jump"_l1),
                    qApp->translate("tremotesf", "Show In &File Manager")
                );
                showInFileManagerAction->setEnabled(!disableBoth);
                QObject::connect(showInFileManagerAction, &QAction::triggered, this, [=, &sourceIndexes] {
                    std::vector<QString> files{};
                    files.reserve(static_cast<size_t>(sourceIndexes.size()));
                    for (const QModelIndex& index : sourceIndexes) {
                        files.push_back(static_cast<const TorrentFilesModel*>(mModel)->localFilePath(index));
                    }
                    launchFileManagerAndSelectFiles(files, this);
                });
            }
        }

        contextMenu.addSeparator();

        QAction* downloadAction = contextMenu.addAction(
            QIcon::fromTheme("download"_l1),
            qApp->translate("tremotesf", "&Download", "File menu item, verb")
        );
        QObject::connect(downloadAction, &QAction::triggered, this, [=, &sourceIndexes] {
            mModel->setFilesWanted(sourceIndexes, true);
        });

        QAction* notDownloadAction =
            contextMenu.addAction(QIcon::fromTheme("dialog-cancel"_l1), qApp->translate("tremotesf", "&Not Download"));
        QObject::connect(notDownloadAction, &QAction::triggered, this, [=, &sourceIndexes] {
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
            if (checked) { mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntry::HighPriority); }
        });

        //: Priority
        QAction* normalPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "&Normal"));
        normalPriorityAction->setCheckable(true);
        QObject::connect(normalPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) { mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntry::NormalPriority); }
        });

        //: Priority
        QAction* lowPriorityAction = priorityGroup.addAction(qApp->translate("tremotesf", "&Low"));
        lowPriorityAction->setCheckable(true);
        QObject::connect(lowPriorityAction, &QAction::triggered, this, [=, &sourceIndexes](bool checked) {
            if (checked) { mModel->setFilesPriority(sourceIndexes, TorrentFilesModelEntry::LowPriority); }
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
            QAction* renameAction =
                contextMenu.addAction(QIcon::fromTheme("edit-rename"_l1), qApp->translate("tremotesf", "&Rename"));
            renameAction->setEnabled(sourceIndexes.size() == 1);
            QObject::connect(renameAction, &QAction::triggered, this, [=] {
                const QModelIndex& index = sourceIndexes.first();
                auto entry = static_cast<const TorrentFilesModelEntry*>(index.internalPointer());
                showFileRenameDialog(entry->name(), this, [=](const auto& newName) {
                    mModel->renameFile(index, newName);
                });
            });
        }

        contextMenu.exec(QCursor::pos());
    }
}
