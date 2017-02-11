/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "torrentfilesmodelentry.h"

#include <QCoreApplication>

namespace tremotesf
{
    TorrentFilesModelEntry::TorrentFilesModelEntry(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name)
        : mRow(row),
          mParentDirectory(parentDirectory),
          mName(name)
    {
    }

    int TorrentFilesModelEntry::row() const
    {
        return mRow;
    }

    TorrentFilesModelDirectory* TorrentFilesModelEntry::parentDirectory() const
    {
        return mParentDirectory;
    }

    QString TorrentFilesModelEntry::name() const
    {
        return mName;
    }

    QString TorrentFilesModelEntry::priorityString() const
    {
        switch (priority()) {
        case TorrentFilesModelEntryEnums::LowPriority:
            return qApp->translate("tremotesf", "Low");
        case TorrentFilesModelEntryEnums::NormalPriority:
            return qApp->translate("tremotesf", "Normal");
        case TorrentFilesModelEntryEnums::HighPriority:
            return qApp->translate("tremotesf", "High");
        case TorrentFilesModelEntryEnums::MixedPriority:
            return qApp->translate("tremotesf", "Mixed");
        }
        return QString();
    }

    TorrentFilesModelDirectory::TorrentFilesModelDirectory(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name)
        : TorrentFilesModelEntry(row, parentDirectory, name)
    {
    }

    TorrentFilesModelDirectory::~TorrentFilesModelDirectory()
    {
        qDeleteAll(mChildren);
    }

    bool TorrentFilesModelDirectory::isDirectory() const
    {
        return true;
    }

    long long TorrentFilesModelDirectory::size() const
    {
        long long bytes = 0;
        for (const TorrentFilesModelEntry* child : mChildren) {
            bytes += child->size();
        }
        return bytes;
    }

    long long TorrentFilesModelDirectory::completedSize() const
    {
        long long bytes = 0;
        for (const TorrentFilesModelEntry* child : mChildren) {
            bytes += child->completedSize();
        }
        return bytes;
    }

    float TorrentFilesModelDirectory::progress() const
    {
        const long long bytes = size();
        if (bytes > 0) {
            return completedSize() / float(bytes);
        }
        return 0;
    }

    TorrentFilesModelEntryEnums::WantedState TorrentFilesModelDirectory::wantedState() const
    {
        const TorrentFilesModelEntryEnums::WantedState first = mChildren.first()->wantedState();
        for (int i = 1, max = mChildren.size(); i < max; ++i) {
            if (mChildren.at(i)->wantedState() != first) {
                return TorrentFilesModelEntryEnums::MixedWanted;
            }
        }
        return first;
    }

    void TorrentFilesModelDirectory::setWanted(bool wanted)
    {
        for (TorrentFilesModelEntry* child : mChildren) {
            child->setWanted(wanted);
        }
    }

    TorrentFilesModelEntryEnums::Priority TorrentFilesModelDirectory::priority() const
    {
        const TorrentFilesModelEntryEnums::Priority first = mChildren.first()->priority();
        for (int i = 1, max = mChildren.size(); i < max; ++i) {
            if (mChildren.at(i)->priority() != first) {
                return TorrentFilesModelEntryEnums::MixedPriority;
            }
        }
        return first;
    }

    void TorrentFilesModelDirectory::setPriority(TorrentFilesModelEntryEnums::Priority priority)
    {
        for (TorrentFilesModelEntry* child : mChildren) {
            child->setPriority(priority);
        }
    }

    const QList<TorrentFilesModelEntry*>& TorrentFilesModelDirectory::children() const
    {
        return mChildren;
    }

    const QHash<QString, TorrentFilesModelEntry*>& TorrentFilesModelDirectory::childrenHash() const
    {
        return mChildrenHash;
    }

    void TorrentFilesModelDirectory::addChild(TorrentFilesModelEntry* child)
    {
        mChildren.append(child);
        mChildrenHash.insert(child->name(), child);
    }

    void TorrentFilesModelDirectory::clearChildren()
    {
        qDeleteAll(mChildren);
        mChildren.clear();
        mChildrenHash.clear();
    }

    QVariantList TorrentFilesModelDirectory::childrenIds() const
    {
        QVariantList ids;
        for (const TorrentFilesModelEntry* child : mChildren) {
            if (child->isDirectory()) {
                ids.append(static_cast<const TorrentFilesModelDirectory*>(child)->childrenIds());
            } else {
                ids.append(static_cast<const TorrentFilesModelFile*>(child)->id());
            }
        }
        return ids;
    }

    bool TorrentFilesModelDirectory::isChanged() const
    {
        for (TorrentFilesModelEntry* child : mChildren) {
            if (child->isChanged()) {
                return true;
            }
        }
        return false;
    }

    TorrentFilesModelFile::TorrentFilesModelFile(int row,
                                                 TorrentFilesModelDirectory* parentDirectory,
                                                 int id,
                                                 const QString& name,
                                                 long long size)
        : TorrentFilesModelEntry(row, parentDirectory, name),
          mSize(size),
          mCompletedSize(0),
          mWantedState(TorrentFilesModelEntryEnums::Unwanted),
          mPriority(TorrentFilesModelEntryEnums::NormalPriority),
          mId(id)
    {
    }

    bool TorrentFilesModelFile::isDirectory() const
    {
        return false;
    }

    long long TorrentFilesModelFile::size() const
    {
        return mSize;
    }

    long long TorrentFilesModelFile::completedSize() const
    {
        return mCompletedSize;
    }

    float TorrentFilesModelFile::progress() const
    {
        if (mSize > 0) {
            return mCompletedSize / float(mSize);
        }
        return 0;
    }

    TorrentFilesModelEntryEnums::WantedState TorrentFilesModelFile::wantedState() const
    {
        return mWantedState;
    }

    void TorrentFilesModelFile::setWanted(bool wanted)
    {
        TorrentFilesModelEntryEnums::WantedState wantedState;
        if (wanted) {
            wantedState = TorrentFilesModelEntryEnums::Wanted;
        } else {
            wantedState = TorrentFilesModelEntryEnums::Unwanted;
        }
        if (wantedState != mWantedState) {
            mWantedState = wantedState;
            mChanged = true;
        }
    }

    TorrentFilesModelEntryEnums::Priority TorrentFilesModelFile::priority() const
    {
        return mPriority;
    }

    void TorrentFilesModelFile::setPriority(TorrentFilesModelEntryEnums::Priority priority)
    {
        if (priority != mPriority) {
            mPriority = priority;
        }
    }

    bool TorrentFilesModelFile::isChanged() const
    {
        return mChanged;
    }

    void TorrentFilesModelFile::setChanged(bool changed)
    {
        mChanged = changed;
    }

    int TorrentFilesModelFile::id() const
    {
        return mId;
    }

    void TorrentFilesModelFile::setSize(long long size)
    {
        mSize = size;
    }

    void TorrentFilesModelFile::setCompletedSize(long long completedSize)
    {
        if (completedSize != mCompletedSize) {
            mCompletedSize = completedSize;
            mChanged = true;
        }
    }
}
