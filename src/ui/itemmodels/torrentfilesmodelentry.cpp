// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QMimeDatabase>

#include "desktoputils.h"
#include "stdutils.h"
#include "torrentfilesmodelentry.h"

namespace tremotesf {
    TorrentFilesModelEntry::Priority TorrentFilesModelEntry::fromFilePriority(TorrentFile::Priority priority) {
        switch (priority) {
        case TorrentFile::Priority::Low:
            return Priority::Low;
        case TorrentFile::Priority::Normal:
            return Priority::Normal;
        case TorrentFile::Priority::High:
            return Priority::High;
        default:
            return Priority::Normal;
        }
    }

    TorrentFile::Priority TorrentFilesModelEntry::toFilePriority(TorrentFilesModelEntry::Priority priority) {
        switch (priority) {
        case Priority::Low:
            return TorrentFile::Priority::Low;
        case Priority::Normal:
            return TorrentFile::Priority::Normal;
        case Priority::High:
            return TorrentFile::Priority::High;
        default:
            return TorrentFile::Priority::Normal;
        }
    }

    TorrentFilesModelEntry::TorrentFilesModelEntry(
        int row,
        TorrentFilesModelDirectory* parentDirectory,
        QString name,
        long long size,
        long long completedSize,
        bool wanted,
        Priority priority
    )
        : mRow(row),
          mParentDirectory(parentDirectory),
          mName(std::move(name)),
          mSize(size),
          mCompletedSize(completedSize),
          mWantedState(wanted ? WantedState::Wanted : WantedState::Unwanted),
          mPriority(priority) {}

    QString TorrentFilesModelEntry::path() const {
        QString path(mName);
        const auto* parent = mParentDirectory;
        while (parent && !parent->name().isEmpty()) {
            path.prepend('/');
            path.prepend(parent->name());
            parent = parent->parentDirectory();
        }
        return path;
    }

    bool TorrentFilesModelEntry::setWanted(bool wanted) {
        WantedState wantedState{};
        if (wanted) {
            wantedState = WantedState::Wanted;
        } else {
            wantedState = WantedState::Unwanted;
        }
        if (wantedState != mWantedState) {
            mWantedState = wantedState;
            return true;
        }
        return false;
    }

    QString TorrentFilesModelEntry::priorityString() const {
        switch (priority()) {
        case Priority::Low:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "Low");
        case Priority::Normal:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "Normal");
        case Priority::High:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "High");
        case Priority::Mixed:
            //: Torrent's file loading priority
            return qApp->translate("tremotesf", "Mixed");
        }
        return {};
    }

    bool TorrentFilesModelEntry::setPriority(Priority priority) {
        if (priority != mPriority) {
            mPriority = priority;
            return true;
        }
        return false;
    }

    bool TorrentFilesModelEntry::update(bool wanted, Priority priority, long long completedSize) {
        return update(wanted ? WantedState::Wanted : WantedState::Unwanted, priority, completedSize);
    }

    bool TorrentFilesModelEntry::update(WantedState wantedState, Priority priority, long long completedSize) {
        bool changed = false;
        setChanged(mWantedState, wantedState, changed);
        setChanged(mPriority, priority, changed);
        setChanged(mCompletedSize, completedSize, changed);
        return changed;
    }

    TorrentFilesModelDirectory::TorrentFilesModelDirectory(
        int row, TorrentFilesModelDirectory* parentDirectory, QString name
    )
        : TorrentFilesModelEntry(row, parentDirectory, std::move(name), 0, 0, true, Priority::Normal) {}

    TorrentFilesModelFile* TorrentFilesModelDirectory::addFile(
        int id, const QString& name, long long size, long long completedSize, bool wanted, Priority priority
    ) {
        const int row = static_cast<int>(mChildren.size());
        auto file = std::make_unique<TorrentFilesModelFile>(row, this, id, name, size, completedSize, wanted, priority);
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

    void TorrentFilesModelDirectory::clearChildren() { mChildren.clear(); }

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

    QIcon tremotesf::TorrentFilesModelDirectory::icon() const { return desktoputils::standardDirIcon(); }

    bool TorrentFilesModelDirectory::recalculateFromChildren() {
        if (mChildren.empty()) return false;
        long long completedSize{};
        WantedState wantedState = mChildren.front()->wantedState();
        Priority priority = mChildren.front()->priority();
        for (const auto& child : mChildren) {
            completedSize += child->completedSize();
            if (wantedState != TorrentFilesModelEntry::WantedState::Mixed && child->wantedState() != wantedState) {
                wantedState = TorrentFilesModelEntry::WantedState::Mixed;
            }
            if (priority != TorrentFilesModelEntry::Priority::Mixed && child->priority() != priority) {
                priority = TorrentFilesModelEntry::Priority::Mixed;
            }
        }
        return update(wantedState, priority, completedSize);
    }

    void TorrentFilesModelDirectory::addChild(std::unique_ptr<TorrentFilesModelEntry>&& child) {
        mChildren.push_back(std::move(child));
    }

    TorrentFilesModelFile::TorrentFilesModelFile(
        int row,
        TorrentFilesModelDirectory* parentDirectory,
        int id,
        QString name,
        long long size,
        long long completedSize,
        bool wanted,
        Priority priority
    )
        : TorrentFilesModelEntry(row, parentDirectory, std::move(name), size, completedSize, wanted, priority),
          mId(id) {}

    namespace {
        QIcon determineFileIcon(const QString& fileName) {
            static const QMimeDatabase mimeDb{};
            const auto mimeType = mimeDb.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);
            if (!mimeType.isValid()) {
                return desktoputils::standardFileIcon();
            }
            if (const auto icon = QIcon::fromTheme(mimeType.iconName()); !icon.isNull()) {
                return icon;
            }
            if (const auto icon = QIcon::fromTheme(mimeType.genericIconName()); !icon.isNull()) {
                return icon;
            }
            return desktoputils::standardFileIcon();
        }
    }

    QIcon TorrentFilesModelFile::icon() const {
        if (!mInitializedIcon) {
            mIcon = determineFileIcon(name());
            mInitializedIcon = true;
        }
        return mIcon;
    }
}
