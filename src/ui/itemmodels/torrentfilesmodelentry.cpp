// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfilesmodelentry.h"

#include <QCoreApplication>

#include "literals.h"

namespace tremotesf {
    TorrentFilesModelEntry::Priority TorrentFilesModelEntry::fromFilePriority(TorrentFile::Priority priority) {
        switch (priority) {
        case TorrentFile::Priority::Low:
            return LowPriority;
        case TorrentFile::Priority::Normal:
            return NormalPriority;
        case TorrentFile::Priority::High:
            return HighPriority;
        default:
            return NormalPriority;
        }
    }

    TorrentFile::Priority TorrentFilesModelEntry::toFilePriority(TorrentFilesModelEntry::Priority priority) {
        switch (priority) {
        case LowPriority:
            return TorrentFile::Priority::Low;
        case NormalPriority:
            return TorrentFile::Priority::Normal;
        case HighPriority:
            return TorrentFile::Priority::High;
        default:
            return TorrentFile::Priority::Normal;
        }
    }

    TorrentFilesModelEntry::TorrentFilesModelEntry(int row, TorrentFilesModelDirectory* parentDirectory, QString name)
        : mRow(row), mParentDirectory(parentDirectory), mName(std::move(name)) {}

    int TorrentFilesModelEntry::row() const { return mRow; }

    TorrentFilesModelDirectory* TorrentFilesModelEntry::parentDirectory() const { return mParentDirectory; }

    QString TorrentFilesModelEntry::name() const { return mName; }

    void TorrentFilesModelEntry::setName(const QString& name) { mName = name; }

    QString TorrentFilesModelEntry::path() const {
        QString path(mName);
        TorrentFilesModelDirectory* parent = mParentDirectory;
        while (parent && !parent->name().isEmpty()) {
            path.prepend('/');
            path.prepend(parent->name());
            parent = parent->parentDirectory();
        }
        return path;
    }

