// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESMODELENTRY_H
#define TREMOTESF_TORRENTFILESMODELENTRY_H

#include <variant>
#include <vector>

#include <QObject>
#include <QIcon>

#include "rpc/torrentfile.h"

namespace tremotesf {
    class TorrentFilesModelEntry {
        Q_GADGET

    public:
        enum class WantedState : char { Wanted, Unwanted, Mixed };
        Q_ENUM(WantedState)

        enum class Priority : char { Low, Normal, High, Mixed };
        Q_ENUM(Priority)

        static Priority fromFilePriority(TorrentFile::Priority priority);
        static TorrentFile::Priority toFilePriority(Priority priority);

        inline int row() const { return mRow; }

        inline TorrentFilesModelEntry* parentDirectory() const { return mParentDirectory; }
        inline void setParentDirectory(TorrentFilesModelEntry* directory) { mParentDirectory = directory; }

        inline QString name() const { return mName; }
        inline void setName(const QString& name) { mName = name; }

        QString path() const;

        inline long long size() const { return mSize; }
        void setSize(long long size) { mSize = size; }

        inline long long completedSize() const { return mCompletedSize; }
        inline double progress() const {
            return mSize > 0 ? static_cast<double>(mCompletedSize) / static_cast<double>(mSize) : 0.0;
        }

        inline WantedState wantedState() const { return mWantedState; }

        bool setWanted(bool wanted);

        inline Priority priority() const { return mPriority; }
        QString priorityString() const;

        bool setPriority(Priority priority);

        bool update(bool wanted, Priority priority, long long completedSize);
        bool update(WantedState wantedState, Priority priority, long long completedSize);

        QIcon icon() const;
        void getFileIds(std::vector<int>& ids) const;

        inline bool isDirectory() const { return std::holds_alternative<DirectoryData>(mFileOrDirectoryData); }

        inline std::vector<TorrentFilesModelEntry>& children() {
            return std::get<DirectoryData>(mFileOrDirectoryData).children;
        }
        inline const std::vector<TorrentFilesModelEntry>& children() const {
            return std::get<DirectoryData>(mFileOrDirectoryData).children;
        }

        bool recalculateFromChildren();

        inline int fileId() const { return std::get<FileData>(mFileOrDirectoryData).id; }

        inline static TorrentFilesModelEntry createFile(
            int id, int row, QString name, long long size, long long completedSize, bool wanted, Priority priority
        ) {
            return TorrentFilesModelEntry(
                row,
                std::move(name),
                size,
                completedSize,
                wanted,
                priority,
                FileData{.id = id}
            );
        }

        inline static TorrentFilesModelEntry createDirectory(int row, QString name) {
            return TorrentFilesModelEntry(row, std::move(name), DirectoryData{});
        }

    private:
        TorrentFilesModelEntry* mParentDirectory{};
        QString mName;
        long long mSize{};
        long long mCompletedSize{};
        int mRow;
        WantedState mWantedState{};
        Priority mPriority{};

        struct FileData {
            mutable QIcon icon{};
            mutable bool initializedIcon{};
            int id;
        };

        struct DirectoryData {
            std::vector<TorrentFilesModelEntry> children;
        };

        std::variant<FileData, DirectoryData> mFileOrDirectoryData;

        inline TorrentFilesModelEntry(
            int row,
            QString name,
            long long size,
            long long completedSize,
            bool wanted,
            Priority priority,
            FileData data
        )
            : mName(std::move(name)),
              mSize(size),
              mCompletedSize(completedSize),
              mRow(row),
              mWantedState(wanted ? WantedState::Wanted : WantedState::Unwanted),
              mPriority(priority),
              mFileOrDirectoryData(std::move(data)) {}

        inline TorrentFilesModelEntry(int row, QString name, DirectoryData data)
            : mName(std::move(name)), mRow(row), mFileOrDirectoryData(std::move(data)) {}
    };
}

#endif // TREMOTESF_TORRENTFILESMODELENTRY_H
