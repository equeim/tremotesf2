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

#include "torrentfilesmodel.h"

#include <QFutureWatcher>
#include <QStringBuilder>
#include <QtConcurrentRun>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "libtremotesf/serversettings.h"
#include "rpc.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        const QString nameKey(QLatin1String("name"));
        const QString sizeKey(QLatin1String("length"));
        const QString completedSizeKey(QLatin1String("bytesCompleted"));
        const QString wantedKey(QLatin1String("wanted"));
        const QString priorityKey(QLatin1String("priority"));

        void updateFile(TorrentFilesModelFile* treeFile, const libtremotesf::TorrentFile* file)
        {
            treeFile->setChanged(false);
            treeFile->setCompletedSize(file->completedSize);
            treeFile->setWanted(file->wanted);
            treeFile->setPriority(TorrentFilesModelEntry::fromFilePriority(file->priority));
        }

        QVariantList idsFromIndex(const QModelIndex& index)
        {
            TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
            if (entry->isDirectory()) {
                return static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds();
            }
            return {static_cast<TorrentFilesModelFile*>(entry)->id()};
        }

        QVariantList idsFromIndexes(const QList<QModelIndex>& indexes)
        {
            QVariantList ids;
            for (const QModelIndex& index : indexes) {
                TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                if (entry->isDirectory()) {
                    ids.append(static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds());
                } else {
                    ids.append(static_cast<TorrentFilesModelFile*>(entry)->id());
                }
            }
            std::sort(ids.begin(), ids.end());
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            return ids;
        }

        using FutureWatcher = QFutureWatcher<std::pair<std::shared_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>>;

        std::pair<std::shared_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>
        doCreateTree(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files)
        {
            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> treeFiles;

            for (int fileIndex = 0, filesCount = files.size(); fileIndex < filesCount; ++fileIndex) {
                const libtremotesf::TorrentFile* file = files[fileIndex].get();

                TorrentFilesModelDirectory* currentDirectory = rootDirectory.get();

                const std::vector<QString> parts(file->path);
                QString path;

                for (int partIndex = 0, partsCount = parts.size(), lastPartIndex = partsCount - 1; partIndex < partsCount; ++partIndex) {
                    const QString& part = parts[partIndex];
                    if (partIndex > 0) {
                        path += '/';
                    }
                    path += part;

                    if (partIndex == lastPartIndex) {
                        auto childFile = new TorrentFilesModelFile(currentDirectory->children().size(),
                                                                   currentDirectory,
                                                                   fileIndex,
                                                                   part,
                                                                   path,
                                                                   file->size);

                        updateFile(childFile, file);
                        currentDirectory->addChild(childFile);
                        treeFiles.push_back(childFile);
                    } else {
                        const auto& childrenHash = currentDirectory->childrenHash();
                        const auto found = childrenHash.find(part);
                        if (found != childrenHash.end()) {
                            currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                        } else {
                            auto childDirectory = new TorrentFilesModelDirectory(currentDirectory->children().size(),
                                                                                 currentDirectory,
                                                                                 part,
                                                                                 path);
                            currentDirectory->addChild(childDirectory);
                            currentDirectory = childDirectory;
                        }
                    }
                }
            }

            return {std::move(rootDirectory), std::move(treeFiles)};
        }
    }

    TorrentFilesModel::TorrentFilesModel(libtremotesf::Torrent* torrent, Rpc* rpc, QObject* parent)
        : BaseTorrentFilesModel(parent),
          mTorrent(nullptr),
          mRpc(rpc),
          mLoaded(false),
          mLoading(true),
          mCreatingTree(false),
          mResetAfterCreate(false),
          mUpdateAfterCreate(false)
    {
        setTorrent(torrent);
    }

    TorrentFilesModel::~TorrentFilesModel()
    {
        if (mTorrent) {
            mTorrent->setFilesEnabled(false);
        }
    }

    int TorrentFilesModel::columnCount(const QModelIndex&) const
    {
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return ColumnCount;
#endif
    }

    QVariant TorrentFilesModel::data(const QModelIndex& index, int role) const
    {
        const TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return entry->name();
        case IsDirectoryRole:
            return entry->isDirectory();
        case CompletedSizeRole:
            return entry->completedSize();
        case SizeRole:
            return entry->size();
        case ProgressRole:
            return entry->progress();
        case WantedStateRole:
            return entry->wantedState();
        case PriorityRole:
            return entry->priority();
        }
