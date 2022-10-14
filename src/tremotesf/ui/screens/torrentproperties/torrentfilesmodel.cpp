// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfilesmodel.h"

#include <QApplication>
#include <QFutureWatcher>
#include <QStringBuilder>
#include <QStyle>
#include <QtConcurrentRun>

#include "libtremotesf/log.h"
#include "libtremotesf/torrent.h"
#include "libtremotesf/serversettings.h"
#include "tremotesf/rpc/trpc.h"

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
                        auto* childFile = currentDirectory->addFile(static_cast<int>(fileIndex),
                                                                    part,
                                                                    file.size);
                        updateFile(childFile, file);
                        childFile->setChanged(false);
                        treeFiles.push_back(childFile);
                    } else {
                        const auto& childrenHash = currentDirectory->childrenHash();
                        const auto found = childrenHash.find(part);
                        if (found != childrenHash.end()) {
                            currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                        } else {
                            currentDirectory = currentDirectory->addDirectory(part);
                        }
                    }
                }
            }

            return {std::move(rootDirectory), std::move(treeFiles)};
        }
    }

    TorrentFilesModel::TorrentFilesModel(libtremotesf::Torrent* torrent, Rpc* rpc, QObject* parent)
        : BaseTorrentFilesModel({Column::Name,
                                 Column::Size,
                                 Column::ProgressBar,
                                 Column::Progress,
                                 Column::Priority}, parent)
    {
        setRpc(rpc);
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

            if (mTorrent) {
                QObject::connect(mTorrent, &libtremotesf::Torrent::filesUpdated, this, &TorrentFilesModel::update);
                QObject::connect(mTorrent, &libtremotesf::Torrent::fileRenamed, this, &TorrentFilesModel::fileRenamed);
                if (mTorrent->isFilesEnabled()) {
                    logWarning("{} already has enabled files, this shouldn't happen", *mTorrent);
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
        mRpc = rpc;
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
        const auto parts = path.split('/', Qt::SkipEmptyParts);
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
            path += ".part"_l1;
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
        beginResetModel();

        const auto future = QtConcurrent::run(doCreateTree, mTorrent->files());

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=] {
            auto [rootDirectory, files] = watcher->result();

            mRootDirectory = std::move(rootDirectory);
            endResetModel();

            mFiles = std::move(files);

            setLoaded(true);
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
        mLoaded = loaded;
    }
}
