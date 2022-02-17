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
#include "bencodeparser.h"

namespace tremotesf
{
    class TorrentFileParser;

    class LocalTorrentFilesModel : public BaseTorrentFilesModel
    {
        Q_OBJECT
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
        Q_PROPERTY(bool isSuccessfull READ isSuccessfull NOTIFY loadedChanged)
        Q_PROPERTY(QString errorString READ errorString NOTIFY loadedChanged)
        Q_PROPERTY(QVariantList unwantedFiles READ unwantedFiles NOTIFY wantedFilesChanged)
        Q_PROPERTY(QVariantList highPriorityFiles READ highPriorityFiles NOTIFY filesPriorityChanged)
        Q_PROPERTY(QVariantList lowPriorityFiles READ lowPriorityFiles NOTIFY filesPriorityChanged)
        Q_PROPERTY(QVariantMap renamedFiles READ renamedFiles NOTIFY renamedFilesChanged)
    public:
        explicit LocalTorrentFilesModel(QObject* parent = nullptr);

        Q_INVOKABLE void load(const QString& filePath);

        bool isLoaded() const;
        bool isSuccessfull() const;
        QString errorString() const;

        QVariantList unwantedFiles() const;
        QVariantList highPriorityFiles() const;
        QVariantList lowPriorityFiles() const;

        const QVariantMap& renamedFiles() const;

        void setFileWanted(const QModelIndex& index, bool wanted) override;
        void setFilesWanted(const QModelIndexList& indexes, bool wanted) override;
        void setFilePriority(const QModelIndex& index, tremotesf::TorrentFilesModelEntry::Priority priority) override;
        void setFilesPriority(const QModelIndexList& indexes, tremotesf::TorrentFilesModelEntry::Priority priority) override;

        void renameFile(const QModelIndex& index, const QString& newName) override;

    private:
        std::vector<TorrentFilesModelFile*> mFiles;
        bool mLoaded;
        bencode::Error mError;

        QVariantMap mRenamedFiles;

    signals:
        void loadedChanged();
        void wantedFilesChanged();
        void filesPriorityChanged();
        void renamedFilesChanged();
    };
}

#endif // TREMOTESF_LOCALTORRENTFILESMODEL_H
