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

    TorrentFile::Priority TorrentFilesModelEntry::toFilePriority(Priority priority) {
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
        switch (mPriority) {
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

    QIcon TorrentFilesModelEntry::icon() const {
        return std::visit(
            [&](auto&& data) {
                if constexpr (std::same_as<FileData, std::remove_cvref_t<decltype(data)>>) {
                    if (!data.initializedIcon) {
                        data.icon = determineFileIcon(name());
                        data.initializedIcon = true;
                    }
                    return data.icon;
                } else {
                    return desktoputils::standardDirIcon();
                }
            },
            mFileOrDirectoryData
        );
    }

    void TorrentFilesModelEntry::getFileIds(std::vector<int>& ids) const {
        std::visit(
            [&](auto&& data) {
                if constexpr (std::same_as<FileData, std::remove_cvref_t<decltype(data)>>) {
                    ids.push_back(data.id);
                } else {
                    for (const auto& child : data.children) {
                        child.getFileIds(ids);
                    }
                }
            },
            mFileOrDirectoryData
        );
    }

    bool TorrentFilesModelEntry::recalculateFromChildren() {
        const auto& children = std::get<DirectoryData>(mFileOrDirectoryData).children;
        if (children.empty()) return false;
        const auto& firstChild = children.front();
        long long completedSize = firstChild.completedSize();
        WantedState wantedState = firstChild.wantedState();
        Priority priority = firstChild.priority();
        for (const auto& child : children | std::views::drop(1)) {
            completedSize += child.completedSize();
            if (wantedState != TorrentFilesModelEntry::WantedState::Mixed && child.wantedState() != wantedState) {
                wantedState = TorrentFilesModelEntry::WantedState::Mixed;
            }
            if (priority != TorrentFilesModelEntry::Priority::Mixed && child.priority() != priority) {
                priority = TorrentFilesModelEntry::Priority::Mixed;
            }
        }
        return update(wantedState, priority, completedSize);
    }
}
