// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfilesmodel.h"

#include <QStringBuilder>

#include "coroutines/threadpool.h"
#include "log/log.h"
#include "rpc/mounteddirectoriesutils.h"
#include "rpc/torrent.h"
#include "rpc/serversettings.h"
#include "rpc/rpc.h"
#include "ui/itemmodels/torrentfilestreebuilder.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        std::vector<int> idsFromIndexes(const QList<QModelIndex>& indexes) {
            std::vector<int> ids{};
            // at least indexes.size(), but may be more
            ids.reserve(static_cast<size_t>(indexes.size()));
            for (const QModelIndex& index : indexes) {
                auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                entry->getFileIds(ids);
            }
            std::ranges::sort(ids);
            const auto toErase = std::ranges::unique(ids);
            ids.erase(toErase.begin(), toErase.end());
            return ids;
        }

        std::pair<std::unique_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>
        doCreateTree(const std::vector<TorrentFile>& files) {
            auto rootDirectory = std::make_unique<TorrentFilesModelDirectory>();
            TorrentFilesTreeBuilder builder(rootDirectory.get(), files.size());
            for (const TorrentFile& file : files) {
                builder.addFile(
                    file.pathParts(),
                    file.size,
                    file.completedSize,
                    file.wanted,
                    TorrentFilesModelEntry::fromFilePriority(file.priority)
                );
            }
            builder.calculateDirectoriesRecursively();
            return {std::move(rootDirectory), std::move(builder.files)};
        }
    }

    TorrentFilesModel::TorrentFilesModel(Rpc* rpc, QObject* parent)
        : BaseTorrentFilesModel(
              {Column::Name, Column::Size, Column::ProgressBar, Column::Progress, Column::Priority}, parent
          ) {
        setRpc(rpc);
    }

    TorrentFilesModel::~TorrentFilesModel() {
        if (mTorrent) {
            mTorrent->setFilesEnabled(false);
        }
    }

    Torrent* TorrentFilesModel::torrent() const { return mTorrent; }

    void TorrentFilesModel::setTorrent(Torrent* torrent, bool oldTorrentDestroyed) {
        if (torrent == mTorrent) {
            return;
        }
        if (mTorrent && !oldTorrentDestroyed) {
            QObject::disconnect(mTorrent, nullptr, this, nullptr);
            mTorrent->setFilesEnabled(false);
        }
        mTorrent = torrent;
        resetTree();
        if (mTorrent) {
            QObject::connect(mTorrent, &Torrent::filesUpdated, this, &TorrentFilesModel::update);
            QObject::connect(mTorrent, &Torrent::fileRenamed, this, &TorrentFilesModel::fileRenamed);
            if (mTorrent->isFilesEnabled()) {
                warning().log("{} already has enabled files, this shouldn't happen", *mTorrent);
            }
            mTorrent->setFilesEnabled(true);
            QObject::connect(mTorrent, &QObject::destroyed, this, [this] { setTorrent(nullptr, true); });
        }
    }

    Rpc* TorrentFilesModel::rpc() const { return mRpc; }

    void TorrentFilesModel::setRpc(Rpc* rpc) { mRpc = rpc; }

    void TorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted) {
        BaseTorrentFilesModel::setFilesWanted(indexes, wanted);
        mTorrent->setFilesWanted(idsFromIndexes(indexes), wanted);
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
            if (!entry->isDirectory()) return;
            const auto& children = static_cast<const TorrentFilesModelDirectory*>(entry)->children();
            const auto found = std::ranges::find(children, part, &TorrentFilesModelEntry::name);
            if (found == children.end()) return;
            entry = found->get();
        }
        BaseTorrentFilesModel::fileRenamed(entry, newName);
    }

    QString TorrentFilesModel::localFilePath(const QModelIndex& index) const {
        if (!index.isValid()) {
            return localTorrentDownloadDirectoryPath(mRpc, mTorrent);
        }
        const auto* entry = static_cast<const TorrentFilesModelEntry*>(index.internalPointer());
        QString path(entry->path());
        if (!entry->isDirectory() && entry->progress() < 1 && mRpc->serverSettings()->data().renameIncompleteFiles) {
            path += ".part"_L1;
        }
        return localTorrentDownloadDirectoryPath(mRpc, mTorrent) % '/' % path;
    }

    bool TorrentFilesModel::isWanted(const QModelIndex& index) const {
        if (!index.isValid()) {
            return true;
        }
        return static_cast<const TorrentFilesModelEntry*>(index.internalPointer())->wantedState()
               != TorrentFilesModelEntry::WantedState::Unwanted;
    }

    void TorrentFilesModel::update(std::span<const int> changed) {
        if (mLoaded) {
            updateTree(changed);
        } else {
            mCoroutineScope.launch(createTree());
        }
    }

    Coroutine<> TorrentFilesModel::createTree() {
        if (mCreatingTree) {
            co_return;
        }

        mCreatingTree = true;
        beginResetModel();

        auto [rootDirectory, files] = co_await runOnThreadPool(
            [](const std::vector<TorrentFile>& files) { return doCreateTree(files); },
            mTorrent->files()
        );

        mRootDirectory = std::move(rootDirectory);
        endResetModel();

        mFiles = std::move(files);

        setLoaded(true);
        mCreatingTree = false;
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
        const auto& jsons = mTorrent->files();
        updateFiles(changed, [&](size_t i, TorrentFilesModelFile* file) {
            const auto& json = jsons.at(i);
            file->update(json.wanted, TorrentFilesModelEntry::fromFilePriority(json.priority), json.completedSize);
        });
    }

    void TorrentFilesModel::setLoaded(bool loaded) { mLoaded = loaded; }
}