    QString TorrentFilesModelEntry::priorityString() const {
        switch (priority()) {
        case LowPriority:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "Low");
        case NormalPriority:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "Normal");
        case HighPriority:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "High");
        case MixedPriority:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "Mixed");
        }
        return {};
    }

    TorrentFilesModelDirectory::TorrentFilesModelDirectory(
        int row, TorrentFilesModelDirectory* parentDirectory, const QString& name
    )
        : TorrentFilesModelEntry(row, parentDirectory, name) {}

    bool TorrentFilesModelDirectory::isDirectory() const { return true; }

    long long TorrentFilesModelDirectory::size() const {
        long long bytes = 0;
        for (const auto& child : mChildren) {
            bytes += child->size();
        }
        return bytes;
    }

    long long TorrentFilesModelDirectory::completedSize() const {
        long long bytes = 0;
        for (const auto& child : mChildren) {
            bytes += child->completedSize();
        }
        return bytes;
    }

    double TorrentFilesModelDirectory::progress() const {
        const long long bytes = size();
        if (bytes > 0) {
            return static_cast<double>(completedSize()) / static_cast<double>(bytes);
        }
        return 0;
    }

    TorrentFilesModelEntry::WantedState TorrentFilesModelDirectory::wantedState() const {
        const TorrentFilesModelEntry::WantedState first = mChildren.front()->wantedState();
        for (auto i = mChildren.begin() + 1, end = mChildren.end(); i != end; ++i) {
            if ((*i)->wantedState() != first) {
                return MixedWanted;
            }
        }
        return first;
    }

    void TorrentFilesModelDirectory::setWanted(bool wanted) {
        for (auto& child : mChildren) {
            child->setWanted(wanted);
        }
    }

    TorrentFilesModelEntry::Priority TorrentFilesModelDirectory::priority() const {
        const Priority first = mChildren.front()->priority();
        for (auto i = mChildren.begin() + 1, end = mChildren.end(); i != end; ++i) {
            if ((*i)->priority() != first) {
                return MixedPriority;
            }
        }
        return first;
    }

    void TorrentFilesModelDirectory::setPriority(Priority priority) {
        for (const auto& child : mChildren) {
            child->setPriority(priority);
        }
    }

    const std::vector<std::unique_ptr<TorrentFilesModelEntry>>& TorrentFilesModelDirectory::children() const {
        return mChildren;
    }

    const std::unordered_map<QString, TorrentFilesModelEntry*>& TorrentFilesModelDirectory::childrenHash() const {
        return mChildrenHash;
    }

    TorrentFilesModelFile* TorrentFilesModelDirectory::addFile(int id, const QString& name, long long size) {
        const int row = static_cast<int>(mChildren.size());
        auto file = std::make_unique<TorrentFilesModelFile>(row, this, id, name, size);
        auto* filePtr = file.get();
        addChild(std::move(file));
        return filePtr;
    }

    TorrentFilesModelDirectory* TorrentFilesModelDirectory::addDirectory(const QString& name) {
        const int row = static_cast<int>(mChildren.size());
        auto directory = std::make_unique<TorrentFilesModelDirectory>(row, this, name);
        auto* directoryPtr = directory.get();
        addChild(std::move(directory));
        return directoryPtr;
    }

    void TorrentFilesModelDirectory::clearChildren() {
        mChildren.clear();
        mChildrenHash.clear();
    }

    std::vector<int> TorrentFilesModelDirectory::childrenIds() const {
        std::vector<int> ids{};
        ids.reserve(mChildren.size());
        for (const auto& child : mChildren) {
            if (child->isDirectory()) {
                const auto childrenIds = static_cast<TorrentFilesModelDirectory*>(child.get())->childrenIds();
                ids.reserve(ids.size() + childrenIds.size());
                ids.insert(ids.end(), childrenIds.begin(), childrenIds.end());
            } else {
                ids.push_back(static_cast<const TorrentFilesModelFile*>(child.get())->id());
            }
        }
        return ids;
    }

    bool TorrentFilesModelDirectory::isChanged() const {
        return std::any_of(mChildren.begin(), mChildren.end(), [](const auto& child) { return child->isChanged(); });
    }

    void TorrentFilesModelDirectory::addChild(std::unique_ptr<TorrentFilesModelEntry>&& child) {
        mChildrenHash.emplace(child->name(), child.get());
        mChildren.push_back(std::move(child));
    }

    TorrentFilesModelFile::TorrentFilesModelFile(
        int row, TorrentFilesModelDirectory* parentDirectory, int id, const QString& name, long long size
    )
        : TorrentFilesModelEntry(row, parentDirectory, name),
          mSize(size),
          mCompletedSize(0),
          mWantedState(Unwanted),
          mPriority(NormalPriority),
          mId(id),
          mChanged(false) {}

    bool TorrentFilesModelFile::isDirectory() const { return false; }

    long long TorrentFilesModelFile::size() const { return mSize; }

    long long TorrentFilesModelFile::completedSize() const { return mCompletedSize; }

    double TorrentFilesModelFile::progress() const {
        if (mSize > 0) {
            return static_cast<double>(mCompletedSize) / static_cast<double>(mSize);
        }
        return 0;
    }

    TorrentFilesModelEntry::WantedState TorrentFilesModelFile::wantedState() const { return mWantedState; }

    void TorrentFilesModelFile::setWanted(bool wanted) {
        WantedState wantedState{};
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

    TorrentFilesModelEntry::Priority TorrentFilesModelFile::priority() const { return mPriority; }

    void TorrentFilesModelFile::setPriority(Priority priority) {
        if (priority != mPriority) {
            mPriority = priority;
        }
    }

    bool TorrentFilesModelFile::isChanged() const { return mChanged; }

    void TorrentFilesModelFile::setChanged(bool changed) { mChanged = changed; }

    int TorrentFilesModelFile::id() const { return mId; }

    void TorrentFilesModelFile::setSize(long long size) { mSize = size; }

    void TorrentFilesModelFile::setCompletedSize(long long completedSize) {
        if (completedSize != mCompletedSize) {
            mCompletedSize = completedSize;
            mChanged = true;
        }
    }
}
