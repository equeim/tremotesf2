// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localtorrentfilesmodel.h"

#include "coroutines/threadpool.h"
#include "ui/itemmodels/torrentfilesmodelentry.h"
#include "torrentfileparser.h"

namespace tremotesf {
    namespace {
        struct CreateTreeResult {
            std::shared_ptr<TorrentFilesModelDirectory> rootDirectory;
            std::vector<TorrentFilesModelFile*> files;
        };

        CreateTreeResult createTree(TorrentMetainfoFile torrentFile) {
            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> files;

            if (torrentFile.isSingleFile()) {
                auto* file = rootDirectory->addFile(0, torrentFile.rootFileName, torrentFile.singleFileSize());
                file->setWanted(true);
                file->setPriority(TorrentFilesModelEntry::Priority::Normal);
                file->setChanged(false);
                files.push_back(file);
            } else {
                const auto torrentDirectoryName = torrentFile.rootFileName;

                auto* torrentDirectory = rootDirectory->addDirectory(torrentDirectoryName);

                auto torrentFiles = torrentFile.files();
                files.reserve(torrentFiles.size());
                int fileIndex = -1;
                for (TorrentMetainfoFile::File file : torrentFiles) {
                    ++fileIndex;

                    TorrentFilesModelDirectory* currentDirectory = torrentDirectory;

                    auto pathParts = file.path();

                    int partIndex = -1;
                    const int lastPartIndex = static_cast<int>(pathParts.size()) - 1;

                    for (const QString& part : pathParts) {
                        ++partIndex;
                        if (partIndex == lastPartIndex) {
                            auto* childFile = currentDirectory->addFile(fileIndex, part, file.size);
                            childFile->setWanted(true);
                            childFile->setPriority(TorrentFilesModelEntry::Priority::Normal);
                            childFile->setChanged(false);
                            files.push_back(childFile);
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
            }

            return {.rootDirectory = std::move(rootDirectory), .files = std::move(files)};
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
