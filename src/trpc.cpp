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

#include "trpc.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStringBuilder>

#include "libtremotesf/serversettings.h"
#include "libtremotesf/torrent.h"
#include "servers.h"
#include "settings.h"

namespace tremotesf
{
    Rpc::Rpc(QObject* parent)
        : libtremotesf::Rpc(nullptr, parent),
          mIncompleteDirectoryMounted(false)
    {
        QObject::connect(this, &Rpc::statusChanged, this, &Rpc::statusStringChanged);
        QObject::connect(this, &Rpc::errorChanged, this, &Rpc::statusStringChanged);

        QObject::connect(this, &Rpc::connectedChanged, this, [=]() {
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
                                                            [&hashString](const LastTorrentsTorrent& torrent) {
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

        QObject::connect(this, &Rpc::torrentAdded, this, [=](const libtremotesf::Torrent* torrent) {
            if (Settings::instance()->notificationOnAddingTorrent()) {
                emit addedNotificationRequested({torrent->hashString()}, {torrent->name()});
            }
        });

        QObject::connect(this, &Rpc::torrentFinished, this, [=](const libtremotesf::Torrent* torrent) {
            if (Settings::instance()->notificationOfFinishedTorrents()) {
                emit finishedNotificationRequested({torrent->hashString()}, {torrent->name()});
            }
        });

        QObject::connect(this, &Rpc::aboutToDisconnect, this, [=]() {
            Servers::instance()->saveCurrentServerLastTorrents(this);
        });

        QObject::connect(this, &Rpc::torrentsUpdated, this, [=]() {
            mMountedIncompleteDirectory = Servers::instance()->fromRemoteToLocalDirectory(serverSettings()->incompleteDirectory());
            mIncompleteDirectoryMounted = !mMountedIncompleteDirectory.isEmpty();
        });
    }

    QString Rpc::statusString() const
    {
        switch (status()) {
        case Disconnected:
            switch (error()) {
            case NoError:
                return qApp->translate("tremotesf", "Disconnected");
            case TimedOut:
                return qApp->translate("tremotesf", "Timed out");
            case ConnectionError:
                return qApp->translate("tremotesf", "Connection error");
            case AuthenticationError:
                return qApp->translate("tremotesf", "Authentication error");
            case ParseError:
                return qApp->translate("tremotesf", "Parse error");
            case ServerIsTooNew:
                return qApp->translate("tremotesf", "Server is too new");
            case ServerIsTooOld:
                return qApp->translate("tremotesf", "Server is too old");
            }
            break;
        case Connecting:
            return qApp->translate("tremotesf", "Connecting...");
        case Connected:
            return qApp->translate("tremotesf", "Connected");
        }

        return QString();
    }

    bool Rpc::isIncompleteDirectoryMounted() const
    {
        return mIncompleteDirectoryMounted;
    }

    bool Rpc::isTorrentLocalMounted(const libtremotesf::Torrent* torrent) const
    {
        return isTorrentLocalMounted(torrent->downloadDirectory());
    }

    bool Rpc::isTorrentLocalMounted(const QString& downloadDirectory) const
    {
        return isLocal() ||
                (Servers::instance()->currentServerHasMountedDirectories() &&
                 (serverSettings()->isIncompleteDirectoryEnabled() ? mIncompleteDirectoryMounted : true) &&
                 !Servers::instance()->fromRemoteToLocalDirectory(downloadDirectory).isEmpty());
    }

    QString Rpc::localTorrentDownloadDirectoryPath(libtremotesf::Torrent* torrent) const
    {
        const bool incompleteDirectoryEnabled = serverSettings()->isIncompleteDirectoryEnabled();
        QString filePath;
        if (isLocal()) {
            filePath = incompleteDirectoryEnabled && QFileInfo::exists(serverSettings()->incompleteDirectory() % '/' % torrentRootFileName(torrent))
                    ? serverSettings()->incompleteDirectory()
                    : torrent->downloadDirectory();
        } else {
            if (incompleteDirectoryEnabled && mIncompleteDirectoryMounted) {
                filePath = QFileInfo::exists(mMountedIncompleteDirectory % '/' % torrentRootFileName(torrent))
                        ? mMountedIncompleteDirectory
                        : Servers::instance()->fromRemoteToLocalDirectory(torrent->downloadDirectory());
            } else {
                filePath = Servers::instance()->fromRemoteToLocalDirectory(torrent->downloadDirectory());
            }
        }
        return filePath;
    }

    QString Rpc::localTorrentFilesPath(libtremotesf::Torrent* torrent) const
    {
        return localTorrentDownloadDirectoryPath(torrent) % '/' % torrentRootFileName(torrent);
    }

    QString Rpc::torrentRootFileName(const libtremotesf::Torrent* torrent) const
    {
        if (torrent->isSingleFile() && torrent->leftUntilDone() > 0 && serverSettings()->renameIncompleteFiles()) {
            return torrent->name() % QLatin1String(".part");
        }
        return torrent->name();
    }
}
