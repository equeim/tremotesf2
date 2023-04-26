// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESMODELENTRY_H
#define TREMOTESF_TORRENTFILESMODELENTRY_H

#include <memory>
#include <vector>
#include <unordered_map>

#include <QObject>

#include "libtremotesf/torrentfile.h"

namespace tremotesf {
    class TorrentFilesModelDirectory;

    class TorrentFilesModelEntry {
        Q_GADGET

    public:
        enum WantedState { Wanted, Unwanted, MixedWanted };
        Q_ENUM(WantedState)

        enum Priority { LowPriority, NormalPriority, HighPriority, MixedPriority };
        Q_ENUM(Priority)

        static Priority fromFilePriority(libtremotesf::TorrentFile::Priority priority);
        static libtremotesf::TorrentFile::Priority toFilePriority(Priority priority);

        TorrentFilesModelEntry() = default;
        explicit TorrentFilesModelEntry(int row, TorrentFilesModelDirectory* parentDirectory, QString name);
        virtual ~TorrentFilesModelEntry() = default;

        int row() const;
        TorrentFilesModelDirectory* parentDirectory() const;

        QString name() const;
        void setName(const QString& name);

        QString path() const;

        virtual bool isDirectory() const = 0;

        virtual long long size() const = 0;
        virtual long long completedSize() const = 0;
        virtual double progress() const = 0;

        virtual WantedState wantedState() const = 0;
        virtual void setWanted(bool wanted) = 0;

        virtual Priority priority() const = 0;
        QString priorityString() const;
        virtual void setPriority(Priority priority) = 0;

        virtual bool isChanged() const = 0;

    private:
        int mRow = 0;
        TorrentFilesModelDirectory* mParentDirectory = nullptr;
        QString mName;
    };

    class TorrentFilesModelFile;

    class TorrentFilesModelDirectory final : public TorrentFilesModelEntry {

    public:
        TorrentFilesModelDirectory() = default;
        explicit TorrentFilesModelDirectory(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name);

        bool isDirectory() const override;
        long long size() const override;
        long long completedSize() const override;
        double progress() const override;
        WantedState wantedState() const override;
        void setWanted(bool wanted) override;
        Priority priority() const override;
        void setPriority(Priority priority) override;

        const std::vector<std::unique_ptr<TorrentFilesModelEntry>>& children() const;
        const std::unordered_map<QString, TorrentFilesModelEntry*>& childrenHash() const;

        TorrentFilesModelFile* addFile(int id, const QString& name, long long size);
        TorrentFilesModelDirectory* addDirectory(const QString& name);

        void clearChildren();
        std::vector<int> childrenIds() const;

        bool isChanged() const override;

    private:
        void addChild(std::unique_ptr<TorrentFilesModelEntry>&& child);

        std::vector<std::unique_ptr<TorrentFilesModelEntry>> mChildren;
        std::unordered_map<QString, TorrentFilesModelEntry*> mChildrenHash;
    };

    class TorrentFilesModelFile final : public TorrentFilesModelEntry {

    public:
        explicit TorrentFilesModelFile(
            int row, TorrentFilesModelDirectory* parentDirectory, int id, const QString& name, long long size
        );

        bool isDirectory() const override;
        long long size() const override;
        long long completedSize() const override;
        double progress() const override;
        WantedState wantedState() const override;
        void setWanted(bool wanted) override;
        Priority priority() const override;
        void setPriority(Priority priority) override;

        bool isChanged() const override;
        void setChanged(bool changed);

        int id() const;
        void setSize(long long size);
        void setCompletedSize(long long completedSize);

    private:
        long long mSize;
        long long mCompletedSize;
        WantedState mWantedState;
        Priority mPriority;
        int mId;

        bool mChanged;
    };
}

#endif // TREMOTESF_TORRENTFILESMODELENTRY_H
