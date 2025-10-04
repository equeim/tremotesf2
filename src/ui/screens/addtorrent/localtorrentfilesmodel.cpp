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
            std::unique_ptr<TorrentFilesModelDirectory> rootDirectory;
            std::vector<TorrentFilesModelFile*> files;
        };

        CreateTreeResult createTree(TorrentMetainfoFile torrentFile) {
            auto rootDirectory = std::make_unique<TorrentFilesModelDirectory>();
            if (torrentFile.isSingleFile() == 1) {
                auto* const file = rootDirectory->addFile(
                    0,
                    torrentFile.rootFileName,
                    torrentFile.singleFileSize(),
                    0,
                    true,
                    TorrentFilesModelEntry::Priority::Normal
                );
                return {.rootDirectory = std::move(rootDirectory), .files = {file}};
            }

            const auto files = torrentFile.files();
            TorrentFilesTreeBuilder builder(rootDirectory->addDirectory(torrentFile.rootFileName), files.size());
            for (TorrentMetainfoFile::File file : files) {
                builder.addFile(file.path(), file.size, 0, true, TorrentFilesModelEntry::Priority::Normal);
            }
            return {.rootDirectory = std::move(rootDirectory), .files = std::move(builder.files)};
        }
    }

    LocalTorrentFilesModel::LocalTorrentFilesModel(QObject* parent)
        : BaseTorrentFilesModel({Column::Name, Column::Size, Column::Priority}, parent) {}

    Coroutine<> LocalTorrentFilesModel::load(TorrentMetainfoFile torrentFile) {
        beginResetModel();
        try {
            auto [rootDirectory, files] = co_await runOnThreadPool(&createTree, std::move(torrentFile));
            mRootDirectory = std::move(rootDirectory);
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
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->wantedState() == TorrentFilesModelEntry::WantedState::Unwanted) {
                files.push_back(file->id());
            }
        }
        return files;
    }

    std::vector<int> LocalTorrentFilesModel::highPriorityFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::Priority::High) {
                files.push_back(file->id());
            }
        }
        return files;
    }

    std::vector<int> LocalTorrentFilesModel::lowPriorityFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::Priority::Low) {
                files.push_back(file->id());
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
