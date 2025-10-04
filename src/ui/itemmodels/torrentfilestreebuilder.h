// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESTREEBUILDER_H
#define TREMOTESF_TORRENTFILESTREEBUILDER_H

#include <ranges>
#include <vector>

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

            const auto lastPartIndex = pathParts.size() - 1;

            for (auto&& [index, part] : pathParts | std::views::enumerate) {
                if (static_cast<decltype(lastPartIndex)>(index) == lastPartIndex) {
                    auto* const file = currentDirectory->addFile(mFileId, part, size, completedSize, wanted, priority);
                    files.push_back(file);
                    ++mFileId;
                } else {
                    const auto& childrenHash = currentDirectory->childrenHash();
                    const auto found = childrenHash.find(part);
                    if (found != childrenHash.end()) {
                        currentDirectory = static_cast<TorrentFilesModelDirectory*>(found->second);
                    } else {
                        currentDirectory = currentDirectory->addDirectory(part);
                    }
                }
            }
        }

        TorrentFilesModelDirectory* rootDirectory;
        std::vector<TorrentFilesModelFile*> files{};

    private:
        int mFileId{};
    };
}

#endif // TREMOTESF_TORRENTFILESTREEBUILDER_H