#else
        switch (role) {
        case Qt::CheckStateRole:
            if (index.column() == NameColumn) {
                switch (entry->wantedState()) {
                case TorrentFilesModelEntry::Wanted:
                    return Qt::Checked;
                case TorrentFilesModelEntry::Unwanted:
                    return Qt::Unchecked;
                case TorrentFilesModelEntry::MixedWanted:
                    return Qt::PartiallyChecked;
                }
            }
            break;
        case Qt::DecorationRole:
            if (index.column() == NameColumn) {
                if (entry->isDirectory()) {
                    return qApp->style()->standardIcon(QStyle::SP_DirIcon);
                }
                return qApp->style()->standardIcon(QStyle::SP_FileIcon);
            }
            break;
        case Qt::DisplayRole:
            switch (index.column()) {
            case NameColumn:
                return entry->name();
            case SizeColumn:
                return Utils::formatByteSize(entry->size());
            case ProgressColumn:
                return Utils::formatProgress(entry->progress());
            case PriorityColumn:
                return entry->priorityString();
            }
            break;
        case SortRole:
            switch (index.column()) {
            case SizeColumn:
                return entry->size();
            case ProgressBarColumn:
            case ProgressColumn:
                return entry->progress();
            case PriorityColumn:
                return entry->priority();
            default:
                return data(index, Qt::DisplayRole);
            }
        }
#endif
        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    Qt::ItemFlags TorrentFilesModel::flags(const QModelIndex& index) const
    {
        if (index.column() == NameColumn) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        }
        return QAbstractItemModel::flags(index);
    }

    QVariant TorrentFilesModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case NameColumn:
                return qApp->translate("tremotesf", "Name");
            case SizeColumn:
                return qApp->translate("tremotesf", "Size");
            case ProgressBarColumn:
                return qApp->translate("tremotesf", "Progress Bar");
            case ProgressColumn:
                return qApp->translate("tremotesf", "Progress");
            case PriorityColumn:
                return qApp->translate("tremotesf", "Priority");
            }
        }
        return QVariant();
    }

    bool TorrentFilesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (index.column() == NameColumn && role == Qt::CheckStateRole) {
            setFileWanted(index, (value.toInt() == Qt::Checked));
            return true;
        }
        return false;
    }
