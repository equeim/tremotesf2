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

#include <QFutureWatcher>
#include <QtConcurrentRun>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "torrentfileparser.h"
#include "torrentfilesmodelentry.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        using FutureWatcher = QFutureWatcher<std::pair<std::shared_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>>;

        std::pair<std::shared_ptr<TorrentFilesModelDirectory>, std::vector<TorrentFilesModelFile*>>
        createTree(const QVariantMap& parseResult)
        {
            auto rootDirectory = std::make_shared<TorrentFilesModelDirectory>();
            std::vector<TorrentFilesModelFile*> files;

            const QVariantMap infoMap(parseResult.value(QLatin1String("info")).toMap());
            if (infoMap.contains(QLatin1String("files"))) {
                const QString torrentDirectoryName(infoMap[QLatin1String("name")].toString());
                auto torrentDirectory = new TorrentFilesModelDirectory(0,
                                                                       rootDirectory.get(),
                                                                       torrentDirectoryName,
                                                                       torrentDirectoryName);
                rootDirectory->addChild(torrentDirectory);

                const QVariantList fileMaps(infoMap.value(QLatin1String("files")).toList());
                files.reserve(fileMaps.size());
                for (int fileIndex = 0, filesCount = fileMaps.size(); fileIndex < filesCount; ++fileIndex) {
                    const QVariantMap fileMap(fileMaps.at(fileIndex).toMap());

                    TorrentFilesModelDirectory* currentDirectory = torrentDirectory;
                    const QStringList pathParts(fileMap.value(QLatin1String("path")).toStringList());
                    QString path(torrentDirectoryName);
                    for (int partIndex = 0, partsCount = pathParts.size(), lastPartIndex = partsCount - 1; partIndex < partsCount; ++partIndex) {
                        const QString& part = pathParts.at(partIndex);
                        path += '/';
                        path += part;
                        if (partIndex == lastPartIndex) {
                            auto childFile = new TorrentFilesModelFile(currentDirectory->children().size(),
                                                                       currentDirectory,
                                                                       fileIndex,
                                                                       part,
                                                                       path,
                                                                       fileMap.value(QLatin1String("length")).toLongLong());
                            childFile->setWanted(true);
                            childFile->setPriority(TorrentFilesModelEntry::NormalPriority);
                            childFile->setChanged(false);
                            currentDirectory->addChild(childFile);
                            files.push_back(childFile);
                        } else {
                            const auto& childrenHash = currentDirectory->childrenHash();
                            const auto found = childrenHash.find(part);
                            if (found != childrenHash.end()) {
                                currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                            } else {
                                auto childDirectory = new TorrentFilesModelDirectory(currentDirectory->children().size(),
                                                                                     currentDirectory,
                                                                                     part,
                                                                                     path);
                                currentDirectory->addChild(childDirectory);
                                currentDirectory = childDirectory;
                            }
                        }
                    }
                }
            } else {
                const QString name(infoMap[QLatin1String("name")].toString());
                auto file = new TorrentFilesModelFile(0,
                                                      rootDirectory.get(),
                                                      0,
                                                      name,
                                                      name,
                                                      infoMap.value(QLatin1String("length")).toLongLong());
                file->setWanted(true);
                file->setPriority(TorrentFilesModelEntry::NormalPriority);
                file->setChanged(false);
                rootDirectory->addChild(file);
                files.push_back(file);
            }

            return {std::move(rootDirectory), std::move(files)};
        }
    }

    LocalTorrentFilesModel::LocalTorrentFilesModel(const QVariantMap& parseResult, QObject* parent)
#ifdef TREMOTESF_SAILFISHOS
        : BaseTorrentFilesModel(parent),
#else
        : BaseTorrentFilesModel({NameColumn, SizeColumn, PriorityColumn}, parent),
#endif
          mLoaded(false)
    {
        if (!parseResult.isEmpty()) {
            load(parseResult);
        }
    }

    void LocalTorrentFilesModel::load(TorrentFileParser* parser)
    {
        load(parser->parseResult());
    }

    bool LocalTorrentFilesModel::isLoaded() const
    {
        return mLoaded;
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

    void LocalTorrentFilesModel::load(const QVariantMap& parseResult)
    {
        beginResetModel();

        const auto future = QtConcurrent::run(createTree, parseResult);

        auto watcher = new FutureWatcher(this);
        QObject::connect(watcher, &FutureWatcher::finished, this, [=] {
            auto result = watcher->result();
            mRootDirectory = std::move(result.first);
            mFiles = std::move(result.second);

            endResetModel();
            mLoaded = true;
            emit loadedChanged();

            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }
}
