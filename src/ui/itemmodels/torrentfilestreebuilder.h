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
                std::ranges::input_range<PathParts> && std::same_as<QString, std::ranges::range_value_t<PathParts>>
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

            auto iter = pathParts.begin();
            while (true) {
                auto part = *iter;
                ++iter;
                if (iter != pathParts.end()) {
                    // This was not the last part, therefore a directory name
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
                } else {
                    // This was the last part, therefore a file name
                    auto* const file = currentDirectory->addFile(mFileId, part, size, completedSize, wanted, priority);
                    files.push_back(file);
                    ++mFileId;
                    break;
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