#endif

    libtremotesf::Torrent* TorrentFilesModel::torrent() const
    {
        return mTorrent;
    }

    void TorrentFilesModel::setTorrent(libtremotesf::Torrent* torrent)
    {
        if (torrent != mTorrent) {
            mTorrent = torrent;
            if (mTorrent) {
                QObject::connect(mTorrent, &libtremotesf::Torrent::filesUpdated, this, &TorrentFilesModel::update);
                QObject::connect(mTorrent, &libtremotesf::Torrent::fileRenamed, this, &TorrentFilesModel::fileRenamed);
                mTorrent->setFilesEnabled(true);
            } else {
                resetTree();
            }
        }
    }

    Rpc* TorrentFilesModel::rpc() const
    {
        return mRpc;
    }

    void TorrentFilesModel::setRpc(Rpc* rpc)
    {
        mRpc = rpc;
    }

    bool TorrentFilesModel::isLoaded() const
    {
        return mLoaded;
    }

    bool TorrentFilesModel::isLoading() const
    {
        return mLoading;
    }

    void TorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesWanted(idsFromIndex(index), wanted);
    }

    void TorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        }
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesWanted(idsFromIndexes(indexes), wanted);
    }

    void TorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority)
    {
        qDebug() << priority;
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesPriority(idsFromIndex(index), TorrentFilesModelEntry::toFilePriority(priority));
    }

    void TorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        }
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesPriority(idsFromIndexes(indexes), TorrentFilesModelEntry::toFilePriority(priority));
    }

    void TorrentFilesModel::renameFile(const QModelIndex& index, const QString& newName) const
    {
        const TorrentFilesModelEntry* entry = static_cast<const TorrentFilesModelEntry*>(index.internalPointer());
        mTorrent->renameFile(entry->path(), newName);
    }

    void TorrentFilesModel::fileRenamed(const QString& path, const QString& newName)
    {
        if (!mLoaded || mCreatingTree) {
            return;
        }
        TorrentFilesModelEntry* entry = mRootDirectory.get();
        for (const QString& part : path.split('/', QString::SkipEmptyParts)) {
            entry = static_cast<const TorrentFilesModelDirectory*>(entry)->childrenHash().at(part);
        }
        entry->setName(newName);
        emit dataChanged(createIndex(entry->row(), 0, entry),
                         createIndex(entry->row(), columnCount() - 1, entry));
    }

    QString TorrentFilesModel::localFilePath(const QModelIndex &index) const
    {
        if (!index.isValid()) {
            return mRpc->localTorrentDownloadDirectoryPath(mTorrent);
        }
        if (mTorrent->isSingleFile()) {
            return mRpc->localTorrentFilesPath(mTorrent);
        }
        const auto* entry = static_cast<const TorrentFilesModelEntry*>(index.internalPointer());
        QString path(entry->path());
        if (!entry->isDirectory() && entry->progress() < 1 && mRpc->serverSettings()->renameIncompleteFiles()) {
            path += QLatin1String(".part");
        }
        return mRpc->localTorrentDownloadDirectoryPath(mTorrent) % '/' % path;
    }

    bool TorrentFilesModel::isWanted(const QModelIndex& index) const
    {
        if (!index.isValid()) {
            return true;
        }
        return static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->wantedState() != TorrentFilesModelEntry::Unwanted;
    }

    void TorrentFilesModel::update(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files)
    {
        mResetAfterCreate = false;
        mUpdateAfterCreate = false;

        if (files.empty()) {
            if (mLoaded) {
                resetTree();
            } else if (mCreatingTree) {
                mResetAfterCreate = true;
            }
        } else {
            if (mLoaded) {
                updateTree(files, true);
            } else if (mCreatingTree) {
                mUpdateAfterCreate = true;
            } else {
                createTree(files);
            }
        }
    }

    void TorrentFilesModel::createTree(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files)
    {
        mCreatingTree = true;
        setLoading(true);
        beginResetModel();

        const auto future = QtConcurrent::run(doCreateTree, files);

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=]() {
            auto result = watcher->result();
            mRootDirectory = std::move(result.first);
            mFiles = std::move(result.second);

            if (mResetAfterCreate) {
                mRootDirectory->clearChildren();
                endResetModel();
                mFiles.clear();
                setLoading(false);
                return;
            }

            if (mUpdateAfterCreate) {
                updateTree(mTorrent->files(), false);
            }

            endResetModel();

            setLoaded(true);
            setLoading(false);

            mCreatingTree = false;
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    void TorrentFilesModel::resetTree()
    {
        beginResetModel();
        endResetModel();
        mRootDirectory->clearChildren();
        endResetModel();
        mFiles.clear();
        setLoaded(false);
    }

    void TorrentFilesModel::updateTree(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files,
                                       bool emitSignal)
    {
        for (int i = 0, size = files.size(); i < size; i++) {
            updateFile(mFiles[i], files[i].get());
        }
        if (emitSignal) {
            updateDirectoryChildren(mRootDirectory.get());
        }
    }

    void TorrentFilesModel::setLoaded(bool loaded)
    {
        if (loaded != mLoaded) {
            mLoaded = loaded;
            emit loadedChanged();
        }
    }

    void TorrentFilesModel::setLoading(bool loading)
    {
        if (loading != mLoading) {
            mLoading = loading;
            emit loadingChanged();
        }
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> TorrentFilesModel::roleNames() const
    {
        return {{NameRole, "name"},
                {IsDirectoryRole, "isDirectory"},
                {CompletedSizeRole, "completedSize"},
                {SizeRole, "size"},
                {ProgressRole, "progress"},
                {WantedStateRole, "wantedState"},
                {PriorityRole, "priority"}};
    }
#endif
}
