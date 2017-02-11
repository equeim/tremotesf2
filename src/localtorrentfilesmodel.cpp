/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include <QThread>

#include <QDebug>

#include "torrentfileparser.h"
#include "torrentfilesmodelentry.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        class TreeCreationWorker : public QObject
        {
            Q_OBJECT
        public:
            explicit TreeCreationWorker(TorrentFilesModelDirectory* rootDirectory,
                                        QList<TorrentFilesModelFile*>& files)
                : mRootDirectory(rootDirectory),
                  mFiles(files)
            {
            }

            void createTree(const QVariantMap& parseResult)
            {
                mRootDirectory->clearChildren();
                mFiles.clear();

                const QVariantMap infoMap(parseResult.value(QStringLiteral("info")).toMap());
                if (infoMap.contains(QStringLiteral("files"))) {
                    auto torrentDirectory = new TorrentFilesModelDirectory(0,
                                                                           mRootDirectory,
                                                                           infoMap.value(QStringLiteral("name")).toString());
                    mRootDirectory->addChild(torrentDirectory);

                    const QVariantList files(infoMap.value(QStringLiteral("files")).toList());
                    for (int fileIndex = 0, filesCount = files.size(); fileIndex < filesCount; ++fileIndex) {
                        const QVariantMap fileMap(files.at(fileIndex).toMap());

                        TorrentFilesModelDirectory* currentDirectory = static_cast<TorrentFilesModelDirectory*>(torrentDirectory);
                        const QStringList pathParts(fileMap.value(QStringLiteral("path")).toStringList());
                        for (int partIndex = 0, partsCount = pathParts.size(), lastPartIndex = partsCount - 1; partIndex < partsCount; ++partIndex) {
                            const QString& part = pathParts.at(partIndex);
                            if (partIndex == lastPartIndex) {
                                auto childFile = new TorrentFilesModelFile(currentDirectory->children().size(),
                                                                           currentDirectory,
                                                                           fileIndex,
                                                                           part,
                                                                           fileMap.value(QStringLiteral("length")).toLongLong());
                                childFile->setWanted(true);
                                childFile->setPriority(TorrentFilesModelEntryEnums::NormalPriority);
                                currentDirectory->addChild(childFile);
                                mFiles.append(childFile);
                            } else {
                                TorrentFilesModelEntry* found = currentDirectory->childrenHash().value(part);
                                if (found) {
                                    currentDirectory = static_cast<TorrentFilesModelDirectory*>(found);
                                } else {
                                    auto childDirectory = new TorrentFilesModelDirectory(currentDirectory->children().size(),
                                                                                         currentDirectory,
                                                                                         part);
                                    currentDirectory->addChild(childDirectory);
                                    currentDirectory = childDirectory;
                                }
                            }
                        }
                    }
                } else {
                    auto file = new TorrentFilesModelFile(0,
                                                          mRootDirectory,
                                                          0,
                                                          infoMap.value(QStringLiteral("name")).toString(),
                                                          infoMap.value(QStringLiteral("length")).toLongLong());
                    file->setWanted(true);
                    file->setPriority(TorrentFilesModelEntryEnums::NormalPriority);
                    mRootDirectory->addChild(file);
                    mFiles.append(file);
                }

                emit done();
            }

        private:
            TorrentFilesModelDirectory* const mRootDirectory;
            QList<TorrentFilesModelFile*>& mFiles;
        signals:
            void done();
        };
    }

    LocalTorrentFilesModel::LocalTorrentFilesModel(const QVariantMap& parseResult, QObject* parent)
        : BaseTorrentFilesModel(parent),
          mWorkerThread(new QThread(this)),
          mLoaded(false)
    {
        if (!parseResult.isEmpty()) {
            load(parseResult);
        }
    }

    LocalTorrentFilesModel::~LocalTorrentFilesModel()
    {
        mWorkerThread->quit();
        mWorkerThread->wait();
    }

    int LocalTorrentFilesModel::columnCount(const QModelIndex&) const
    {
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return ColumnCount;
#endif
    }

    QVariant LocalTorrentFilesModel::data(const QModelIndex& index, int role) const
    {
        const TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return entry->name();
        case IsDirectoryRole:
            return entry->isDirectory();
        case SizeRole:
            return entry->size();
        case WantedStateRole:
            return entry->wantedState();
        case PriorityRole:
            return entry->priority();
        }
