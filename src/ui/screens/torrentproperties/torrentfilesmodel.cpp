// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfilesmodel.h"

#include <algorithm>

#include <QApplication>
#include <QFutureWatcher>
#include <QStringBuilder>
#include <QStyle>
#include <QtConcurrentRun>

#include "log/log.h"
#include "rpc/mounteddirectoriesutils.h"
#include "rpc/torrent.h"
#include "rpc/serversettings.h"
#include "rpc/rpc.h"

namespace tremotesf {
    namespace {
        void updateFile(TorrentFilesModelFile* treeFile, const TorrentFile& file) {
            treeFile->setCompletedSize(file.completedSize);
            treeFile->setWanted(file.wanted);
            treeFile->setPriority(TorrentFilesModelEntry::fromFilePriority(file.priority));
        }

        std::vector<int> idsFromIndex(const QModelIndex& index) {
            auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
            if (entry->isDirectory()) {
                return static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds();
            }
            return {static_cast<TorrentFilesModelFile*>(entry)->id()};
        }

        std::vector<int> idsFromIndexes(const QList<QModelIndex>& indexes) {
            std::vector<int> ids{};
            // at least indexes.size(), but may be more
            ids.reserve(static_cast<size_t>(indexes.size()));
            for (const QModelIndex& index : indexes) {
                auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                if (entry->isDirectory()) {
                    const auto childrenIds = static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds();
                    ids.reserve(ids.size() + childrenIds.size());
                    ids.insert(ids.end(), childrenIds.begin(), childrenIds.end());
                } else {
                    ids.push_back(static_cast<TorrentFilesModelFile*>(entry)->id());
                }
            }
            std::ranges::sort(ids);
            const auto toErase = std::ranges::unique(ids);
            ids.erase(toErase.begin(), toErase.end());
            return ids;
        }

        struct CreateTreeResult {
            std::shared_ptr<TorrentFilesModelDirectory> rootDirectory;
            std::vector<TorrentFilesModelFile*> files;
        };

        CreateTreeResult doCreateTree(const std::vector<TorrentFile>& files) {
            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> treeFiles{};

            for (size_t fileIndex = 0, filesCount = files.size(); fileIndex < filesCount; ++fileIndex) {
                const TorrentFile& file = files[fileIndex];

                TorrentFilesModelDirectory* currentDirectory = rootDirectory.get();

                const std::vector<QString>& parts = file.path;

                for (size_t partIndex = 0, partsCount = parts.size(), lastPartIndex = partsCount - 1;
                     partIndex < partsCount;
                     ++partIndex) {
                    const QString& part = parts[partIndex];

                    if (partIndex == lastPartIndex) {
                        currentDirectory->addFile(TorrentFilesModelFile(
                            static_cast<int>(fileIndex),
                            currentDirectory->childrenCount(),
                            currentDirectory,
                            part,
                            file.size,
                            file.completedSize,
                            file.wanted ? TorrentFilesModelEntry::WantedState::Wanted
                                        : TorrentFilesModelEntry::WantedState::Unwanted,
                            TorrentFilesModelEntry::fromFilePriority(file.priority)
                        ));
                    } else {
                        auto* childForName = currentDirectory->childForName(part);
                        if (childForName) {
                            if (childForName->isDirectory()) {
                                currentDirectory = static_cast<TorrentFilesModelDirectory*>(childForName);
                            } else {
                                throw std::runtime_error(fmt::format(
                                    "Path element at index {} for file at index {} was already added as a file",
                                    partIndex,
                                    fileIndex
                                ));
                            }
                        } else {
                            currentDirectory = currentDirectory->addDirectory(part);
                        }
                    }
                }
            }

            if (rootDirectory->childrenCount() != 0) {
                auto* child = rootDirectory->childAtRow(0);
                if (child->isDirectory()) {
                    treeFiles.reserve(files.size());
                    static_cast<TorrentFilesModelDirectory*>(child)->initiallyCalculateFromAllChildrenRecursively(
                        treeFiles
                    );
                }
            }

            return {std::move(rootDirectory), std::move(treeFiles)};
        }

        using FutureWatcher = QFutureWatcher<std::optional<CreateTreeResult>>;
    }

    TorrentFilesModel::TorrentFilesModel(Torrent* torrent, Rpc* rpc, QObject* parent)
        : BaseTorrentFilesModel(
              {Column::Name, Column::Size, Column::ProgressBar, Column::Progress, Column::Priority}, parent
          ) {
        setRpc(rpc);
        setTorrent(torrent);
    }

    TorrentFilesModel::~TorrentFilesModel() {
        if (mTorrent) {
            mTorrent->setFilesEnabled(false);
        }
    }

    Torrent* TorrentFilesModel::torrent() const { return mTorrent; }

    void TorrentFilesModel::setTorrent(Torrent* torrent) {
        if (torrent != mTorrent) {
            if (const auto oldTorrent = mTorrent.data(); oldTorrent) {
                QObject::disconnect(oldTorrent, nullptr, this, nullptr);
            }

            mTorrent = torrent;

            if (mTorrent) {
                QObject::connect(mTorrent, &Torrent::filesUpdated, this, &TorrentFilesModel::update);
                QObject::connect(mTorrent, &Torrent::fileRenamed, this, &TorrentFilesModel::fileRenamed);
                if (mTorrent->isFilesEnabled()) {
                    warning().log("{} already has enabled files, this shouldn't happen", *mTorrent);
                }
                mTorrent->setFilesEnabled(true);
            } else {
                resetTree();
            }
        }
    }

