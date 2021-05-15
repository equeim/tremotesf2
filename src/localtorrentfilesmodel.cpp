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

#include "localtorrentfilesmodel.h"

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "bencodeparser.h"
#include "torrentfilesmodelentry.h"
#include "utils.h"

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

        template<typename T, bool WarnOnNoKey = true>
        std::optional<T> findValue(bencode::Dictionary& dict, std::string_view key, std::string_view dictName, const char* valueType)
        {
            if (auto found = dict.find(key); found != dict.end()) {
                auto maybeValue = found->second.takeValue<T>();
                if (!maybeValue) {
                    qWarning("createTree: %s dictionary value for key %s is not of %s type", dictName.data(), key.data(), valueType);
                }
                return maybeValue;
            }
            if constexpr (WarnOnNoKey) {
                qWarning("createTree: %s dictionary doesn't have key %s", dictName.data(), key.data());
            }
            return std::nullopt;
        }

        struct CreateTreeResult
        {
            std::shared_ptr<TorrentFilesModelDirectory> rootDirectory;
            std::vector<TorrentFilesModelFile*> files;
        };

        CreateTreeResult createTree(bencode::Value&& parseResult)
        {
            auto rootMap = parseResult.takeDictionary();
            if (!rootMap) {
                qWarning("createTree: root element is not a dictionary");
                return {};
            }

            auto infoMap = findValue<bencode::Dictionary>(*rootMap, infoKey, "root", "dictionary");
            if (!infoMap) {
                return {};
            }

            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> files;

            if (auto filesList = findValue<bencode::List, false>(*infoMap, filesKey, infoKey, "list"); filesList) {
                const auto torrentDirectoryName = findValue<QString>(*infoMap, nameKey, infoKey, "string");
                if (!torrentDirectoryName) {
                    return {};
                }

                auto torrentDirectory = new TorrentFilesModelDirectory(0,
                                                                       rootDirectory.get(),
                                                                       *torrentDirectoryName,
                                                                       *torrentDirectoryName);
                rootDirectory->addChild(torrentDirectory);

                const auto filesCount = filesList->size();
                files.reserve(filesCount);
                int fileIndex = -1;
                for (auto& fileValue : *filesList) {
                    ++fileIndex;

                    auto fileMap = fileValue.takeDictionary();
                    if (!fileMap) {
                        qWarning("createTree: files list element at index %d is not a dictionary", fileIndex);
                        return {};
                    }

                    TorrentFilesModelDirectory* currentDirectory = torrentDirectory;

                    auto pathParts = findValue<bencode::List>(*fileMap, pathKey, filesKey, "list");
                    if (!pathParts) {
                        return {};
                    }

                    auto path = *torrentDirectoryName;
                    int partIndex = -1;
                    const int lastPartIndex = static_cast<int>(pathParts->size()) - 1;

                    for (auto& partValue : *pathParts) {
                        ++partIndex;

                        auto part = partValue.takeString();
                        if (!part) {
                            qWarning("createTree: path element '%d' for file at index '%d' is not a string", partIndex, fileIndex);
                            return {};
                        }

                        path += QLatin1Char('/');
                        path += *part;
                        if (partIndex == lastPartIndex) {
                            auto length = findValue<bencode::Integer>(*fileMap, lengthKey, filesKey, "integer");
                            if (!length) {
                                return {};
                            }

                            auto childFile = new TorrentFilesModelFile(static_cast<int>(currentDirectory->children().size()),
                                                                       currentDirectory,
                                                                       fileIndex,
                                                                       *part,
                                                                       path,
                                                                       *length);
                            childFile->setWanted(true);
                            childFile->setPriority(TorrentFilesModelEntry::NormalPriority);
                            childFile->setChanged(false);
                            currentDirectory->addChild(childFile);
                            files.push_back(childFile);
                        } else {
                            const auto& childrenHash = currentDirectory->childrenHash();
                            const auto found = childrenHash.find(*part);
                            if (found != childrenHash.end()) {
                                currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                            } else {
                                auto childDirectory = new TorrentFilesModelDirectory(static_cast<int>(currentDirectory->children().size()),
                                                                                     currentDirectory,
                                                                                     *part,
                                                                                     path);
                                currentDirectory->addChild(childDirectory);
                                currentDirectory = childDirectory;
                            }
                        }
                    }
                }
            } else {
                auto name = findValue<QString>(*infoMap, nameKey, infoKey, "string");
                if (!name) {
                    return {};
                }
                auto length = findValue<bencode::Integer>(*infoMap, lengthKey, infoKey, "integer");
                if (!length) {
                    return {};
                }

                auto file = new TorrentFilesModelFile(0,
                                                      rootDirectory.get(),
                                                      0,
                                                      *name,
                                                      *name,
                                                      *length);
                file->setWanted(true);
                file->setPriority(TorrentFilesModelEntry::NormalPriority);
                file->setChanged(false);
                rootDirectory->addChild(file);
                files.push_back(file);
            }

            return {std::move(rootDirectory), std::move(files)};
        }
    }

    LocalTorrentFilesModel::LocalTorrentFilesModel(QObject* parent)
#ifdef TREMOTESF_SAILFISHOS
        : BaseTorrentFilesModel(parent),
#else
        : BaseTorrentFilesModel({NameColumn, SizeColumn, PriorityColumn}, parent),
#endif
          mLoaded(false), mError(bencode::NoError)
    {

    }

    void LocalTorrentFilesModel::load(const QString& filePath)
    {
        beginResetModel();

        using FutureWatcher = QFutureWatcher<std::pair<bencode::Error, CreateTreeResult>>;

        const auto future = QtConcurrent::run([=]() -> std::pair<bencode::Error, CreateTreeResult> {
            auto parseResult = bencode::parse(filePath);
            if (parseResult.error == bencode::NoError) {
                if (auto createTreeResult = createTree(std::move(parseResult.parseResult)); createTreeResult.rootDirectory) {
                    return {bencode::NoError, std::move(createTreeResult)};
                }
                return {bencode::ParsingError, {}};
            }
            return {parseResult.error, {}};
        });

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=] {
            auto [error, createTreeResult] = watcher->result();

            mRootDirectory = std::move(createTreeResult.rootDirectory);
            endResetModel();

            mFiles = std::move(createTreeResult.files);

            mLoaded = true;
            mError = error;
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
        return mError == bencode::NoError;
    }

    QString LocalTorrentFilesModel::errorString() const
    {
        switch (mError) {
        case bencode::ReadingError:
            return qApp->translate("tremotesf", "Error reading torrent file");
        case bencode::ParsingError:
            return qApp->translate("tremotesf", "Error parsing torrent file");
        default:
            return QString();
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
        fileRenamed(entry, newName);
        mRenamedFiles.insert(entry->path(), newName);
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> LocalTorrentFilesModel::roleNames() const
    {
        return {{NameRole, "name"},
                {IsDirectoryRole, "isDirectory"},
                {SizeRole, "size"},
                {WantedStateRole, "wantedState"},
                {PriorityRole, "priority"}};
    }
#endif
}
