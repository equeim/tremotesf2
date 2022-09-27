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
