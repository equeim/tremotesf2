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

namespace tremotesf {
    namespace {
        using namespace std::string_view_literals;

        constexpr auto infoKey = "info"sv;
        constexpr auto filesKey = "files"sv;
        constexpr auto nameKey = "name"sv;
        constexpr auto pathKey = "path"sv;
        constexpr auto lengthKey = "length"sv;

        template<bencode::ValueType Expected>
        std::optional<Expected>
        maybeTakeDictValue(bencode::Dictionary& dict, std::string_view key, std::string_view dictName) {
            if (auto found = dict.find(key); found != dict.end()) {
                auto& [_, value] = *found;
                auto maybeValue = value.maybeTakeValue<Expected>();
                if (!maybeValue) {
                    throw bencode::Error(
                        bencode::Error::Type::Parsing,
                        fmt::format(
                            R"(Value of dictionary "{}" with key "{}" is not of {} type)",
                            dictName,
                            key,
                            bencode::getValueTypeName<Expected>()
                        )
                    );
                }
                return maybeValue;
            }
            return {};
        }

        template<bencode::ValueType Expected>
        Expected takeDictValue(bencode::Dictionary& dict, std::string_view key, std::string_view dictName) {
            if (auto maybeValue = maybeTakeDictValue<Expected>(dict, key, dictName); maybeValue) {
                return std::move(*maybeValue);
            }
            throw bencode::Error(
                bencode::Error::Type::Parsing,
                fmt::format(R"(Dictionary "{}" does not contain value with key "{}")", dictName, key)
            );
        }

        struct CreateTreeResult {
            std::shared_ptr<TorrentFilesModelDirectory> rootDirectory;
            std::vector<TorrentFilesModelFile*> files;
        };

        CreateTreeResult createTree(bencode::Value bencodeParseResult) {
            auto rootMap = bencodeParseResult.maybeTakeDictionary();
            if (!rootMap) {
                throw bencode::Error(bencode::Error::Type::Parsing, "Root element is not a dictionary");
            }

            auto infoMap = takeDictValue<bencode::Dictionary>(*rootMap, infoKey, "<root dictionary>");

            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> files;

            if (auto filesList = maybeTakeDictValue<bencode::List>(infoMap, filesKey, infoKey); filesList) {
                const auto torrentDirectoryName = takeDictValue<QString>(infoMap, nameKey, infoKey);

                auto* torrentDirectory = rootDirectory->addDirectory(torrentDirectoryName);

                const auto filesCount = filesList->size();
                files.reserve(filesCount);
                int fileIndex = -1;
                for (auto& fileValue : *filesList) {
                    ++fileIndex;

                    auto fileMap = fileValue.maybeTakeDictionary();
                    if (!fileMap) {
                        throw bencode::Error(
                            bencode::Error::Type::Parsing,
                            fmt::format("Files list element at index {} is not a dictionary", fileIndex)
                        );
                    }

                    TorrentFilesModelDirectory* currentDirectory = torrentDirectory;

                    auto pathParts = takeDictValue<bencode::List>(*fileMap, pathKey, filesKey);

                    int partIndex = -1;
                    const int lastPartIndex = static_cast<int>(pathParts.size()) - 1;

                    for (auto& partValue : pathParts) {
                        ++partIndex;

                        auto part = partValue.maybeTakeString();
                        if (!part) {
                            throw bencode::Error(
                                bencode::Error::Type::Parsing,
                                fmt::format(
                                    "Path element at index {} for file at index is not a string",
                                    partIndex,
                                    fileIndex
                                )
                            );
                        }

                        if (partIndex == lastPartIndex) {
                            const auto length = takeDictValue<bencode::Integer>(*fileMap, lengthKey, filesKey);

                            auto* childFile = currentDirectory->addFile(fileIndex, *part, length);
                            childFile->setWanted(true);
                            childFile->setPriority(TorrentFilesModelEntry::NormalPriority);
                            childFile->setChanged(false);
                            files.push_back(childFile);
                        } else {
                            const auto& childrenHash = currentDirectory->childrenHash();
                            const auto found = childrenHash.find(*part);
                            if (found != childrenHash.end()) {
                                currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                            } else {
                                currentDirectory = currentDirectory->addDirectory(*part);
                            }
                        }
                    }
                }
            } else {
                const auto name = takeDictValue<QString>(infoMap, nameKey, infoKey);
                const auto length = takeDictValue<bencode::Integer>(infoMap, lengthKey, infoKey);
                auto* file = rootDirectory->addFile(0, name, length);
                file->setWanted(true);
                file->setPriority(TorrentFilesModelEntry::NormalPriority);
                file->setChanged(false);
                files.push_back(file);
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
                return createTree(bencode::parse(filePath));
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
