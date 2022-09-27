// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localtorrentfilesmodel.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QStyle>
#include <QtConcurrentRun>

#include "libtremotesf/log.h"
#include "tremotesf/ui/itemmodels/torrentfilesmodelentry.h"

namespace tremotesf
{
    namespace
    {
        using namespace std::string_view_literals;

        constexpr auto infoKey = "info"sv;
        constexpr auto filesKey = "files"sv;
        constexpr auto nameKey = "name"sv;
        constexpr auto pathKey = "path"sv;
        constexpr auto lengthKey = "length"sv;

        template<typename ValueType>
        std::optional<ValueType> maybeTakeDictValue(bencode::Dictionary& dict, std::string_view key, std::string_view dictName)
        {
            if (auto found = dict.find(key); found != dict.end()) {
                auto& [_, value] = *found;
                auto maybeValue = value.maybeTakeValue<ValueType>();
                if (!maybeValue) {
                    throw bencode::Error(bencode::Error::Type::Parsing, std::string("Value of dictionary \"") + dictName.data() + "\" with key \"" + key.data() + "\" is not of " + bencode::getValueTypeName<ValueType>() + " type");
                }
                return maybeValue;
            }
            return {};
        }

        template<typename ValueType>
        ValueType takeDictValue(bencode::Dictionary& dict, std::string_view key, std::string_view dictName)
        {
            if (auto maybeValue = maybeTakeDictValue<ValueType>(dict, key, dictName); maybeValue) {
                return std::move(*maybeValue);
            }
            throw bencode::Error(bencode::Error::Type::Parsing, std::string("Dictionary \"") + dictName.data() + "\" does not contain value with key \"" + key.data() + "\"");
        }

        struct CreateTreeResult
        {
            std::shared_ptr<TorrentFilesModelDirectory> rootDirectory;
            std::vector<TorrentFilesModelFile*> files;
        };

        CreateTreeResult createTree(bencode::Value&& bencodeParseResult)
        {
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
                        throw bencode::Error(bencode::Error::Type::Parsing, "Files list element at index " + std::to_string(fileIndex) + " is not a dictionary");
                    }

                    TorrentFilesModelDirectory* currentDirectory = torrentDirectory;

                    auto pathParts = takeDictValue<bencode::List>(*fileMap, pathKey, filesKey);

                    int partIndex = -1;
                    const int lastPartIndex = static_cast<int>(pathParts.size()) - 1;

                    for (auto& partValue : pathParts) {
                        ++partIndex;

                        auto part = partValue.maybeTakeString();
                        if (!part) {
                            throw bencode::Error(bencode::Error::Type::Parsing, "Path element at index " + std::to_string(partIndex) + " for file at index " + std::to_string(fileIndex) + " is not a string");
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
        : BaseTorrentFilesModel({Column::Name, Column::Size, Column::Priority}, parent),
          mLoaded(false)
    {

    }

    void LocalTorrentFilesModel::load(const QString& filePath)
    {
        beginResetModel();

        using FutureResult = std::variant<CreateTreeResult, bencode::Error::Type>;
        using FutureWatcher = QFutureWatcher<FutureResult>;

        const auto future = QtConcurrent::run([=]() -> FutureResult {
            try {
                auto bencodeParseResult = bencode::parse(filePath);
                return createTree(std::move(bencodeParseResult));
            } catch (const bencode::Error& e) {
                logWarningWithException(e, "Failed to parse torrent file {}", filePath);
                return e.type();
            }
        });

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=] {
            auto result = watcher->result();
            if (auto createTreeResult = std::get_if<CreateTreeResult>(&result); createTreeResult) {
                mRootDirectory = std::move(createTreeResult->rootDirectory);
                mFiles = std::move(createTreeResult->files);
            } else {
                mErrorType = std::get<bencode::Error::Type>(result);
            }
            endResetModel();
            mLoaded = true;
            emit loadedChanged();
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    bool LocalTorrentFilesModel::isLoaded() const
    {
        return mLoaded;
    }

    bool LocalTorrentFilesModel::isSuccessfull() const
    {
        return !mErrorType.has_value();
    }

    QString LocalTorrentFilesModel::errorString() const
    {
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

    QVariantList LocalTorrentFilesModel::unwantedFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->wantedState() == TorrentFilesModelEntry::Unwanted) {
                files.append(file->id());
            }
        }
        return files;
    }

    QVariantList LocalTorrentFilesModel::highPriorityFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::HighPriority) {
                files.append(file->id());
            }
        }
        return files;
    }

    QVariantList LocalTorrentFilesModel::lowPriorityFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntry::LowPriority) {
                files.append(file->id());
            }
        }
        return files;
    }

    const QVariantMap& LocalTorrentFilesModel::renamedFiles() const
    {
        return mRenamedFiles;
    }

    void LocalTorrentFilesModel::renameFile(const QModelIndex& index, const QString& newName)
    {
        auto entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
        mRenamedFiles.insert(entry->path(), newName);
        fileRenamed(entry, newName);
    }
}
