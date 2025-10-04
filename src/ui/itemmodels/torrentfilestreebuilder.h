// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESTREEBUILDER_H
#define TREMOTESF_TORRENTFILESTREEBUILDER_H

#include <ranges>
#include <vector>
#include <QHash>

#include "torrentfilesmodelentry.h"

namespace tremotesf {
    class TorrentFilesTreeBuilder final {
    public:
        explicit TorrentFilesTreeBuilder(TorrentFilesModelDirectory* rootDirectory, size_t reserveFilesCount)
            : rootDirectory(rootDirectory) {
            files.reserve(reserveFilesCount);
        }

        ~TorrentFilesTreeBuilder() = default;
        Q_DISABLE_COPY_MOVE(TorrentFilesTreeBuilder)

        template<typename PathParts>
            requires(
                std::ranges::forward_range<PathParts>
                && std::ranges::sized_range<PathParts>
                && std::same_as<QString, std::ranges::range_value_t<PathParts>>
            )
        void addFile(
            PathParts&& pathParts,
            long long size,
            long long completedSize,
            bool wanted,
            TorrentFilesModelEntry::Priority priority
        ) {
            TorrentFilesModelDirectory* currentDirectory = rootDirectory;
            DirectoryCacheEntry* currentDirectoryCacheEntry = &mRootDirectoryCacheEntry;

            const auto lastPartIndex = pathParts.size() - 1;

            for (auto&& [index, part] : pathParts | std::views::enumerate) {
                if (static_cast<decltype(lastPartIndex)>(index) == lastPartIndex) {
                    auto* const file = currentDirectory->addFile(mFileId, part, size, completedSize, wanted, priority);
                    files.push_back(file);
                    ++mFileId;
                } else {
                    const auto found = currentDirectoryCacheEntry->childDirectoriesCache.find(part);
                    if (found != currentDirectoryCacheEntry->childDirectoriesCache.constEnd()) {
                        currentDirectory = found.value().directory;
                        currentDirectoryCacheEntry = &found.value();
                    } else {
                        currentDirectory = currentDirectory->addDirectory(part);
                        const auto inserted = currentDirectoryCacheEntry->childDirectoriesCache.emplace(
                            part,
                            DirectoryCacheEntry{.directory = currentDirectory}
                        );
                        currentDirectoryCacheEntry = &inserted.value();
                    }
                }
            }
        }

        TorrentFilesModelDirectory* rootDirectory;
        std::vector<TorrentFilesModelFile*> files{};

    private:
        struct DirectoryCacheEntry {
            TorrentFilesModelDirectory* directory;
            QHash<QString, DirectoryCacheEntry> childDirectoriesCache{};
        };
        DirectoryCacheEntry mRootDirectoryCacheEntry{};

        int mFileId{};
    };
}

#endif // TREMOTESF_TORRENTFILESTREEBUILDER_H
