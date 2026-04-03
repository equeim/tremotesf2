// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESMODELENTRY_H
#define TREMOTESF_TORRENTFILESMODELENTRY_H

#include <memory>
#include <vector>

#include <QObject>
#include <QIcon>

#include "rpc/torrentfile.h"

namespace tremotesf {
    class TorrentFilesModelDirectory;

    class TorrentFilesModelEntry {
        Q_GADGET
    public:
        enum class WantedState { Wanted, Unwanted, Mixed };
        Q_ENUM(WantedState)

        enum class Priority { Low, Normal, High, Mixed };
        Q_ENUM(Priority)

        static Priority fromFilePriority(TorrentFile::Priority priority);
        static TorrentFile::Priority toFilePriority(Priority priority);

        TorrentFilesModelEntry() = default;
        explicit TorrentFilesModelEntry(
            int row,
            TorrentFilesModelDirectory* parentDirectory,
            QString name,
            long long size,
            long long completedSize,
            bool wanted,
            TorrentFilesModelEntry::Priority priority
        );
        virtual ~TorrentFilesModelEntry() = default;
        Q_DISABLE_COPY_MOVE(TorrentFilesModelEntry)

        inline int row() const { return mRow; }
        inline TorrentFilesModelDirectory* parentDirectory() const { return mParentDirectory; }

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

        virtual bool isDirectory() const = 0;
        virtual QIcon icon() const = 0;

    protected:
        int mRow{};
        TorrentFilesModelDirectory* mParentDirectory{};
        QString mName;
        long long mSize;
        long long mCompletedSize;
        WantedState mWantedState;
        Priority mPriority;
    };

    class TorrentFilesModelFile;

    class TorrentFilesModelDirectory final : public TorrentFilesModelEntry {
    public:
        TorrentFilesModelDirectory() = default;
        explicit TorrentFilesModelDirectory(int row, TorrentFilesModelDirectory* parentDirectory, QString name);

        inline bool isDirectory() const override { return true; }

        inline const std::vector<std::unique_ptr<TorrentFilesModelEntry>>& children() const { return mChildren; }

        TorrentFilesModelFile* addFile(
            int id,
            const QString& name,
            long long size,
            long long completedSize,
            bool wanted,
            TorrentFilesModelEntry::Priority priority
        );
        TorrentFilesModelDirectory* addDirectory(const QString& name);

        void clearChildren();
        std::vector<int> childrenIds() const;

        QIcon icon() const override;

        bool recalculateFromChildren();

    private:
        void addChild(std::unique_ptr<TorrentFilesModelEntry>&& child);

        std::vector<std::unique_ptr<TorrentFilesModelEntry>> mChildren;
    };

    class TorrentFilesModelFile final : public TorrentFilesModelEntry {
    public:
        explicit TorrentFilesModelFile(
            int row,
            TorrentFilesModelDirectory* parentDirectory,
            int id,
            QString name,
            long long size,
            long long completedSize,
            bool wanted,
            TorrentFilesModelEntry::Priority priority
        );

        inline bool isDirectory() const override { return false; }

        QIcon icon() const override;

        inline int id() const { return mId; }

    private:
        mutable QIcon mIcon{};
        mutable bool mInitializedIcon{};
        int mId;
    };
}

#endif // TREMOTESF_TORRENTFILESMODELENTRY_H
