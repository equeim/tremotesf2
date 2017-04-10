/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "utils.h"

#include <QCoreApplication>
#include <QFile>
#include <QLocale>

#include "torrentfileparser.h"

Q_DECLARE_METATYPE(tremotesf::TorrentFileParser::Error)

#ifdef TREMOTESF_SAILFISHOS
#include <QModelIndexList>
#include <qqml.h>

#include "alltrackersmodel.h"
#include "baseproxymodel.h"
#include "localtorrentfilesmodel.h"
#include "peersmodel.h"
#include "rpc.h"
#include "servers.h"
#include "serversmodel.h"
#include "serversettings.h"
#include "serverstats.h"
#include "settings.h"
#include "statusfilterstats.h"
#include "torrent.h"
#include "torrentfilesmodel.h"
#include "torrentfilesmodelentry.h"
#include "torrentfilesproxymodel.h"
#include "torrentsmodel.h"
#include "torrentsproxymodel.h"
#include "trackersmodel.h"
#include "utils.h"

#include "sailfishos/directorycontentmodel.h"
#include "sailfishos/selectionmodel.h"

Q_DECLARE_METATYPE(QModelIndexList)
Q_DECLARE_METATYPE(tremotesf::TorrentFilesModelEntryEnums::Priority)
#endif // TREMOTESF_SAILFISHOS

namespace tremotesf
{
    namespace
    {
        enum ByteUnit
        {
            Byte,
            KibiByte,
            MebiByte,
            GibiByte,
            TebiByte,
            PebiByte,
            ExbiByte,
            ZebiByte,
            YobiByte
        };

        QLocale locale;
    }

    QString Utils::formatByteSize(double size)
    {
        int unit = 0;
        while (size >= 1024.0 && unit < YobiByte) {
            size /= 1024.0;
            ++unit;
        }

        QString string;
        switch (unit) {
        case Byte:
            string = qApp->translate("tremotesf", "%1 B");
            break;
        case KibiByte:
            string = qApp->translate("tremotesf", "%1 KiB");
            break;
        case MebiByte:
            string = qApp->translate("tremotesf", "%1 MiB");
            break;
        case GibiByte:
            string = qApp->translate("tremotesf", "%1 GiB");
            break;
        case TebiByte:
            string = qApp->translate("tremotesf", "%1 TiB");
            break;
        case PebiByte:
            string = qApp->translate("tremotesf", "%1 PiB");
            break;
        case ExbiByte:
            string = qApp->translate("tremotesf", "%1 EiB");
            break;
        case ZebiByte:
            string = qApp->translate("tremotesf", "%1 ZiB");
            break;
        case YobiByte:
            string = qApp->translate("tremotesf", "%1 YiB");
        }

        if (unit == Byte) {
            return string.arg(locale.toString(size));
        }
        return string.arg(locale.toString(size, 'f', 1));
    }

    QString Utils::formatByteSpeed(double speed)
    {
        int unit = 0;
        while (speed >= 1024.0 && unit < YobiByte) {
            speed /= 1024.0;
            ++unit;
        }

        QString string;
        switch (unit) {
        case Byte:
            string = qApp->translate("tremotesf", "%1 B/s");
            break;
        case KibiByte:
            string = qApp->translate("tremotesf", "%1 KiB/s");
            break;
        case MebiByte:
            string = qApp->translate("tremotesf", "%1 MiB/s");
            break;
        case GibiByte:
            string = qApp->translate("tremotesf", "%1 GiB/s");
            break;
        case TebiByte:
            string = qApp->translate("tremotesf", "%1 TiB/s");
            break;
        case PebiByte:
            string = qApp->translate("tremotesf", "%1 PiB/s");
            break;
        case ExbiByte:
            string = qApp->translate("tremotesf", "%1 EiB/s");
            break;
        case ZebiByte:
            string = qApp->translate("tremotesf", "%1 ZiB/s");
            break;
        case YobiByte:
            string = qApp->translate("tremotesf", "%1 YiB/s");
        }

        if (unit == Byte) {
            return string.arg(locale.toString(speed));
        }
        return string.arg(locale.toString(speed, 'f', 1));
    }

    QString Utils::formatSpeedLimit(int limit)
    {
        return qApp->translate("tremotesf", "%1 KiB/s").arg(limit);
    }

    QString Utils::formatProgress(float progress)
    {
        QString numberString;
        if (progress == 1) {
            numberString = QLocale().toString(100);
        } else {
            numberString = QLocale().toString(int(progress * 1000) / 10.0, 'f', 1);
        }
        return QStringLiteral("%1%2").arg(numberString).arg(QLocale().percent());
    }

    QString Utils::formatRatio(float ratio)
    {
        if (ratio < 0) {
            return QString();
        }

        int precision = 2;
        if (ratio >= 100) {
            precision = 0;
        } else if (ratio >= 10) {
            precision = 1;
        }
        return locale.toString(ratio, 'f', precision);
    }

