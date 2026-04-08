// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESTREEBUILDER_H
#define TREMOTESF_TORRENTFILESTREEBUILDER_H

#include <memory>
#include <ranges>
#include <vector>
#include <QHash>

#include "torrentfilesmodelentry.h"

namespace tremotesf {
    namespace impl {
        template<typename T>
        concept QStringOrView = std::same_as<QString, T> || std::same_as<QStringView, T>;

        inline QString toString(QStringView view) { return view.toString(); }
        inline const QString& toString(const QString& string) { return string; }
    }

    class TorrentFilesTreeBuilder final {
    public:
        explicit TorrentFilesTreeBuilder(size_t reserveFilesCount) { files.reserve(reserveFilesCount); }

        ~TorrentFilesTreeBuilder() = default;
        Q_DISABLE_COPY_MOVE(TorrentFilesTreeBuilder)

        void initializeRootDirectory(QString name) {
            rootEntry =
                std::make_unique<TorrentFilesModelEntry>(TorrentFilesModelEntry::createDirectory(0, std::move(name)));
        }

        template<typename PathParts>
            requires(std::ranges::input_range<PathParts> && impl::QStringOrView<std::ranges::range_value_t<PathParts>>)
        void addFile(
            PathParts&& pathParts,
            bool pathIncludesFirstPart,
            long long size,
            long long completedSize,
            bool wanted,
            TorrentFilesModelEntry::Priority priority
        ) {
            auto iter = pathParts.begin();
            if (iter == pathParts.end()) return;

            if (pathIncludesFirstPart) {
                if (!rootEntry) {
                    auto firstPart = *iter;
                    ++iter;
                    if (iter == pathParts.end()) {
                        // This is a file not in any directory. We don't expect any addFile() calls
                        rootEntry = std::make_unique<TorrentFilesModelEntry>(TorrentFilesModelEntry::createFile(
                            0,
                            0,
                            impl::toString(firstPart),
                            size,
                            completedSize,
                            wanted,
                            priority
                        ));
                        files.push_back(rootEntry.get());
                        return;
                    }
                    initializeRootDirectory(impl::toString(firstPart));
                } else {
                    if (!rootEntry->isDirectory()) {
                        // Root entry must be a directory
                        return;
                    }
                    ++iter;
                    if (iter == pathParts.end()) {
                        // This is a file not in any directory, but we already have root entry? This shouldn't happen
                        return;
                    }
                }
            } else if (!rootEntry || !rootEntry->isDirectory()) {
                // If path doesn't include first part then root entry must be initialized using initializeRootDirectory()
                return;
            }

            // This a file in a directory(ies)

            TorrentFilesModelEntry* currentDirectory = rootEntry.get();

            DirectoryCacheEntry* currentDirectoryCacheEntry = &mRootDirectoryCacheEntry;

            while (true) {
                auto part = *iter;
                ++iter;
                if (iter != pathParts.end()) {
                    // This was not the last part, therefore a directory name
                    const auto found = currentDirectoryCacheEntry->childDirectoriesCache.find(part);
                    if (found != currentDirectoryCacheEntry->childDirectoriesCache.constEnd()) {
                        currentDirectory = &currentDirectory->children()[found.value().indexInParent];
                        currentDirectoryCacheEntry = &found.value();
                    } else {
                        const auto partString = impl::toString(part);
                        auto& children = currentDirectory->children();
                        children.push_back(
                            TorrentFilesModelEntry::createDirectory(static_cast<int>(children.size()), partString)
                        );
                        const auto indexInParent = children.size() - 1;
                        currentDirectory = &children.back();
                        const auto inserted = currentDirectoryCacheEntry->childDirectoriesCache.emplace(
                            partString,
                            DirectoryCacheEntry{.indexInParent = indexInParent}
                        );
                        currentDirectoryCacheEntry = &inserted.value();
                    }
                } else {
                    // This was the last part, therefore a file name
                    auto& children = currentDirectory->children();
                    children.push_back(
                        TorrentFilesModelEntry::createFile(
                            mFileId,
                            static_cast<int>(children.size()),
                            impl::toString(part),
                            size,
                            completedSize,
                            wanted,
                            priority
                        )
                    );
                    ++mFileId;
                    break;
                }
            }
        }

        void calculateDirectoriesRecursively() {
            if (!rootEntry) return;
            if (!rootEntry->isDirectory()) return;
            calculateFromChildrenRecursively(rootEntry.get());
            std::ranges::sort(files, std::ranges::less{}, &TorrentFilesModelEntry::fileId);
        }

        std::unique_ptr<TorrentFilesModelEntry> rootEntry{};
        std::vector<TorrentFilesModelEntry*> files{};

    private:
        void calculateFromChildrenRecursively(TorrentFilesModelEntry* directory) {
            auto& children = directory->children();
            if (children.empty()) return;
            long long size{};
            long long completedSize{};
            TorrentFilesModelEntry::WantedState wantedState = children.front().wantedState();
            TorrentFilesModelEntry::Priority priority = children.front().priority();
            for (auto& child : children) {
                child.setParentDirectory(directory);
                if (child.isDirectory()) {
                    calculateFromChildrenRecursively(&child);
                } else {
                    files.push_back(&child);
                }
                size += child.size();
                completedSize += child.completedSize();
                if (wantedState != TorrentFilesModelEntry::WantedState::Mixed && child.wantedState() != wantedState) {
                    wantedState = TorrentFilesModelEntry::WantedState::Mixed;
                }
                if (priority != TorrentFilesModelEntry::Priority::Mixed && child.priority() != priority) {
                    priority = TorrentFilesModelEntry::Priority::Mixed;
                }
            }
            directory->setSize(size);
            directory->update(wantedState, priority, completedSize);
        }

        struct DirectoryCacheEntry {
            size_t indexInParent;
            QHash<QString, DirectoryCacheEntry> childDirectoriesCache{};
        };
        DirectoryCacheEntry mRootDirectoryCacheEntry{};

        int mFileId{};
    };
}

#endif // TREMOTESF_TORRENTFILESTREEBUILDER_H
