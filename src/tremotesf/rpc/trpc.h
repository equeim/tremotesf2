#ifndef TREMOTESF_RPC_H
#define TREMOTESF_RPC_H

#include "libtremotesf/rpc.h"

namespace tremotesf
{
    class Rpc : public libtremotesf::Rpc
    {
        Q_OBJECT
    public:
        explicit Rpc(QObject* parent = nullptr);
        QString statusString() const;

        bool isIncompleteDirectoryMounted() const;
        bool isTorrentLocalMounted(libtremotesf::Torrent* torrent) const;
        QString localTorrentFilesPath(libtremotesf::Torrent* torrent) const;
        QString localTorrentDownloadDirectoryPath(libtremotesf::Torrent* torrent) const;
    private:
        QString torrentRootFileName(const libtremotesf::Torrent* torrent) const;

        bool mIncompleteDirectoryMounted;
        QString mMountedIncompleteDirectory;

    signals:
        void addedNotificationRequested(const QStringList& hashStrings, const QStringList& names);
        void finishedNotificationRequested(const QStringList& hashStrings, const QStringList& names);
    };
}

#endif // TREMOTESF_RPC_H
