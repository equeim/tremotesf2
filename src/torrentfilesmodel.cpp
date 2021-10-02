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

#include "libtremotesf/torrent.h"
#include "libtremotesf/torrent_qdebug.h"
#include "libtremotesf/serversettings.h"
#include "trpc.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        void updateFile(TorrentFilesModelFile* treeFile, const libtremotesf::TorrentFile& file)
        {
            treeFile->setChanged(false);
            treeFile->setCompletedSize(file.completedSize);
            treeFile->setWanted(file.wanted);
            treeFile->setPriority(TorrentFilesModelEntry::fromFilePriority(file.priority));
        }

        QVariantList idsFromIndex(const QModelIndex& index)
        {
            auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
            if (entry->isDirectory()) {
                return static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds();
            }
            return {static_cast<TorrentFilesModelFile*>(entry)->id()};
        }

        QVariantList idsFromIndexes(const QList<QModelIndex>& indexes)
        {
            QVariantList ids;
            // at least indexes.size(), but may be more
            ids.reserve(indexes.size());
            for (const QModelIndex& index : indexes) {
                auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                if (entry->isDirectory()) {
                    ids.append(static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds());
                } else {
                    ids.append(static_cast<TorrentFilesModelFile*>(entry)->id());
                }
            }
            std::sort(ids.begin(), ids.end(), [](const auto& a, const auto& b) { return a.toInt() < b.toInt(); });
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            return ids;
        }

        using FutureWatcher = QFutureWatcher<std::pair<std::shared_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>>;

        std::pair<std::shared_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>
        doCreateTree(const std::vector<libtremotesf::TorrentFile>& files)
        {
            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> treeFiles;
            treeFiles.reserve(files.size());

            for (size_t fileIndex = 0, filesCount = files.size(); fileIndex < filesCount; ++fileIndex) {
                const libtremotesf::TorrentFile& file = files[fileIndex];

                TorrentFilesModelDirectory* currentDirectory = rootDirectory.get();

                const std::vector<QString> parts(file.path);

                for (size_t partIndex = 0, partsCount = parts.size(), lastPartIndex = partsCount - 1; partIndex < partsCount; ++partIndex) {
                    const QString& part = parts[partIndex];

                    if (partIndex == lastPartIndex) {
                        auto childFile = new TorrentFilesModelFile(static_cast<int>(currentDirectory->children().size()),
                                                                   currentDirectory,
                                                                   static_cast<int>(fileIndex),
                                                                   part,
                                                                   file.size);

                        updateFile(childFile, file);
                        childFile->setChanged(false);
                        currentDirectory->addChild(childFile);
                        treeFiles.push_back(childFile);
                    } else {
                        const auto& childrenHash = currentDirectory->childrenHash();
                        const auto found = childrenHash.find(part);
                        if (found != childrenHash.end()) {
                            currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                        } else {
                            auto childDirectory = new TorrentFilesModelDirectory(static_cast<int>(currentDirectory->children().size()),
                                                                                 currentDirectory,
                                                                                 part);
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
#ifdef TREMOTESF_SAILFISHOS
        : BaseTorrentFilesModel(parent),
#else
        : BaseTorrentFilesModel({NameColumn,
                                 SizeColumn,
                                 ProgressBarColumn,
                                 ProgressColumn,
                                 PriorityColumn}, parent),
#endif
          mTorrent(nullptr),
          mRpc(rpc),
          mLoaded(false),
          mLoading(true),
          mCreatingTree(false)
    {
        setTorrent(torrent);
    }

    TorrentFilesModel::~TorrentFilesModel()
    {
        if (mTorrent) {
            mTorrent->setFilesEnabled(false);
        }
    }

    libtremotesf::Torrent* TorrentFilesModel::torrent() const
    {
        return mTorrent;
    }

    void TorrentFilesModel::setTorrent(libtremotesf::Torrent* torrent)
    {
        if (torrent != mTorrent) {
            if (mTorrent) {
                QObject::disconnect(mTorrent, nullptr, this, nullptr);
            }

            mTorrent = torrent;
            emit torrentChanged();

            if (mTorrent) {
                QObject::connect(mTorrent, &libtremotesf::Torrent::filesUpdated, this, &TorrentFilesModel::update);
                QObject::connect(mTorrent, &libtremotesf::Torrent::fileRenamed, this, &TorrentFilesModel::fileRenamed);
                if (mTorrent->isFilesEnabled()) {
                    qWarning() << mTorrent << "already has enabled files, this shouldn't happen";
                }
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
        if (rpc != mRpc) {
            mRpc = rpc;
            emit rpcChanged();
        }
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
        BaseTorrentFilesModel::setFileWanted(index, wanted);
        mTorrent->setFilesWanted(idsFromIndex(index), wanted);
    }

    void TorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted)
    {
        BaseTorrentFilesModel::setFilesWanted(indexes, wanted);
        mTorrent->setFilesWanted(idsFromIndexes(indexes), wanted);
    }

    void TorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority)
    {
        BaseTorrentFilesModel::setFilePriority(index, priority);
        mTorrent->setFilesPriority(idsFromIndex(index), TorrentFilesModelEntry::toFilePriority(priority));
    }

    void TorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority)
    {
        BaseTorrentFilesModel::setFilesPriority(indexes, priority);
        mTorrent->setFilesPriority(idsFromIndexes(indexes), TorrentFilesModelEntry::toFilePriority(priority));
    }

    void TorrentFilesModel::renameFile(const QModelIndex& index, const QString& newName)
    {
        mTorrent->renameFile(static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->path(), newName);
    }

    void TorrentFilesModel::fileRenamed(const QString& path, const QString& newName)
    {
        if (!mLoaded || !mRootDirectory) {
            return;
        }
        TorrentFilesModelEntry* entry = mRootDirectory.get();
        const auto parts = path.split('/',
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                                      Qt::SkipEmptyParts);
#else
                                      QString::SkipEmptyParts);
#endif
        for (const QString& part : parts) {
            entry = static_cast<const TorrentFilesModelDirectory*>(entry)->childrenHash().at(part);
        }
        BaseTorrentFilesModel::fileRenamed(entry, newName);
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

    void TorrentFilesModel::update(const std::vector<int>& changed)
    {
        if (mLoaded) {
            updateTree(changed);
        } else {
            createTree();
        }
    }

    void TorrentFilesModel::createTree()
    {
        if (mCreatingTree) {
            return;
        }

        mCreatingTree = true;
        setLoading(true);
        beginResetModel();

        const auto future = QtConcurrent::run(doCreateTree, mTorrent->files());

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=] {
            auto [rootDirectory, files] = watcher->result();

            mRootDirectory = std::move(rootDirectory);
            endResetModel();

            mFiles = std::move(files);

            setLoaded(true);
            setLoading(false);
            mCreatingTree = false;

            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    void TorrentFilesModel::resetTree()
    {
        if (mLoaded) {
            beginResetModel();
            mRootDirectory.reset();
            endResetModel();
            mFiles.clear();
            setLoaded(false);
        }
    }

    void TorrentFilesModel::updateTree(const std::vector<int>& changed)
    {
        if (!changed.empty()) {
            const auto& torrentFiles = mTorrent->files();

            auto changedIter(changed.begin());
            int changedIndex = *changedIter;
            const auto changedEnd(changed.end());

            for (int i = 0, max = static_cast<int>(mFiles.size()); i < max; ++i) {
                const auto& file = mFiles[static_cast<size_t>(i)];
                if (i == changedIndex) {
                    updateFile(file, torrentFiles[static_cast<size_t>(changedIndex)]);
                    ++changedIter;
                    if (changedIter == changedEnd) {
                        changedIndex = -1;
                    } else {
                        changedIndex = *changedIter;
                    }
                } else {
                    file->setChanged(false);
                }
            }
            updateDirectoryChildren();
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
