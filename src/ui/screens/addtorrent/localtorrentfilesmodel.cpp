// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localtorrentfilesmodel.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStyle>

#include "coroutines/threadpool.h"
#include "log/log.h"
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
                file->setPriority(TorrentFilesModelEntry::NormalPriority);
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

                    for (QString part : pathParts) {
                        ++partIndex;
                        if (partIndex == lastPartIndex) {
                            auto* childFile = currentDirectory->addFile(fileIndex, part, file.size);
                            childFile->setWanted(true);
                            childFile->setPriority(TorrentFilesModelEntry::NormalPriority);
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

            return {std::move(rootDirectory), std::move(files)};
        }
    }

    LocalTorrentFilesModel::LocalTorrentFilesModel(QObject* parent)
        : BaseTorrentFilesModel({Column::Name, Column::Size, Column::Priority}, parent) {}

    void LocalTorrentFilesModel::load(const QString& filePath) { mCoroutineScope.launch(loadImpl(filePath)); }

    Coroutine<> LocalTorrentFilesModel::loadImpl(QString filePath) {
        beginResetModel();
        try {
            auto createTreeResult = co_await runOnThreadPool([filePath]() -> CreateTreeResult {
                return createTree(parseTorrentFile(filePath));
            });
            mRootDirectory = std::move(createTreeResult.rootDirectory);
            mFiles = std::move(createTreeResult.files);
        } catch (const bencode::Error& e) {
            warning().logWithException(e, "Failed to parse torrent file {}", filePath);
            mErrorType = e.type();
        }
        endResetModel();
        mLoaded = true;
        emit loadedChanged();
    }

    bool LocalTorrentFilesModel::isLoaded() const { return mLoaded; }

    bool LocalTorrentFilesModel::isSuccessfull() const { return !mErrorType.has_value(); }

    QString LocalTorrentFilesModel::errorString() const {
        if (!mErrorType) {
            return {};
        }
        switch (*mErrorType) {
        case bencode::Error::Type::Reading:
            return qApp->translate("tremotesf", "Error reading torrent file");
        case bencode::Error::Type::Parsing:
            return qApp->translate("tremotesf", "Error parsing torrent file");
        default:
            return {};
        }
    }

    std::vector<int> LocalTorrentFilesModel::unwantedFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->wantedState() == TorrentFilesModelEntry::Unwanted) {
                files.push_back(file->id());
            }
        }
        return files;
    }

    std::vector<int> LocalTorrentFilesModel::highPriorityFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::HighPriority) {
                files.push_back(file->id());
            }
        }
        return files;
    }

    std::vector<int> LocalTorrentFilesModel::lowPriorityFiles() const {
        std::vector<int> files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::LowPriority) {
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