    Rpc* TorrentFilesModel::rpc() const { return mRpc; }

    void TorrentFilesModel::setRpc(Rpc* rpc) { mRpc = rpc; }

    void TorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted) {
        BaseTorrentFilesModel::setFileWanted(index, wanted);
        mTorrent->setFilesWanted(idsFromIndex(index), wanted);
    }

    void TorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted) {
        BaseTorrentFilesModel::setFilesWanted(indexes, wanted);
        mTorrent->setFilesWanted(idsFromIndexes(indexes), wanted);
    }

    void TorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority) {
        BaseTorrentFilesModel::setFilePriority(index, priority);
        mTorrent->setFilesPriority(idsFromIndex(index), TorrentFilesModelEntry::toFilePriority(priority));
    }

    void
    TorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority) {
        BaseTorrentFilesModel::setFilesPriority(indexes, priority);
        mTorrent->setFilesPriority(idsFromIndexes(indexes), TorrentFilesModelEntry::toFilePriority(priority));
    }

    void TorrentFilesModel::renameFile(const QModelIndex& index, const QString& newName) {
        mTorrent->renameFile(static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->path(), newName);
    }

    void TorrentFilesModel::fileRenamed(const QString& path, const QString& newName) {
        if (!mLoaded || !mRootDirectory) {
            return;
        }
        TorrentFilesModelEntry* entry = mRootDirectory.get();
        const auto parts = path.split('/', Qt::SkipEmptyParts);
        for (const QString& part : parts) {
            if (!entry->isDirectory()) {
                return;
            }
            entry = static_cast<TorrentFilesModelDirectory*>(entry)->childForName(part);
        }
        BaseTorrentFilesModel::fileRenamed(entry, newName);
    }

    QString TorrentFilesModel::localFilePath(const QModelIndex& index) const {
        if (!index.isValid()) {
            return localTorrentDownloadDirectoryPath(mRpc, mTorrent);
        }
        if (mTorrent->data().singleFile) {
            return localTorrentRootFilePath(mRpc, mTorrent);
        }
        const auto* entry = static_cast<const TorrentFilesModelEntry*>(index.internalPointer());
        QString path(entry->path());
        if (!entry->isDirectory() && entry->progress() < 1 && mRpc->serverSettings()->data().renameIncompleteFiles) {
            path += ".part"_l1;
        }
        return localTorrentDownloadDirectoryPath(mRpc, mTorrent) % '/' % path;
    }

    bool TorrentFilesModel::isWanted(const QModelIndex& index) const {
        if (!index.isValid()) {
            return true;
        }
        return static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->wantedState() !=
               TorrentFilesModelEntry::WantedState::Unwanted;
    }

    void TorrentFilesModel::update(std::span<const int> changed) {
        if (mLoaded) {
            updateTree(changed);
        } else {
            createTree();
        }
    }

    void TorrentFilesModel::createTree() {
        if (mCreatingTree) {
            return;
        }

        mCreatingTree = true;
        beginResetModel();

        const auto future =
            QtConcurrent::run([files = std::vector(mTorrent->files())]() -> std::optional<CreateTreeResult> {
                try {
                    return doCreateTree(files);
                } catch (const std::exception& e) {
                    warning().logWithException(e, "Failed to create files tree");
                    return std::nullopt;
                }
            });

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=, this] {
            auto result = watcher->result();
            if (result) {
                auto [rootDirectory, files] = *(std::move(result));
                mRootDirectory = std::move(rootDirectory);
                mFiles = std::move(files);
                setLoaded(true);
            }
            endResetModel();
            mCreatingTree = false;
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    void TorrentFilesModel::resetTree() {
        if (mLoaded) {
            beginResetModel();
            mRootDirectory.reset();
            endResetModel();
            mFiles.clear();
            setLoaded(false);
        }
    }

    void TorrentFilesModel::updateTree(std::span<const int> changed) {
        if (changed.empty()) {
            return;
        }
        const auto& torrentFiles = mTorrent->files();
        if (torrentFiles.size() != mFiles.size()) {
            warning().log(
                "Torrent has {} files, but we have already created tree with {} files",
                torrentFiles.size(),
                mFiles.size()
            );
            return;
        }
        std::vector<TorrentFilesModelDirectory*> directoriesToRecalculate{};
        for (int changedIndex : changed) {
            auto* treeFile = mFiles.at(static_cast<size_t>(changedIndex));
            const auto& rpcFile = torrentFiles.at(static_cast<size_t>(changedIndex));
            updateFile(treeFile, rpcFile);
            addParentDirectoriesToRecalculateList(treeFile, directoriesToRecalculate);
        }
        if (!directoriesToRecalculate.empty()) {
            std::ranges::for_each(directoriesToRecalculate, &TorrentFilesModelDirectory::recalculateFromChildren);
        }
        emitDataChangedForChildren();
    }

    void TorrentFilesModel::setLoaded(bool loaded) { mLoaded = loaded; }
}
