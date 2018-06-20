#include "rpc.h"

#include <QCoreApplication>

#include "libtremotesf/torrent.h"
#include "servers.h"
#include "settings.h"

namespace tremotesf
{
    Rpc::Rpc(QObject* parent) : libtremotesf::Rpc(nullptr, parent)
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
                            emit finishedNotificationRequested(addedHashes, addedNames);
                        }
                    }
                }
            }
        });

        QObject::connect(this, &Rpc::torrentAdded, this, [=](const Torrent* torrent) {
            if (Settings::instance()->notificationOnAddingTorrent()) {
                emit addedNotificationRequested({torrent->hashString()}, {torrent->name()});
            }
        });

        QObject::connect(this, &Rpc::torrentFinished, this, [=](const Torrent* torrent) {
            if (Settings::instance()->notificationOfFinishedTorrents()) {
                emit finishedNotificationRequested({torrent->hashString()}, {torrent->name()});
            }
        });

        QObject::connect(this, &Rpc::aboutToDisconnect, this, [=]() {
            Servers::instance()->saveCurrentServerLastTorrents(this);
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
}
