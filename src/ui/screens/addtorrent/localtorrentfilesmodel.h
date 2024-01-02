// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LOCALTORRENTFILESMODEL_H
#define TREMOTESF_LOCALTORRENTFILESMODEL_H

#include <map>
#include <optional>
#include <vector>

#include "ui/itemmodels/basetorrentfilesmodel.h"
#include "bencodeparser.h"

namespace tremotesf {
    class TorrentFileParser;

    class LocalTorrentFilesModel final : public BaseTorrentFilesModel {
        Q_OBJECT

    public:
        explicit LocalTorrentFilesModel(QObject* parent = nullptr);

        void load(const QString& filePath);

        bool isLoaded() const;
        bool isSuccessfull() const;
        QString errorString() const;

        std::vector<int> unwantedFiles() const;
        std::vector<int> highPriorityFiles() const;
        std::vector<int> lowPriorityFiles() const;

        const std::map<QString, QString>& renamedFiles() const;

        void renameFile(const QModelIndex& index, const QString& newName) override;

    private:
        std::vector<TorrentFilesModelFile*> mFiles{};
        bool mLoaded{};
        std::optional<bencode::Error::Type> mErrorType{};

        std::map<QString, QString> mRenamedFiles{};

    signals:
        void loadedChanged();
    };
}

#endif // TREMOTESF_LOCALTORRENTFILESMODEL_H
