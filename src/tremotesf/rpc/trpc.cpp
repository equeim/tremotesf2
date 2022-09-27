#include "trpc.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStringBuilder>

#include "libtremotesf/serversettings.h"
#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/servers.h"
#include "tremotesf/settings.h"

namespace tremotesf
{
    Rpc::Rpc(QObject* parent)
        : libtremotesf::Rpc(parent),
          mIncompleteDirectoryMounted(false)
    {
        QObject::connect(this, &Rpc::connectedChanged, this, [=] {
            if (isConnected()) {
                const bool notifyOnAdded = Settings::instance()->notificationsOnAddedTorrentsSinceLastConnection();
                const bool notifyOnFinished = Settings::instance()->notificationsOnFinishedTorrentsSinceLastConnection();
                if (notifyOnAdded || notifyOnFinished) {
                    const LastTorrents lastTorrents(Servers::instance()->currentServerLastTorrents());
                    if (lastTorrents.saved) {
                        QStringList addedHashes;
                        QStringList addedNames;
                        QStringList finishedHashes;
                        QStringList finishedNames;
                        for (const auto& torrent : torrents()) {
                            const QString hashString(torrent->hashString());
                            const auto found = std::find_if(lastTorrents.torrents.cbegin(),
                                                            lastTorrents.torrents.cend(),
                                                            [&hashString](const auto& torrent) {
                                return torrent.hashString == hashString;
                            });
                            if (found == lastTorrents.torrents.cend()) {
                                if (notifyOnAdded) {
                                    addedHashes.push_back(torrent->hashString());
                                    addedNames.push_back(torrent->name());
                                }
                            } else {
                                if (notifyOnFinished && !found->finished && torrent->isFinished()) {
                                    finishedHashes.push_back(torrent->hashString());
                                    finishedNames.push_back(torrent->name());
                                }
                            }
                        }

                        if (notifyOnAdded && !addedHashes.isEmpty()) {
                            emit addedNotificationRequested(addedHashes, addedNames);
                        }
                        if (notifyOnFinished && !finishedHashes.isEmpty()) {
                            emit finishedNotificationRequested(finishedHashes, finishedNames);
                        }
                    }
                }
            } else {
                mIncompleteDirectoryMounted = false;
            }
        });

        QObject::connect(this, &Rpc::torrentAdded, this, [=](const auto* torrent) {
            if (Settings::instance()->notificationOnAddingTorrent()) {
                emit addedNotificationRequested({torrent->hashString()}, {torrent->name()});
            }
        });

        QObject::connect(this, &Rpc::torrentFinished, this, [=](const auto* torrent) {
            if (Settings::instance()->notificationOfFinishedTorrents()) {
                emit finishedNotificationRequested({torrent->hashString()}, {torrent->name()});
            }
        });

        QObject::connect(this, &Rpc::aboutToDisconnect, this, [=] {
            Servers::instance()->saveCurrentServerLastTorrents(this);
        });

        QObject::connect(this, &Rpc::torrentsUpdated, this, [=] {
            mMountedIncompleteDirectory = Servers::instance()->fromRemoteToLocalDirectory(serverSettings()->incompleteDirectory());
            mIncompleteDirectoryMounted = !mMountedIncompleteDirectory.isEmpty();
        });
    }

    QString Rpc::statusString() const
    {
        switch (connectionState()) {
        case ConnectionState::Disconnected:
            switch (error()) {
            case Error::NoError:
                return qApp->translate("tremotesf", "Disconnected");
            case Error::TimedOut:
                return qApp->translate("tremotesf", "Timed out");
            case Error::ConnectionError:
                return qApp->translate("tremotesf", "Connection error");
            case Error::AuthenticationError:
                return qApp->translate("tremotesf", "Authentication error");
            case Error::ParseError:
                return qApp->translate("tremotesf", "Parse error");
            case Error::ServerIsTooNew:
                return qApp->translate("tremotesf", "Server is too new");
            case Error::ServerIsTooOld:
                return qApp->translate("tremotesf", "Server is too old");
            }
            break;
        case ConnectionState::Connecting:
            return qApp->translate("tremotesf", "Connecting...");
        case ConnectionState::Connected:
            return qApp->translate("tremotesf", "Connected");
        }

        return QString();
    }

    bool Rpc::isIncompleteDirectoryMounted() const
    {
        return mIncompleteDirectoryMounted;
    }

    bool Rpc::isTorrentLocalMounted(libtremotesf::Torrent* torrent) const
    {
        return isLocal() ||
                (Servers::instance()->currentServerHasMountedDirectories() &&
                 (serverSettings()->isIncompleteDirectoryEnabled() && torrent->leftUntilDone() > 0 ? mIncompleteDirectoryMounted : true) &&
                 !Servers::instance()->fromRemoteToLocalDirectory(torrent->downloadDirectory()).isEmpty());
    }

    QString Rpc::localTorrentDownloadDirectoryPath(libtremotesf::Torrent* torrent) const
    {
        const bool incompleteDirectoryEnabled = serverSettings()->isIncompleteDirectoryEnabled();
        QString filePath;
        if (isLocal()) {
            if (incompleteDirectoryEnabled &&
                    torrent->leftUntilDone() > 0 &&
                    QFileInfo::exists(serverSettings()->incompleteDirectory() % '/' % torrentRootFileName(torrent))) {
                filePath = serverSettings()->incompleteDirectory();
            } else {
                filePath = torrent->downloadDirectory();
            }
        } else {
            if (Servers::instance()->currentServerHasMountedDirectories()) {
                if (incompleteDirectoryEnabled &&
                        torrent->leftUntilDone() > 0 &&
                        mIncompleteDirectoryMounted &&
                        QFileInfo::exists(mMountedIncompleteDirectory % '/' % torrentRootFileName(torrent))) {
                    filePath = mMountedIncompleteDirectory;
                } else {
                    filePath = Servers::instance()->fromRemoteToLocalDirectory(torrent->downloadDirectory());
                }
            }
        }
        return filePath;
    }

    QString Rpc::localTorrentFilesPath(libtremotesf::Torrent* torrent) const
    {
        const QString downloadDirectoryPath(localTorrentDownloadDirectoryPath(torrent));
        if (downloadDirectoryPath.isEmpty()) {
            return QString();
        }
        return downloadDirectoryPath % '/' % torrentRootFileName(torrent);
    }

    QString Rpc::torrentRootFileName(const libtremotesf::Torrent* torrent) const
    {
        if (torrent->isSingleFile() && torrent->leftUntilDone() > 0 && serverSettings()->renameIncompleteFiles()) {
            return torrent->name() % QLatin1String(".part");
        }
        return torrent->name();
    }
}
