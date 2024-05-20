// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <ranges>

#include "torrentfilesmodelentry.h"

#include <QCoreApplication>

namespace tremotesf {
    namespace {
        const TorrentFilesModelEntry* entry(const TorrentFilesModelDirectory::Child& child) {
            return std::visit(
                [](auto& value) -> const TorrentFilesModelEntry* {
                    if constexpr (std::same_as<TorrentFilesModelFile, std::remove_cvref_t<decltype(value)>>) {
                        return &value;
                    } else {
                        return value.get();
                    }
                },
                child
            );
        }

        TorrentFilesModelEntry* entry(TorrentFilesModelDirectory::Child& child) {
            return const_cast<TorrentFilesModelEntry*>(entry(const_cast<const TorrentFilesModelDirectory::Child&>(child)
            ));
        }

        auto entries(const TorrentFilesModelDirectory::Children& children) {
            return std::views::values(children) | std::views::transform([](const auto& child) { return entry(child); });
        }

        auto entries(TorrentFilesModelDirectory::Children& children) {
            return std::views::values(children) | std::views::transform([](auto& child) { return entry(child); });
        }
    }

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
        const QString& name,
        long long size,
        long long completedSize,
        WantedState wantedState,
        Priority priority
    )
        : mParentDirectory(parentDirectory),
          mName(name),
          mSize(size),
          mCompletedSize(completedSize),
          mWantedState(wantedState),
          mPriority(priority),
          mRow(row) {}

    void TorrentFilesModelEntry::setName(const QString& name) {
        if (name != mName) {
            mName = name;
            mChangedMark = true;
        }
    }

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

    void TorrentFilesModelEntry::setCompletedSize(long long completedSize) {
        if (completedSize != mCompletedSize) {
            mCompletedSize = completedSize;
            mChangedMark = true;
        }
    }

    void TorrentFilesModelEntry::setWanted(bool wanted) {
        const auto wantedState = wanted ? WantedState::Wanted : WantedState::Unwanted;
        if (wantedState != mWantedState) {
            mWantedState = wantedState;
            mChangedMark = true;
        }
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

    void TorrentFilesModelEntry::setPriority(Priority priority) {
        if (priority != mPriority) {
            mPriority = priority;
            mChangedMark = true;
        }
    }

    void TorrentFilesModelDirectory::setWanted(bool wanted) {
        TorrentFilesModelEntry::setWanted(wanted);
        for (auto child : entries(mChildren)) {
            child->setWanted(wanted);
        }
    }

    void TorrentFilesModelDirectory::setPriority(Priority priority) {
        TorrentFilesModelEntry::setPriority(priority);
        for (auto child : entries(mChildren)) {
            child->setPriority(priority);
        }
    }

    TorrentFilesModelEntry* TorrentFilesModelDirectory::childAtRow(int row) {
        return entry(const_cast<Child&>(mChildren.sequence().at(static_cast<size_t>(row)).second));
    }

    TorrentFilesModelEntry* TorrentFilesModelDirectory::childForName(const QString& name) {
        const auto found = mChildren.find(name);
        if (found == mChildren.end()) {
            return nullptr;
        }
        return entry(found->second);
    }

    void TorrentFilesModelDirectory::addFile(TorrentFilesModelFile&& file) {
        QString name = file.name();
        mChildren.emplace(std::move(name), std::move(file));
    }

    TorrentFilesModelDirectory* TorrentFilesModelDirectory::addDirectory(const QString& name) {
        const int row = static_cast<int>(mChildren.size());
        auto directory = std::make_unique<TorrentFilesModelDirectory>(row, this, name);
        auto* directoryPtr = directory.get();
        mChildren.emplace(name, std::move(directory));
        return directoryPtr;
    }

    std::vector<int> TorrentFilesModelDirectory::childrenIds() const {
        std::vector<int> ids{};
        ids.reserve(mChildren.size());
        for (auto child : entries(mChildren)) {
            if (child->isDirectory()) {
                const auto childrenIds = static_cast<const TorrentFilesModelDirectory*>(child)->childrenIds();
                ids.reserve(ids.size() + childrenIds.size());
                ids.insert(ids.end(), childrenIds.begin(), childrenIds.end());
            } else {
                ids.push_back(static_cast<const TorrentFilesModelFile*>(child)->id());
            }
        }
        return ids;
    }

    void
    TorrentFilesModelDirectory::initiallyCalculateFromAllChildrenRecursively(std::vector<TorrentFilesModelFile*>& files
    ) {
        if (mChildren.empty()) return;
        mChildren.shrink_to_fit();
        std::optional<WantedState> wantedState{};
        std::optional<Priority> priority{};
        for (auto child : entries(mChildren)) {
            if (child->isDirectory()) {
                static_cast<TorrentFilesModelDirectory*>(child)->initiallyCalculateFromAllChildrenRecursively(files);
            } else {
                files.push_back(static_cast<TorrentFilesModelFile*>(child));
            }
            mSize += child->size();
            mCompletedSize += child->completedSize();
            if (!wantedState.has_value()) {
                wantedState = child->wantedState();
            } else if (wantedState != TorrentFilesModelEntry::WantedState::Mixed &&
                       child->wantedState() != wantedState) {
                wantedState = TorrentFilesModelEntry::WantedState::Mixed;
            }
            if (!priority.has_value()) {
                priority = child->priority();
            } else if (priority != TorrentFilesModelEntry::Priority::Mixed && child->priority() != priority) {
                priority = TorrentFilesModelEntry::Priority::Mixed;
            }
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        mWantedState = wantedState.value();
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        mPriority = priority.value();
    }

    void TorrentFilesModelDirectory::recalculateFromChildren() {
        if (mChildren.empty()) return;
        long long newCompletedSize = 0;
        std::optional<WantedState> newWantedState{};
        std::optional<Priority> newPriority{};
        for (auto child : entries(mChildren)) {
            newCompletedSize += child->completedSize();
            if (!newWantedState.has_value()) {
                newWantedState = child->wantedState();
            } else if (newWantedState != TorrentFilesModelEntry::WantedState::Mixed &&
                       child->wantedState() != newWantedState) {
                newWantedState = TorrentFilesModelEntry::WantedState::Mixed;
            }
            if (!newPriority.has_value()) {
                newPriority = child->priority();
            } else if (newPriority != TorrentFilesModelEntry::Priority::Mixed && child->priority() != newPriority) {
                newPriority = TorrentFilesModelEntry::Priority::Mixed;
            }
        }
        if (newCompletedSize != mCompletedSize || newWantedState != mWantedState || newPriority != mPriority) {
            mCompletedSize = newCompletedSize;
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            mWantedState = newWantedState.value();
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            mPriority = newPriority.value();
            mChangedMark = true;
        }
    }

    void TorrentFilesModelDirectory::forEachChild(std::function<void(TorrentFilesModelEntry*)>&& func) {
        std::ranges::for_each(entries(mChildren), std::move(func));
    }
}
