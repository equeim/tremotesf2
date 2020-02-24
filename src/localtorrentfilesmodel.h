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

#ifndef TREMOTESF_LOCALTORRENTFILESMODEL_H
#define TREMOTESF_LOCALTORRENTFILESMODEL_H

#include <vector>
#include <QVariant>
#include <QVariantMap>

#include "basetorrentfilesmodel.h"

namespace tremotesf
{
    class TorrentFileParser;

    class LocalTorrentFilesModel : public BaseTorrentFilesModel
    {
        Q_OBJECT
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
        Q_PROPERTY(QVariantList unwantedFiles READ unwantedFiles)
        Q_PROPERTY(QVariantList highPriorityFiles READ highPriorityFiles)
        Q_PROPERTY(QVariantList lowPriorityFiles READ lowPriorityFiles)
        Q_PROPERTY(QVariantMap renamedFiles READ renamedFiles)
    public:
        explicit LocalTorrentFilesModel(const QVariantMap& parseResult = QVariantMap(),
                                        QObject* parent = nullptr);

        Q_INVOKABLE void load(tremotesf::TorrentFileParser* parser);

        bool isLoaded() const;

        QVariantList unwantedFiles() const;
        QVariantList highPriorityFiles() const;
        QVariantList lowPriorityFiles() const;

        const QVariantMap& renamedFiles() const;

        void setFileWanted(const QModelIndex& index, bool wanted) override;
        void setFilesWanted(const QModelIndexList& indexes, bool wanted) override;
        void setFilePriority(const QModelIndex& index, tremotesf::TorrentFilesModelEntry::Priority priority) override;
        void setFilesPriority(const QModelIndexList& indexes, tremotesf::TorrentFilesModelEntry::Priority priority) override;

        void renameFile(const QModelIndex& index, const QString& newName) override;

#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif
    private:
        void load(const QVariantMap& parseResult);

        std::vector<TorrentFilesModelFile*> mFiles;
        bool mLoaded;

        QVariantMap mRenamedFiles;
    signals:
        void loadedChanged();
    };
}

#endif // TREMOTESF_LOCALTORRENTFILESMODEL_H
