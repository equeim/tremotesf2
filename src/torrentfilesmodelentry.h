/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include <QObject>
#include <QString>
#include <QVariantList>

namespace tremotesf
{
    class TorrentFilesModelEntryEnums : public QObject
    {
        Q_OBJECT
        Q_ENUMS(WantedState)
        Q_ENUMS(Priority)
    public:
        enum WantedState
        {
            Wanted,
            Unwanted,
            MixedWanted
        };

        enum Priority
        {
            LowPriority = -1,
            NormalPriority,
            HighPriority,
            MixedPriority
        };
    };

    class TorrentFilesModelDirectory;

    class TorrentFilesModelEntry
    {
    public:
        TorrentFilesModelEntry() = default;
        explicit TorrentFilesModelEntry(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name);
        virtual ~TorrentFilesModelEntry() = default;

        int row() const;
        TorrentFilesModelDirectory* parentDirectory() const;

        QString name() const;
        void setName(const QString& name);

        virtual bool isDirectory() const = 0;

        virtual long long size() const = 0;
        virtual long long completedSize() const = 0;
        virtual float progress() const = 0;

        virtual TorrentFilesModelEntryEnums::WantedState wantedState() const = 0;
        virtual void setWanted(bool wanted) = 0;

        virtual TorrentFilesModelEntryEnums::Priority priority() const = 0;
        QString priorityString() const;
        virtual void setPriority(TorrentFilesModelEntryEnums::Priority priority) = 0;

        virtual bool isChanged() const = 0;

    private:
        int mRow;
        TorrentFilesModelDirectory* mParentDirectory;
        QString mName;
    };

    class TorrentFilesModelDirectory : public TorrentFilesModelEntry
    {
    public:
        TorrentFilesModelDirectory() = default;
        explicit TorrentFilesModelDirectory(int row, TorrentFilesModelDirectory* parentDirectory, const QString& name);
        ~TorrentFilesModelDirectory() override;

        bool isDirectory() const override;
        long long size() const override;
        long long completedSize() const override;
        float progress() const override;
        TorrentFilesModelEntryEnums::WantedState wantedState() const override;
        void setWanted(bool wanted) override;
        TorrentFilesModelEntryEnums::Priority priority() const override;
        void setPriority(TorrentFilesModelEntryEnums::Priority priority) override;

        const QList<TorrentFilesModelEntry*>& children() const;
        const QHash<QString, TorrentFilesModelEntry*>& childrenHash() const;
        void addChild(TorrentFilesModelEntry* child);
        void clearChildren();
        QVariantList childrenIds() const;

        bool isChanged() const override;

    private:
        QList<TorrentFilesModelEntry*> mChildren;
        QHash<QString, TorrentFilesModelEntry*> mChildrenHash;
    };

    class TorrentFilesModelFile : public TorrentFilesModelEntry
    {
    public:
        explicit TorrentFilesModelFile(int row,
                                       TorrentFilesModelDirectory* parentDirectory,
                                       int id,
                                       const QString& name,
                                       long long size);

        bool isDirectory() const override;
        long long size() const override;
        long long completedSize() const override;
        float progress() const override;
        TorrentFilesModelEntryEnums::WantedState wantedState() const override;
        void setWanted(bool wanted) override;
        TorrentFilesModelEntryEnums::Priority priority() const override;
        void setPriority(TorrentFilesModelEntryEnums::Priority priority) override;

        bool isChanged() const override;
        void setChanged(bool changed);

        int id() const;
        void setSize(long long size);
        void setCompletedSize(long long completedSize);

    private:
        long long mSize;
        long long mCompletedSize;
        TorrentFilesModelEntryEnums::WantedState mWantedState;
        TorrentFilesModelEntryEnums::Priority mPriority;
        int mId;

        bool mChanged;
    };
}

#endif // TREMOTESF_TORRENTFILESMODELENTRY_H
