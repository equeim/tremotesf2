// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILEPARSER_H
#define TREMOTESF_TORRENTFILEPARSER_H

#include <ranges>
#include <set>
#include <vector>
#include <variant>

#include <QString>

#include "log/formatters.h"
#include "bencodeparser.h"

namespace tremotesf {
    class TorrentMetainfoFile final {
    public:
        explicit TorrentMetainfoFile(bencode::Value&& value, QString&& infoHashV1);

        class File final {
        public:
            explicit File(bencode::Value&& value);

            bencode::Integer size;

            std::ranges::view auto path() {
                return mPath |
                       std::views::transform([](bencode::Value& value) { return std::move(value).takeString(); });
            }

        private:
            bencode::List mPath;
        };

        QString infoHashV1;
        std::vector<std::set<QString>> trackers;

        QString rootFileName;

        inline bool isSingleFile() const { return std::holds_alternative<bencode::Integer>(mSingleFileSizeOrFiles); }

        inline bencode::Integer singleFileSize() const { return std::get<bencode::Integer>(mSingleFileSizeOrFiles); }

        inline std::ranges::view auto files() {
            return std::get<bencode::List>(mSingleFileSizeOrFiles) |
                   std::views::transform([](bencode::Value& value) { return File(std::move(value)); });
        }

    private:
        std::variant<bencode::Integer, bencode::List> mSingleFileSizeOrFiles;

        friend struct fmt::formatter<tremotesf::TorrentMetainfoFile>;
    };

    /**
     * @throws tremotesf::bencode::Error
     */
    TorrentMetainfoFile parseTorrentFile(const QString& path);

    QDebug operator<<(QDebug debug, const TorrentMetainfoFile& torrentFile);
}

template<>
struct fmt::formatter<tremotesf::TorrentMetainfoFile> : tremotesf::SimpleFormatter {
    format_context::iterator format(const tremotesf::TorrentMetainfoFile& torrentFile, format_context& ctx) const;
};

#endif // TREMOTESF_TORRENTFILEPARSER_H
