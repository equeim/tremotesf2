/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_LOCALTORRENTFILESMODEL_H
#define TREMOTESF_LOCALTORRENTFILESMODEL_H

#include <memory>
#include <QVariantMap>

#include "basetorrentfilesmodel.h"

namespace tremotesf
{
    class LocalTorrentFilesModel : public BaseTorrentFilesModel
    {
        Q_OBJECT
        Q_PROPERTY(QVariantList wantedFiles READ wantedFiles)
        Q_PROPERTY(QVariantList unwantedFiles READ unwantedFiles)
        Q_PROPERTY(QVariantList highPriorityFiles READ highPriorityFiles)
        Q_PROPERTY(QVariantList normalPriorityFiles READ normalPriorityFiles)
        Q_PROPERTY(QVariantList lowPriorityFiles READ lowPriorityFiles)
    public:
#ifdef TREMOTESF_SAILFISHOS
        enum Role
        {
            NameRole = Qt::UserRole,
            IsDirectoryRole,
            SizeRole,
            WantedStateRole,
            PriorityRole
        };
        Q_ENUMS(Role)
#else
        enum Column
        {
            NameColumn,
            SizeColumn,
            PriorityColumn,
            ColumnCount
        };
#endif

        LocalTorrentFilesModel() = default;
        explicit LocalTorrentFilesModel(const QVariantMap& parseResult, QObject* parent = nullptr);

        int columnCount(const QModelIndex& = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
#ifndef TREMOTESF_SAILFISHOS
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
#endif

        Q_INVOKABLE void setParseResult(const QVariantMap& result);

        QVariantList wantedFiles() const;
        QVariantList unwantedFiles() const;
        QVariantList highPriorityFiles() const;
        QVariantList normalPriorityFiles() const;
        QVariantList lowPriorityFiles() const;

        Q_INVOKABLE void setFileWanted(const QModelIndex& index, bool wanted) override;
        Q_INVOKABLE void setFilesWanted(const QModelIndexList& indexes, bool wanted) override;
        Q_INVOKABLE void setFilePriority(const QModelIndex& index, tremotesf::TorrentFilesModelEntryEnums::Priority priority) override;
        Q_INVOKABLE void setFilesPriority(const QModelIndexList& indexes, tremotesf::TorrentFilesModelEntryEnums::Priority priority) override;
#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif
    private:
        QList<TorrentFilesModelFile*> mFiles;
    };
}

#endif // TREMOTESF_LOCALTORRENTFILESMODEL_H
