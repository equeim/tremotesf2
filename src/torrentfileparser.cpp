// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCryptographicHash>

#include <fmt/ranges.h>

#include "torrentfileparser.h"

#include "fileutils.h"
#include "stdutils.h"

using namespace std::string_view_literals;

namespace tremotesf {
    namespace {
        constexpr auto infoKey = "info"sv;
        constexpr auto filesKey = "files"sv;
        constexpr auto pathKey = "path"sv;
        constexpr auto lengthKey = "length"sv;
        constexpr auto nameKey = "name"sv;
        constexpr auto announceListKey = "announce-list"sv;
        constexpr auto announceKey = "announce"sv;

        template<bencode::ValueType Expected>
        std::optional<Expected> maybeTakeDictValue(bencode::Dictionary& dict, std::string_view key) {
            const auto found = dict.find(key);
            return found != dict.end() ? std::optional(std::move(found->second).takeValue<Expected>()) : std::nullopt;
        }

        template<bencode::ValueType Expected>
        Expected takeDictValue(bencode::Dictionary& dict, std::string_view key, std::string_view dictName) {
            auto found = dict.find(key);
            if (found == dict.end()) {
                throw bencode::Error(
                    bencode::Error::Type::Parsing,
                    fmt::format("{} dictionary does not contain key '{}'", dictName, key)
                );
            }
            return std::move(found->second).takeValue<Expected>();
        }

        QString computeInfoHashV1(const QString& path, qint64 infoDictOffset, qint64 infoDictLength) {
            try {
                QFile file(path);
                openFile(file, QIODevice::ReadOnly);
                skipBytes(file, infoDictOffset);

                QCryptographicHash hash(QCryptographicHash::Sha1);

                static constexpr QByteArray::size_type maxBufferSize = 1024 * 1024 * 1024; // 1 MiB
                QByteArray buffer(std::min(maxBufferSize, static_cast<QByteArray::size_type>(infoDictLength)), '\0');

                auto remaining = static_cast<QByteArray::size_type>(infoDictLength);
                while (remaining > 0) {
                    if (remaining < buffer.size()) {
                        buffer.resize(remaining);
                    }
                    readBytes(file, buffer);
                    hash.addData(buffer);
                    remaining -= buffer.size();
                }

                return {hash.result().toHex()};
            } catch (const QFileError&) {
                std::throw_with_nested(bencode::Error(bencode::Error::Type::Reading, "Failed to compute info hash"));
            }
        }
    }

    TorrentMetainfoFile::File::File(bencode::Value&& value) {
        auto dict = std::move(value).takeDictionary();
        mPath = takeDictValue<bencode::List>(dict, pathKey, "info");
        size = takeDictValue<bencode::Integer>(dict, lengthKey, "info");
    }

    TorrentMetainfoFile::TorrentMetainfoFile(bencode::Value&& value, QString&& infoHashV1)
        : infoHashV1(std::move(infoHashV1)) {
        auto rootDict = std::move(value).takeDictionary();
        if (auto announceList = maybeTakeDictValue<bencode::List>(rootDict, announceListKey); announceList) {
            trackers = toContainer<std::vector>(*announceList | std::views::transform([](bencode::Value& value) {
                return toContainer<std::set>(
                    std::move(value).takeList() |
                    std::views::transform([](bencode::Value& value) { return std::move(value).takeString(); })
                );
            }));
        } else if (auto announce = maybeTakeDictValue<QString>(rootDict, announceKey); announce) {
            trackers = std::vector{std::set{std::move(*announce)}};
        }
        auto infoDict = takeDictValue<bencode::Dictionary>(rootDict, infoKey, "root");
        rootFileName = takeDictValue<QString>(infoDict, nameKey, "info");
        if (auto size = maybeTakeDictValue<bencode::Integer>(infoDict, lengthKey); size) {
            mSingleFileSizeOrFiles = *size;
        } else {
            mSingleFileSizeOrFiles = takeDictValue<bencode::List>(infoDict, filesKey, "info");
        }
    }

    TorrentMetainfoFile parseTorrentFile(const QString& path) {
        std::optional<std::pair<qint64, qint64>> infoDictOffsetAndLength{};
        auto bencodeValue = bencode::parse(path, [&](const bencode::ByteArray& key, qint64 offset, qint64 length) {
            if (key == infoKey) {
                infoDictOffsetAndLength = std::pair(offset, length);
            }
        });
        if (!infoDictOffsetAndLength.has_value()) {
            throw bencode::Error(bencode::Error::Type::Parsing, "root dictionary does not contain key 'info'");
        }
        const auto [offset, length] = *infoDictOffsetAndLength;
        return TorrentMetainfoFile(std::move(bencodeValue), computeInfoHashV1(path, offset, length));
    }

    QDebug operator<<(QDebug debug, const TorrentMetainfoFile& torrentFile) {
        const QDebugStateSaver saver(debug);
        debug.noquote() << fmt::format(impl::singleArgumentFormatString, torrentFile).c_str();
        return debug;
    }
}

fmt::format_context::iterator fmt::formatter<tremotesf::TorrentMetainfoFile>::format(
    const tremotesf::TorrentMetainfoFile& torrentFile, format_context& ctx
) const {
    return fmt::format_to(
        ctx.out(),
        "TorrentMetainfoFile(infoHashV1={}, trackers={}, rootFileName={:?}, filesCount={})",
        torrentFile.infoHashV1,
        torrentFile.trackers,
        torrentFile.rootFileName,
        torrentFile.isSingleFile() ? 1 : std::get<tremotesf::bencode::List>(torrentFile.mSingleFileSizeOrFiles).size()
    );
}
