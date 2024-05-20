// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESMODELENTRY_H
#define TREMOTESF_TORRENTFILESMODELENTRY_H

#include <memory>
#include <vector>
#include <variant>

#include <QObject>

#include <boost/container/flat_map.hpp>

#include "rpc/torrentfile.h"

namespace tremotesf {
    class TorrentFilesModelDirectory;

    class TorrentFilesModelEntry {
        Q_GADGET
    public:
        enum class WantedState : char { Wanted, Unwanted, Mixed };
        Q_ENUM(WantedState)

        enum class Priority : char { Low, Normal, High, Mixed };
        Q_ENUM(Priority)

        static Priority fromFilePriority(TorrentFile::Priority priority);
        static TorrentFile::Priority toFilePriority(Priority priority);

        TorrentFilesModelEntry() = default;
        explicit TorrentFilesModelEntry(
            int row,
            TorrentFilesModelDirectory* parentDirectory,
            const QString& name,
            long long size = 0,
            long long completedSize = 0,
            WantedState wantedState = WantedState::Wanted,
            Priority priority = Priority::Normal
        );
        virtual ~TorrentFilesModelEntry() = default;

        inline int row() const { return mRow; };
        inline TorrentFilesModelDirectory* parentDirectory() const { return mParentDirectory; };

        inline QString name() const { return mName; };
        void setName(const QString& name);

        QString path() const;

        virtual bool isDirectory() const = 0;

        inline long long size() const { return mSize; };
        inline long long completedSize() const { return mCompletedSize; };
        void setCompletedSize(long long completedSize);
        inline double progress() const { return static_cast<double>(mCompletedSize) / static_cast<double>(mSize); };

        inline WantedState wantedState() const { return mWantedState; };
        virtual void setWanted(bool wanted);

        inline Priority priority() const { return mPriority; };
        QString priorityString() const;
        virtual void setPriority(Priority priority);

        inline bool consumeChangedMark() { return std::exchange(mChangedMark, false); };

    protected:
        TorrentFilesModelDirectory* mParentDirectory{};
        QString mName{};
        long long mSize{};
        long long mCompletedSize{};
        WantedState mWantedState{WantedState::Wanted};
        Priority mPriority{Priority::Normal};
        bool mChangedMark{};
        int mRow{};
    };

    class TorrentFilesModelFile final : public TorrentFilesModelEntry {
    public:
        explicit TorrentFilesModelFile(
            int id,
            int row,
            TorrentFilesModelDirectory* parentDirectory,
            const QString& name,
            long long size,
            long long completedSize = 0,
            WantedState wantedState = WantedState::Wanted,
            Priority priority = Priority::Normal
        )
            : TorrentFilesModelEntry(row, parentDirectory, name, size, completedSize, wantedState, priority), mId(id) {}

        inline bool isDirectory() const override { return false; };
        inline int id() const { return mId; };

    private:
        int mId{};
    };

    class TorrentFilesModelDirectory final : public TorrentFilesModelEntry {
    public:
        inline TorrentFilesModelDirectory() = default;
        inline explicit TorrentFilesModelDirectory(
            int row, TorrentFilesModelDirectory* parentDirectory, const QString& name
        )
            : TorrentFilesModelEntry(row, parentDirectory, name) {};

        Q_DISABLE_COPY(TorrentFilesModelDirectory)

        using Child = std::variant<TorrentFilesModelFile, std::unique_ptr<TorrentFilesModelDirectory>>;
        using Children = boost::container::flat_map<QString, Child>;

        inline bool isDirectory() const override { return true; };

        void setWanted(bool wanted) override;
        void setPriority(Priority priority) override;

        inline int childrenCount() const { return static_cast<int>(mChildren.size()); }
        TorrentFilesModelEntry* childAtRow(int row);
        TorrentFilesModelEntry* childForName(const QString& name);

        void addFile(TorrentFilesModelFile&& file);
        TorrentFilesModelDirectory* addDirectory(const QString& name);

        std::vector<int> childrenIds() const;

        void initiallyCalculateFromAllChildrenRecursively(std::vector<TorrentFilesModelFile*>& files);
        void recalculateFromChildren();

        void forEachChild(std::function<void(TorrentFilesModelEntry*)>&& func);

    private:
        Children mChildren;
    };
}

#endif // TREMOTESF_TORRENTFILESMODELENTRY_H