    QString Utils::formatEta(int seconds)
    {
        if (seconds < 0) {
            return "\u221E";
        }

        const int days = seconds / 86400;
        seconds %= 86400;
        const int hours = seconds / 3600;
        seconds %= 3600;
        const int minutes = seconds / 60;
        seconds %= 60;

        const QLocale locale;

        if (days > 0) {
            return qApp->translate("tremotesf", "%1 d %2 h").arg(locale.toString(days)).arg(locale.toString(hours));
        }

        if (hours > 0) {
            return qApp->translate("tremotesf", "%1 h %2 m").arg(locale.toString(hours)).arg(locale.toString(minutes));
        }

        if (minutes > 0) {
            return qApp->translate("tremotesf", "%1 m %2 s").arg(locale.toString(minutes)).arg(locale.toString(seconds));
        }

        return qApp->translate("tremotesf", "%1 s").arg(locale.toString(seconds));
    }

    QString Utils::license()
    {
        QFile licenseFile(QLatin1String(":/license.html"));
        licenseFile.open(QFile::ReadOnly);
        return licenseFile.readAll();
    }

    QString Utils::translators()
    {
        QFile translatorsFile(QLatin1String(":/translators.html"));
        translatorsFile.open(QFile::ReadOnly);
        return translatorsFile.readAll();
    }

    void Utils::registerTypes()
    {
        qRegisterMetaType<TorrentFileParser::Error>();

#ifdef TREMOTESF_SAILFISHOS
        qRegisterMetaType<QModelIndexList>();
        qRegisterMetaType<tremotesf::TorrentFilesModelEntryEnums::Priority>();

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
        qmlRegisterUncreatableType<ServerSettings>(url, versionMajor, versionMinor, "ServerSettings", QString());
        qmlRegisterType<ServerStats>();
        qmlRegisterUncreatableType<Torrent>(url, versionMajor, versionMinor, "Torrent", QString());

        qmlRegisterType<BaseProxyModel>(url, versionMajor, versionMinor, "BaseProxyModel");

        qmlRegisterType<ServersModel>(url, versionMajor, versionMinor, "ServersModel");

        qmlRegisterType<StatusFilterStats>(url, versionMajor, versionMinor, "StatusFilterStats");
        qmlRegisterType<AllTrackersModel>(url, versionMajor, versionMinor, "AllTrackersModel");

        qmlRegisterType<TorrentsModel>(url, versionMajor, versionMinor, "TorrentsModel");
        qmlRegisterType<TorrentsProxyModel>(url, versionMajor, versionMinor, "TorrentsProxyModel");

        qmlRegisterType<TorrentFilesModel>(url, versionMajor, versionMinor, "TorrentFilesModel");
        qmlRegisterUncreatableType<TorrentFilesModelEntryEnums>(url, versionMajor, versionMinor, "TorrentFilesModelEntryEnums", QString());
        qmlRegisterType<TorrentFilesProxyModel>(url, versionMajor, versionMinor, "TorrentFilesProxyModel");

        qmlRegisterType<TrackersModel>(url, versionMajor, versionMinor, "TrackersModel");

        qmlRegisterType<PeersModel>(url, versionMajor, versionMinor, "PeersModel");

        qmlRegisterType<SelectionModel>(url, versionMajor, versionMinor, "SelectionModel");

        qmlRegisterType<DirectoryContentModel>(url, versionMajor, versionMinor, "DirectoryContentModel");

        qmlRegisterType<TorrentFileParser>(url, versionMajor, versionMinor, "TorrentFileParser");
        qmlRegisterType<LocalTorrentFilesModel>(url, versionMajor, versionMinor, "LocalTorrentFilesModel");

        qmlRegisterSingletonType<Utils>(url,
                                        versionMajor,
                                        versionMinor,
                                        "Utils",
                                        [](QQmlEngine*, QJSEngine*) -> QObject* {
                                            return new tremotesf::Utils();
                                        });
#endif // TREMOTESF_SAILFISHOS
    }

#ifdef TREMOTESF_SAILFISHOS
    QString Utils::sdcardPath()
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
#else
    QString Utils::statusIconPath(Utils::StatusIcon icon)
    {
#ifdef Q_OS_WIN
        static const QString iconsPath(QLatin1String("%1/icons").arg(QCoreApplication::applicationDirPath()));
#else
        static const QString iconsPath(QLatin1String(ICONS_PATH));
#endif // Q_OS_WIN
        static const QString active(QString::fromLatin1("%1/active.png").arg(iconsPath));
        static const QString checking(QString::fromLatin1("%1/checking.png").arg(iconsPath));
        static const QString downloading(QString::fromLatin1("%1/downloading.png").arg(iconsPath));
        static const QString errored(QString::fromLatin1("%1/errored.png").arg(iconsPath));
        static const QString paused(QString::fromLatin1("%1/paused.png").arg(iconsPath));
        static const QString queued(QString::fromLatin1("%1/queued.png").arg(iconsPath));
        static const QString seeding(QString::fromLatin1("%1/seeding.png").arg(iconsPath));
        static const QString stalledDownloading(QString::fromLatin1("%1/stalled-downloading.png").arg(iconsPath));
        static const QString stalledSeeding(QString::fromLatin1("%1/stalled-seeding.png").arg(iconsPath));

        switch (icon) {
        case ActiveIcon:
            return active;
        case CheckingIcon:
            return checking;
        case DownloadingIcon:
            return downloading;
        case ErroredIcon:
            return errored;
        case PausedIcon:
            return paused;
        case QueuedIcon:
            return queued;
        case SeedingIcon:
            return seeding;
        case StalledDownloadingIcon:
            return stalledDownloading;
        case StalledSeedingIcon:
            return stalledSeeding;
        }

        return QString();
    }
#endif // TREMOTESF_SAILFISHOS
}