#else
        switch (role) {
        case Qt::CheckStateRole:
            if (index.column() == NameColumn) {
                switch (entry->wantedState()) {
                case TorrentFilesModelEntryEnums::Wanted:
                    return Qt::Checked;
                case TorrentFilesModelEntryEnums::Unwanted:
                    return Qt::Unchecked;
                case TorrentFilesModelEntryEnums::MixedWanted:
                    return Qt::PartiallyChecked;
                }
            }
            break;
        case Qt::DecorationRole:
            if (index.column() == NameColumn) {
                if (entry->isDirectory()) {
                    return qApp->style()->standardIcon(QStyle::SP_DirIcon);
                }
                return qApp->style()->standardIcon(QStyle::SP_FileIcon);
            }
            break;
        case Qt::DisplayRole:
            switch (index.column()) {
            case NameColumn:
                return entry->name();
            case SizeColumn:
                return Utils::formatByteSize(entry->size());
            case PriorityColumn:
                return entry->priorityString();
            }
            break;
        case SortRole:
            switch (index.column()) {
            case SizeColumn:
                return entry->size();
            case PriorityColumn:
                return entry->priority();
            }
            return data(index, Qt::DisplayRole);
        }
#endif
        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    Qt::ItemFlags LocalTorrentFilesModel::flags(const QModelIndex& index) const
    {
        if (index.column() == NameColumn) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        }
        return QAbstractItemModel::flags(index);
    }

    QVariant LocalTorrentFilesModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case NameColumn:
                return qApp->translate("tremotesf", "Name");
            case SizeColumn:
                return qApp->translate("tremotesf", "Size");
            case PriorityColumn:
                return qApp->translate("tremotesf", "Priority");
            }
        }
        return QVariant();
    }

    bool LocalTorrentFilesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (index.column() == NameColumn && role == Qt::CheckStateRole) {
            setFileWanted(index, (value.toInt() == Qt::Checked));
            return true;
        }
        return false;
    }
#endif

    void LocalTorrentFilesModel::load(TorrentFileParser* parser)
    {
        load(parser->parseResult());
    }

    bool LocalTorrentFilesModel::isLoaded() const
    {
        return mLoaded;
    }

    QVariantList LocalTorrentFilesModel::wantedFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->wantedState() == TorrentFilesModelEntryEnums::Wanted) {
                files.append(file->id());
            }
        }
        return files;
    }

    QVariantList LocalTorrentFilesModel::unwantedFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->wantedState() == TorrentFilesModelEntryEnums::Unwanted) {
                files.append(file->id());
            }
        }
        return files;
    }

    QVariantList LocalTorrentFilesModel::highPriorityFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntryEnums::HighPriority) {
                files.append(file->id());
            }
        }
        return files;
    }

    QVariantList LocalTorrentFilesModel::normalPriorityFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntryEnums::NormalPriority) {
                files.append(file->id());
            }
        }
        return files;
    }

    QVariantList LocalTorrentFilesModel::lowPriorityFiles() const
    {
        QVariantList files;
        for (const TorrentFilesModelFile* file : mFiles) {
            if (file->priority() == TorrentFilesModelEntryEnums::LowPriority) {
                files.append(file->id());
            }
        }
        return files;
    }

    void LocalTorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        updateDirectoryChildren(mRootDirectory.get());
    }

    void LocalTorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        }
        updateDirectoryChildren(mRootDirectory.get());
    }

    void LocalTorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntryEnums::Priority priority)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        updateDirectoryChildren(mRootDirectory.get());
    }

    void LocalTorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntryEnums::Priority priority)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        }
        updateDirectoryChildren(mRootDirectory.get());
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> LocalTorrentFilesModel::roleNames() const
    {
        return {{NameRole, QByteArrayLiteral("name")},
                {IsDirectoryRole, QByteArrayLiteral("isDirectory")},
                {SizeRole, QByteArrayLiteral("size")},
                {WantedStateRole, QByteArrayLiteral("wantedState")},
                {PriorityRole, QByteArrayLiteral("priority")}};
    }
#endif

    void LocalTorrentFilesModel::load(const QVariantMap& parseResult)
    {
        beginResetModel();

        auto worker = new TreeCreationWorker(mRootDirectory.get(), mFiles);
        worker->moveToThread(mWorkerThread);
        QObject::connect(mWorkerThread, &QThread::finished, worker, &QObject::deleteLater);
        QObject::connect(this, &LocalTorrentFilesModel::requestTreeCreation, worker, &TreeCreationWorker::createTree);
        QObject::connect(worker, &TreeCreationWorker::done, this, [=]() {
            endResetModel();
            mLoaded = true;
            emit loadedChanged();
            mWorkerThread->quit();
        });
        mWorkerThread->start();

        emit requestTreeCreation(parseResult);
    }
}

#include "localtorrentfilesmodel.moc"
