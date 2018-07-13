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

#include "torrentfilesmodelentry.h"

#include <QCoreApplication>

namespace tremotesf
{
    TorrentFilesModelEntry::Priority TorrentFilesModelEntry::fromFilePriority(libtremotesf::TorrentFile::Priority priority)
    {
        using libtremotesf::TorrentFile;
        switch (priority) {
        case TorrentFile::LowPriority:
            return LowPriority;
        case TorrentFile::NormalPriority:
            return NormalPriority;
        case TorrentFile::HighPriority:
            return HighPriority;
        default:
            return NormalPriority;
        }
    }

    libtremotesf::TorrentFile::Priority TorrentFilesModelEntry::toFilePriority(TorrentFilesModelEntry::Priority priority)
    {
        using libtremotesf::TorrentFile;
        switch (priority) {
        case LowPriority:
            return TorrentFile::LowPriority;
        case NormalPriority:
            return TorrentFile::NormalPriority;
        case HighPriority:
            return TorrentFile::HighPriority;
        default:
            return TorrentFile::NormalPriority;
        }
    }

    TorrentFilesModelEntry::TorrentFilesModelEntry(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name, const QString& path)
        : mRow(row),
          mParentDirectory(parentDirectory),
          mName(name),
          mPath(path)
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

    void TorrentFilesModelEntry::setName(const QString& name)
    {
        mName = name;
    }

    QString TorrentFilesModelEntry::path() const
    {
        return mPath;
    }

    void TorrentFilesModelEntry::setPath(const QString& path)
    {
        mPath = path;
    }

    QString TorrentFilesModelEntry::priorityString() const
    {
        switch (priority()) {
        case LowPriority:
            //: Priority
            return qApp->translate("tremotesf", "Low");
        case NormalPriority:
            //: Priority
            return qApp->translate("tremotesf", "Normal");
        case HighPriority:
            //: Priority
            return qApp->translate("tremotesf", "High");
        case MixedPriority:
            //: Priority
            return qApp->translate("tremotesf", "Mixed");
        }
        return QString();
    }

    TorrentFilesModelDirectory::TorrentFilesModelDirectory(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name, const QString& path)
        : TorrentFilesModelEntry(row, parentDirectory, name, path)
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

    double TorrentFilesModelDirectory::progress() const
    {
        const long long bytes = size();
        if (bytes > 0) {
            return completedSize() / static_cast<double>(bytes);
        }
        return 0;
    }

    TorrentFilesModelEntry::WantedState TorrentFilesModelDirectory::wantedState() const
    {
        const TorrentFilesModelEntry::WantedState first = mChildren.front()->wantedState();
        for (int i = 1, max = mChildren.size(); i < max; ++i) {
            if (mChildren[i]->wantedState() != first) {
                return MixedWanted;
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

    TorrentFilesModelEntry::Priority TorrentFilesModelDirectory::priority() const
    {
        const Priority first = mChildren.front()->priority();
        for (int i = 1, max = mChildren.size(); i < max; ++i) {
            if (mChildren[i]->priority() != first) {
                return MixedPriority;
            }
        }
        return first;
    }

    void TorrentFilesModelDirectory::setPriority(Priority priority)
    {
        for (TorrentFilesModelEntry* child : mChildren) {
            child->setPriority(priority);
        }
    }

    const std::vector<TorrentFilesModelEntry*>& TorrentFilesModelDirectory::children() const
    {
        return mChildren;
    }

    const std::unordered_map<QString, TorrentFilesModelEntry*>& TorrentFilesModelDirectory::childrenHash() const
    {
        return mChildrenHash;
    }

    void TorrentFilesModelDirectory::addChild(TorrentFilesModelEntry* child)
    {

        mChildren.push_back(child);
        mChildrenHash.insert({child->name(), child});
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
                                                 const QString& path,
                                                 long long size)
        : TorrentFilesModelEntry(row, parentDirectory, name, path),
          mSize(size),
          mCompletedSize(0),
          mWantedState(Unwanted),
          mPriority(NormalPriority),
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

    double TorrentFilesModelFile::progress() const
    {
        if (mSize > 0) {
            return mCompletedSize / static_cast<double>(mSize);
        }
        return 0;
    }

    TorrentFilesModelEntry::WantedState TorrentFilesModelFile::wantedState() const
    {
        return mWantedState;
    }

    void TorrentFilesModelFile::setWanted(bool wanted)
    {
        WantedState wantedState;
        if (wanted) {
            wantedState = Wanted;
        } else {
            wantedState = Unwanted;
        }
        if (wantedState != mWantedState) {
            mWantedState = wantedState;
            mChanged = true;
        }
    }

    TorrentFilesModelEntry::Priority TorrentFilesModelFile::priority() const
    {
        return mPriority;
    }

    void TorrentFilesModelFile::setPriority(Priority priority)
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
