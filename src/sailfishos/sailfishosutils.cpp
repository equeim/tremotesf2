/*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#include "sailfishosutils.h"

#include <QFile>
#include <QModelIndexList>
#include <QQuickItem>
#include <qqml.h>

#include "../alltrackersmodel.h"
#include "../baseproxymodel.h"
#include "../downloaddirectoriesmodel.h"
#include "../localtorrentfilesmodel.h"
#include "../peersmodel.h"
#include "../servers.h"
#include "../serversmodel.h"
#include "../settings.h"
#include "../statusfilterstats.h"
#include "../torrentfilesmodel.h"
#include "../torrentfilesmodelentry.h"
#include "../torrentfilesproxymodel.h"
#include "../torrentsmodel.h"
#include "../torrentsproxymodel.h"
#include "../trackersmodel.h"
#include "../trpc.h"

#include "../libtremotesf/serversettings.h"
#include "../libtremotesf/serverstats.h"
#include "../libtremotesf/torrent.h"

#include "directorycontentmodel.h"
#include "selectionmodel.h"

Q_DECLARE_METATYPE(libtremotesf::Server)

namespace tremotesf
{
    void SailfishOSUtils::registerTypes()
    {
        qRegisterMetaType<QModelIndexList>();

        const char* url = "harbour.tremotesf";
        const int versionMajor = 1;
        const int versionMinor = 0;

        qmlRegisterSingletonType<Settings>(url,
                                           versionMajor,
                                           versionMinor,
                                           "Settings",
                                           [](QQmlEngine*, QJSEngine*) -> QObject* {
                                               return Settings::instance();
                                           });

        qmlRegisterSingletonType<Servers>(url,
                                          versionMajor,
                                          versionMinor,
                                          "Servers",
                                          [](QQmlEngine*, QJSEngine*) -> QObject* {
                                              return Servers::instance();
                                          });

        qmlRegisterType<Rpc>(url, versionMajor, versionMinor, "Rpc");
        qRegisterMetaType<libtremotesf::Server>();
        qmlRegisterUncreatableType<libtremotesf::ServerSettings>(url, versionMajor, versionMinor, "ServerSettings", QString());
        qmlRegisterType<libtremotesf::ServerStats>();
        qRegisterMetaType<libtremotesf::SessionStats>();
        qmlRegisterUncreatableType<libtremotesf::Torrent>(url, versionMajor, versionMinor, "Torrent", QString());

        qmlRegisterType<BaseProxyModel>(url, versionMajor, versionMinor, "BaseProxyModel");

        qmlRegisterType<ServersModel>(url, versionMajor, versionMinor, "ServersModel");
        qmlRegisterUncreatableType<Server>(url, versionMajor, versionMinor, "Server", QString());
        qRegisterMetaType<Server::ProxyType>();

        qmlRegisterType<StatusFilterStats>(url, versionMajor, versionMinor, "StatusFilterStats");
        qmlRegisterType<AllTrackersModel>(url, versionMajor, versionMinor, "AllTrackersModel");

        qmlRegisterType<TorrentsModel>(url, versionMajor, versionMinor, "TorrentsModel");
        qmlRegisterType<TorrentsProxyModel>(url, versionMajor, versionMinor, "TorrentsProxyModel");

        qmlRegisterType<TorrentFilesModel>(url, versionMajor, versionMinor, "TorrentFilesModel");
        qmlRegisterUncreatableType<TorrentFilesModelEntry>(url, versionMajor, versionMinor, "TorrentFilesModelEntry", QString());
        qRegisterMetaType<TorrentFilesModelEntry::Priority>();
        qmlRegisterType<TorrentFilesProxyModel>(url, versionMajor, versionMinor, "TorrentFilesProxyModel");

        qmlRegisterType<TrackersModel>(url, versionMajor, versionMinor, "TrackersModel");
        qmlRegisterType<DownloadDirectoriesModel>(url, versionMajor, versionMinor, "DownloadDirectoriesModel");

        qmlRegisterType<PeersModel>(url, versionMajor, versionMinor, "PeersModel");

        qmlRegisterType<SelectionModel>(url, versionMajor, versionMinor, "SelectionModel");

        qmlRegisterType<DirectoryContentModel>(url, versionMajor, versionMinor, "DirectoryContentModel");

        qmlRegisterType<LocalTorrentFilesModel>(url, versionMajor, versionMinor, "LocalTorrentFilesModel");

        qmlRegisterSingletonType<SailfishOSUtils>(url,
                                                  versionMajor,
                                                  versionMinor,
                                                  "Utils",
                                                  [](QQmlEngine*, QJSEngine*) -> QObject* {
                                                      return new SailfishOSUtils();
                                                  });
    }

    QString SailfishOSUtils::sdcardPath()
    {
        QFile mtab(QLatin1String("/etc/mtab"));
        if (mtab.open(QIODevice::ReadOnly)) {
            const QStringList mmcblk1p1(QString(mtab.readAll()).split('\n').filter(QLatin1String("/dev/mmcblk1p1")));
            if (!mmcblk1p1.isEmpty()) {
                return mmcblk1p1.first().split(' ').at(1);
            }
        }
        return QLatin1String("/media/sdcard");
    }

    bool SailfishOSUtils::fileExists(const QString& filePath)
    {
        return QFile::exists(filePath);
    }

    QStringList SailfishOSUtils::splitByNewlines(const QString& string)
    {
        return string.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    }

    namespace
    {
        QQuickItem* nextItemInFocusChainNotLoopingRecursive(QQuickItem* parent, QQuickItem* currentItem, bool& foundCurrent)
        {
            for (QQuickItem* child : parent->childItems()) {
                if (child->isVisible() && child->isEnabled()) {
                    if (foundCurrent) {
                        if (child->activeFocusOnTab()) {
                            // Found
                            return child;
                        }
                    } else if (child == currentItem) {
                        foundCurrent = true;
                    }
                    QQuickItem* recursive = nextItemInFocusChainNotLoopingRecursive(child, currentItem, foundCurrent);
                    if (recursive) {
                        return recursive;
                    }
                }
            }
            return nullptr;
        }
    }

    QQuickItem* SailfishOSUtils::nextItemInFocusChainNotLooping(QQuickItem* rootItem, QQuickItem* currentItem)
    {
        bool foundCurrent = false;
        return nextItemInFocusChainNotLoopingRecursive(rootItem, currentItem, foundCurrent);
    }
}
