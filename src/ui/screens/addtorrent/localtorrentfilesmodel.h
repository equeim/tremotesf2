// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LOCALTORRENTFILESMODEL_H
#define TREMOTESF_LOCALTORRENTFILESMODEL_H

#include <map>
#include <vector>

#include "ui/itemmodels/basetorrentfilesmodel.h"
#include "coroutines/coroutinefwd.h"

namespace tremotesf {
    class TorrentMetainfoFile;

    class LocalTorrentFilesModel final : public BaseTorrentFilesModel {
        Q_OBJECT

    public:
        explicit LocalTorrentFilesModel(QObject* parent = nullptr);

        /**
         * @throws bencode::Error
         */
        Coroutine<> load(TorrentMetainfoFile torrentFile);

        bool isLoaded() const;

        std::vector<int> unwantedFiles() const;
        std::vector<int> highPriorityFiles() const;
        std::vector<int> lowPriorityFiles() const;

        const std::map<QString, QString>& renamedFiles() const;

        void renameFile(const QModelIndex& index, const QString& newName) override;

    private:
        std::vector<TorrentFilesModelFile*> mFiles{};
        bool mLoaded{};

        std::map<QString, QString> mRenamedFiles{};
    };
}

#endif // TREMOTESF_LOCALTORRENTFILESMODEL_H
