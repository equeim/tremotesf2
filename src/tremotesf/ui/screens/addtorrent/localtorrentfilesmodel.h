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

#include <optional>
#include <vector>
#include <QVariant>
#include <QVariantMap>

#include "tremotesf/ui/itemmodels/basetorrentfilesmodel.h"
#include "tremotesf/bencodeparser.h"

namespace tremotesf
{
    class TorrentFileParser;

    class LocalTorrentFilesModel : public BaseTorrentFilesModel
    {
        Q_OBJECT
    public:
        explicit LocalTorrentFilesModel(QObject* parent = nullptr);

        void load(const QString& filePath);

        bool isLoaded() const;
        bool isSuccessfull() const;
        QString errorString() const;

        QVariantList unwantedFiles() const;
        QVariantList highPriorityFiles() const;
        QVariantList lowPriorityFiles() const;

        const QVariantMap& renamedFiles() const;

        void renameFile(const QModelIndex& index, const QString& newName) override;

    private:
        std::vector<TorrentFilesModelFile*> mFiles;
        bool mLoaded;
        std::optional<bencode::Error::Type> mErrorType;

        QVariantMap mRenamedFiles;

    signals:
        void loadedChanged();
    };
}

#endif // TREMOTESF_LOCALTORRENTFILESMODEL_H
