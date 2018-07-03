/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_TORRENTFILESMODELENTRY_H
#define TREMOTESF_TORRENTFILESMODELENTRY_H

#include <vector>
#include <unordered_map>

#include <QObject>
#include <QVariantList>

#include "libtremotesf/stdutils.h"
#include "libtremotesf/torrent.h"

namespace tremotesf
{
    class TorrentFilesModelDirectory;

    class TorrentFilesModelEntry
    {
    public:
        enum WantedState
        {
            Wanted,
            Unwanted,
            MixedWanted
        };

        enum Priority
        {
            LowPriority,
            NormalPriority,
            HighPriority,
            MixedPriority
        };

        static Priority fromFilePriority(libtremotesf::TorrentFile::Priority priority);
        static libtremotesf::TorrentFile::Priority toFilePriority(Priority priority);

        TorrentFilesModelEntry() = default;
        explicit TorrentFilesModelEntry(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name, const QString& path);
        virtual ~TorrentFilesModelEntry() = default;

        int row() const;
        TorrentFilesModelDirectory* parentDirectory() const;

        QString name() const;
        void setName(const QString& name);

        QString path() const;
        void setPath(const QString& path);

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
        QString mPath;
    };

    class TorrentFilesModelDirectory : public TorrentFilesModelEntry
    {
    public:
        TorrentFilesModelDirectory() = default;
        explicit TorrentFilesModelDirectory(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name, const QString& path);
        ~TorrentFilesModelDirectory() override;

        bool isDirectory() const override;
        long long size() const override;
        long long completedSize() const override;
        double progress() const override;
        WantedState wantedState() const override;
        void setWanted(bool wanted) override;
        Priority priority() const override;
        void setPriority(Priority priority) override;

        const std::vector<TorrentFilesModelEntry*>& children() const;
        const std::unordered_map<QString, TorrentFilesModelEntry*>& childrenHash() const;
        void addChild(TorrentFilesModelEntry* child);
        void clearChildren();
        QVariantList childrenIds() const;

        bool isChanged() const override;

    private:
        std::vector<TorrentFilesModelEntry*> mChildren;
        std::unordered_map<QString, TorrentFilesModelEntry*> mChildrenHash;
    };

    class TorrentFilesModelFile : public TorrentFilesModelEntry
    {
    public:
        explicit TorrentFilesModelFile(int row,
                                       TorrentFilesModelDirectory* parentDirectory,
                                       int id,
                                       const QString& name,
                                       const QString& path,
                                       long long size);

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
