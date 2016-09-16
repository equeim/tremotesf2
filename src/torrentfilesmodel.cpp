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

#include "torrentfilesmodel.h"

#include "torrent.h"

#include <algorithm>

#include <QDebug>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "utils.h"

namespace tremotesf
{
    namespace
    {
        void updateFile(TorrentFilesModelFile* file, const QVariantMap& fileMap)
        {
            file->setSize(fileMap.value("length").toLongLong());
            file->setCompletedSize(fileMap.value("bytesCompleted").toLongLong());
            file->setWanted(fileMap.value("wanted").toBool());
            file->setPriority(static_cast<TorrentFilesModelEntryEnums::Priority>(fileMap.value("priority").toInt()));
        }

        QVariantList idsFromIndex(const QModelIndex& index)
        {
            TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
            if (entry->isDirectory()) {
                return static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds();
            }
            return {static_cast<TorrentFilesModelFile*>(entry)->id()};
        }

        QVariantList idsFromIndexes(const QList<QModelIndex>& indexes)
        {
            QVariantList ids;
            for (const QModelIndex& index : indexes) {
                TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                if (entry->isDirectory()) {
                    ids.append(static_cast<TorrentFilesModelDirectory*>(entry)->childrenIds());
                } else {
                    ids.append(static_cast<TorrentFilesModelFile*>(entry)->id());
                }
            }
            std::sort(ids.begin(), ids.end());
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            return ids;
        }
    }

    TorrentFilesModel::TorrentFilesModel(Torrent* torrent, QObject* parent)
        : BaseTorrentFilesModel(parent),
          mTorrent(nullptr),
          mLoaded(false)
    {
        setTorrent(torrent);
    }

    TorrentFilesModel::~TorrentFilesModel()
    {
        if (mTorrent) {
            mTorrent->setFilesEnabled(false);
        }
    }

    int TorrentFilesModel::columnCount(const QModelIndex&) const
    {
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return ColumnCount;
#endif
    }

    QVariant TorrentFilesModel::data(const QModelIndex& index, int role) const
    {
        const TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return entry->name();
        case IsDirectoryRole:
            return entry->isDirectory();
        case CompletedSizeRole:
            return entry->completedSize();
        case SizeRole:
            return entry->size();
        case ProgressRole:
            return entry->progress();
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
            case ProgressColumn:
                return Utils::formatProgress(entry->progress());
            case PriorityColumn:
                return entry->priorityString();
            }
            break;
        case SortRole:
            switch (index.column()) {
            case SizeColumn:
                return entry->size();
            case ProgressBarColumn:
            case ProgressColumn:
                return entry->progress();
            case PriorityColumn:
                return entry->priority();
            default:
                return data(index, Qt::DisplayRole);
            }
        }
#endif
        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    Qt::ItemFlags TorrentFilesModel::flags(const QModelIndex& index) const
    {
        if (index.column() == NameColumn) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        }
        return QAbstractItemModel::flags(index);
    }

    QVariant TorrentFilesModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case NameColumn:
                return qApp->translate("tremotesf", "Name");
            case SizeColumn:
                return qApp->translate("tremotesf", "Size");
            case ProgressBarColumn:
                return qApp->translate("tremotesf", "Progress Bar");
            case ProgressColumn:
                return qApp->translate("tremotesf", "Progress");
            case PriorityColumn:
                return qApp->translate("tremotesf", "Priority");
            }
        }
        return QVariant();
    }

    bool TorrentFilesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (index.column() == NameColumn && role == Qt::CheckStateRole) {
            setFileWanted(index, (value.toInt() == Qt::Checked));
            return true;
        }
        return false;
    }
#endif

    Torrent* TorrentFilesModel::torrent() const
    {
        return mTorrent;
    }

    void TorrentFilesModel::setTorrent(Torrent* torrent)
    {
        if (!torrent || mTorrent) {
            return;
        }

        mTorrent = torrent;

        QObject::connect(mTorrent, &Torrent::filesUpdated, this, [=](const QVariantList& files) {
            if (mRootDirectory->children().isEmpty()) {
                beginResetModel();

                for (int fileIndex = 0, filesCount = files.size(); fileIndex < filesCount; ++fileIndex) {
                    const QVariantMap fileMap(files.at(fileIndex).toMap());

                    TorrentFilesModelDirectory* currentDirectory = mRootDirectory.get();

                    const QString filePath(fileMap.value("name").toString());
                    const QStringList parts(filePath.split('/', QString::SkipEmptyParts));

                    for (int partIndex = 0, partsCount = parts.size(), lastPartIndex = partsCount - 1; partIndex < partsCount; ++partIndex) {
                        const QString& part = parts.at(partIndex);

                        if (partIndex == lastPartIndex) {
                            std::shared_ptr<TorrentFilesModelFile> childFile(std::make_shared<TorrentFilesModelFile>(currentDirectory->children().size(),
                                                                                                                     currentDirectory,
                                                                                                                     part,
                                                                                                                     fileIndex));
                            updateFile(childFile.get(), fileMap);
                            currentDirectory->addChild(childFile);
                            mFiles.insert(filePath, childFile.get());
                        } else {
                            bool found = false;
                            for (const std::shared_ptr<TorrentFilesModelEntry>& child : currentDirectory->children()) {
                                if (child->isDirectory() && child->name() == part) {
                                    currentDirectory = static_cast<TorrentFilesModelDirectory*>(child.get());
                                    found = true;
                                    break;
                                }
                            }

                            if (!found) {
                                const std::shared_ptr<TorrentFilesModelEntry> childDirectory(std::make_shared<TorrentFilesModelDirectory>(currentDirectory->children().size(),
                                                                                                                                          currentDirectory,
                                                                                                                                          part));
                                currentDirectory->addChild(childDirectory);
                                currentDirectory = static_cast<TorrentFilesModelDirectory*>(childDirectory.get());
                            }
                        }
                    }
                }

                endResetModel();
            } else {
                for (const QVariant& file : files) {
                    const QVariantMap fileMap(file.toMap());
                    updateFile(mFiles.value(fileMap.value("name").toString()), fileMap);
                }
                updateDirectoryChildren(mRootDirectory.get());
            }

            if (!mLoaded) {
                mLoaded = true;
                emit loadedChanged();
            }
        });

        QObject::connect(mTorrent, &Torrent::destroyed, this, [this](){
            beginResetModel();
            mRootDirectory->clearChildren();
            endResetModel();
            mFiles.clear();
            mTorrent = nullptr;
        });

        mTorrent->setFilesEnabled(true);
    }

    bool TorrentFilesModel::isLoaded() const
    {
        return mLoaded;
    }

    void TorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesWanted(idsFromIndex(index), wanted);
    }

    void TorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        }
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesWanted(idsFromIndexes(indexes), wanted);
    }

    void TorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntryEnums::Priority priority)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesPriority(idsFromIndex(index), priority);
    }

    void TorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntryEnums::Priority priority)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        }
        updateDirectoryChildren(mRootDirectory.get());
        mTorrent->setFilesPriority(idsFromIndexes(indexes), priority);
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> TorrentFilesModel::roleNames() const
    {
        return {{NameRole, "name"},
                {IsDirectoryRole, "isDirectory"},
                {CompletedSizeRole, "completedSize"},
                {SizeRole, "size"},
                {ProgressRole, "progress"},
                {WantedStateRole, "wantedState"},
                {PriorityRole, "priority"}};
    }
#endif
}
