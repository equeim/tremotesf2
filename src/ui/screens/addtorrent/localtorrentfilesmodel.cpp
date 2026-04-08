// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localtorrentfilesmodel.h"

#include "coroutines/threadpool.h"
#include "ui/itemmodels/torrentfilesmodelentry.h"
#include "ui/itemmodels/torrentfilestreebuilder.h"
#include "torrentfileparser.h"

namespace tremotesf {
    namespace {
        struct CreateTreeResult {
            std::unique_ptr<TorrentFilesModelEntry> rootEntry;
            std::vector<TorrentFilesModelEntry*> files;
        };

        CreateTreeResult createTree(TorrentMetainfoFile torrentFile) {
            if (torrentFile.isSingleFile() == 1) {
                auto rootEntry = std::make_unique<TorrentFilesModelEntry>(TorrentFilesModelEntry::createFile(
                    0,
                    0,
                    std::move(torrentFile.rootFileName),
                    torrentFile.singleFileSize(),
                    0,
                    true,
                    TorrentFilesModelEntry::Priority::Normal
                ));
                auto rootEntryPtr = rootEntry.get();
                return {.rootEntry = std::move(rootEntry), .files = {rootEntryPtr}};
            }

            auto files = torrentFile.files();
            TorrentFilesTreeBuilder builder(files.size());
            builder.initializeRootDirectory(std::move(torrentFile.rootFileName));
            for (TorrentMetainfoFile::File file : files) {
                builder.addFile(file.path(), false, file.size, 0, true, TorrentFilesModelEntry::Priority::Normal);
            }
            builder.calculateDirectoriesRecursively();
            return {.rootEntry = std::move(builder.rootEntry), .files = std::move(builder.files)};
        }
    }

    LocalTorrentFilesModel::LocalTorrentFilesModel(QObject* parent)
        : BaseTorrentFilesModel({Column::Name, Column::Size, Column::Priority}, parent) {}

    Coroutine<> LocalTorrentFilesModel::load(TorrentMetainfoFile torrentFile) {
        beginResetModel();
        try {
            auto [rootEntry, files] = co_await runOnThreadPool(&createTree, std::move(torrentFile));
            mRootEntry = std::move(rootEntry);
            mFiles = std::move(files);
            mLoaded = true;
            endResetModel();
        } catch (const bencode::Error&) {
            endResetModel();
            throw;
        }
    }

    bool LocalTorrentFilesModel::isLoaded() const { return mLoaded; }

    std::vector<int> LocalTorrentFilesModel::unwantedFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelEntry* file : mFiles) {
            if (file->wantedState() == TorrentFilesModelEntry::WantedState::Unwanted) {
                files.push_back(file->fileId());
            }
        }
        return files;
    }

    std::vector<int> LocalTorrentFilesModel::highPriorityFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelEntry* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::Priority::High) {
                files.push_back(file->fileId());
            }
        }
        return files;
    }

    std::vector<int> LocalTorrentFilesModel::lowPriorityFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelEntry* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::Priority::Low) {
                files.push_back(file->fileId());
            }
        }
        return files;
    }

    const std::map<QString, QString>& LocalTorrentFilesModel::renamedFiles() const { return mRenamedFiles; }

    void LocalTorrentFilesModel::renameFile(const QModelIndex& index, const QString& newName) {
        auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
        mRenamedFiles.emplace(entry->path(), newName);
        fileRenamed(entry, newName);
    }
}
