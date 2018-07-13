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

#include "utils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QLocale>

#include "torrentfileparser.h"

Q_DECLARE_METATYPE(tremotesf::TorrentFileParser::Error)

#ifdef TREMOTESF_SAILFISHOS
#include <QModelIndexList>
#include <qqml.h>

#include "alltrackersmodel.h"
#include "baseproxymodel.h"
#include "downloaddirectoriesmodel.h"
#include "localtorrentfilesmodel.h"
#include "peersmodel.h"
#include "servers.h"
#include "serversmodel.h"
#include "settings.h"
#include "statusfilterstats.h"
#include "torrentfilesmodel.h"
#include "torrentfilesmodelentry.h"
#include "torrentfilesproxymodel.h"
#include "torrentsmodel.h"
#include "torrentsproxymodel.h"
#include "trackersmodel.h"
#include "trpc.h"

#include "libtremotesf/serversettings.h"
#include "libtremotesf/serverstats.h"
#include "libtremotesf/torrent.h"

#include "sailfishos/directorycontentmodel.h"
#include "sailfishos/selectionmodel.h"

Q_DECLARE_METATYPE(libtremotesf::Server)
Q_DECLARE_METATYPE(tremotesf::TorrentFilesModelEntry::Priority)

namespace tremotesf
{
    class TorrentFilesModelEntryEnums : public QObject
    {
        Q_OBJECT
    public:
        enum WantedState
        {
            Wanted = TorrentFilesModelEntry::Wanted,
            Unwanted = TorrentFilesModelEntry::Unwanted,
            MixedWanted = TorrentFilesModelEntry::MixedWanted
        };
        Q_ENUM(WantedState)

        enum Priority
        {
            LowPriority = TorrentFilesModelEntry::LowPriority,
            NormalPriority = TorrentFilesModelEntry::NormalPriority,
            HighPriority = TorrentFilesModelEntry::HighPriority,
            MixedPriority = TorrentFilesModelEntry::MixedPriority
        };
        Q_ENUM(Priority)
    };
}
#else
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrl>
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QProcess>
#include <QRegularExpression>
#include <QVersionNumber>
#elif defined(Q_OS_WIN)
#include <shlobj.h>
#endif
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

    QString Utils::formatProgress(double progress)
    {
        QString numberString;
        if (qFuzzyCompare(progress, 1.0)) {
            numberString = QLocale().toString(100);
        } else {
            numberString = QLocale().toString(int(progress * 1000) / 10.0, 'f', 1);
        }
        return QStringLiteral("%1%2").arg(numberString).arg(QLocale().percent());
    }

    QString Utils::formatRatio(double ratio)
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

    QString Utils::formatRatio(long long downloaded, long long uploaded)
    {
        if (downloaded == 0) {
            return formatRatio(0);
        }
        return formatRatio(static_cast<double>(uploaded) / downloaded);
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
        qmlRegisterUncreatableType<libtremotesf::Torrent>(url, versionMajor, versionMinor, "Torrent", QString());

        qmlRegisterType<BaseProxyModel>(url, versionMajor, versionMinor, "BaseProxyModel");

        qmlRegisterType<ServersModel>(url, versionMajor, versionMinor, "ServersModel");

        qmlRegisterType<StatusFilterStats>(url, versionMajor, versionMinor, "StatusFilterStats");
        qmlRegisterType<AllTrackersModel>(url, versionMajor, versionMinor, "AllTrackersModel");

        qmlRegisterType<TorrentsModel>(url, versionMajor, versionMinor, "TorrentsModel");
        qmlRegisterType<TorrentsProxyModel>(url, versionMajor, versionMinor, "TorrentsProxyModel");

        qmlRegisterType<TorrentFilesModel>(url, versionMajor, versionMinor, "TorrentFilesModel");
        qmlRegisterUncreatableType<TorrentFilesModelEntryEnums>(url, versionMajor, versionMinor, "TorrentFilesModelEntry", QString());
        qRegisterMetaType<TorrentFilesModelEntry::Priority>();
        qmlRegisterType<TorrentFilesProxyModel>(url, versionMajor, versionMinor, "TorrentFilesProxyModel");

        qmlRegisterType<TrackersModel>(url, versionMajor, versionMinor, "TrackersModel");
        qmlRegisterType<DownloadDirectoriesModel>(url, versionMajor, versionMinor, "DownloadDirectoriesModel");

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
        static const QString iconsPath(QString::fromLatin1("%1/icons").arg(QCoreApplication::applicationDirPath()));
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

    void Utils::openFile(const QString& filePath, QWidget* parent)
    {
        const QString nativePath(QDir::toNativeSeparators(filePath));
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(nativePath))) {
            qWarning() << "QDesktopServices::openUrl" << nativePath << "failed";
            auto dialog = new QMessageBox(QMessageBox::Warning,
                                          qApp->translate("tremotesf", "Error"),
                                          qApp->translate("tremotesf", "Error opening %1").arg(nativePath),
                                          QMessageBox::Close,
                                          parent);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }

    void Utils::selectFilesInFileManager(const QStringList& files, QWidget* parent)
    {
        const auto openParentDirectory = [=](const QString& filePath) {
            const QString directory(QFileInfo(filePath).path());
            qDebug() << "executing QDesktopServices::openUrl" << directory;
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(directory))) {
                qWarning() << "QDesktopServices::openUrl" << directory << "failed";
                if (parent) {
                    auto dialog = new QMessageBox(QMessageBox::Warning,
                                                  qApp->translate("tremotesf", "Error"),
                                                  qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(directory)),
                                                  QMessageBox::Close,
                                                  parent);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->show();
                }
            }
        };

        const auto openParentDirectoryForAll = [=]() {
            for (const QString& filePath : files) {
                openParentDirectory(filePath);
            }
        };
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        auto xdgMimeProcess = new QProcess(qApp);
        QObject::connect(xdgMimeProcess,
                         static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                         qApp,
                         [=](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                const QByteArray fileManager(xdgMimeProcess->readLine().trimmed());
                qDebug() << "detected file manager:" << fileManager;
                if (fileManager == "dolphin.desktop" || fileManager == "org.kde.dolphin.desktop") {
                    qDebug() << "launching dolphin";
                    QProcess::startDetached(QLatin1String("dolphin"), QStringList{QLatin1String("--select")} + files);
                } else if (fileManager == "nautilus.desktop" || fileManager == "org.gnome.Nautilus.desktop" || fileManager == "nautilus-folder-handler.desktop") {
                    auto nautilusProcess = new QProcess(qApp);
                    QObject::connect(nautilusProcess,
                                     static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                                     qApp,
                                     [=](int exitCode, QProcess::ExitStatus exitStatus) {
                        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                            const QByteArray processOutput(nautilusProcess->readLine().trimmed());
                            const QVersionNumber version(QVersionNumber::fromString(QRegularExpression(QLatin1String("[0-9.]+")).match(processOutput).captured()));
                            if (version.isNull()) {
                                qWarning() << "failed to detect nautilus version, \"nautilus --version\" output was" << processOutput;
                            } else {
                                qDebug() << "detected nautilus version:" << version;
                                if (version >= QVersionNumber(3, 28, 0)) {
                                    qDebug() << "launching nautilus";
                                    QProcess::startDetached(QLatin1String("nautilus"), QStringList{QLatin1String("--select")} + files);
                                } else if (version >= QVersionNumber(3, 8, 0)) {
                                    qDebug() << "launching nautilus";
                                    QProcess::startDetached(QLatin1String("nautilus"), QStringList{QLatin1String("--no-desktop"), QLatin1String("--select")} + files);
                                } else {
                                    qWarning() << "nautilus version is too old";
                                    openParentDirectoryForAll();
                                }
                            }
                        } else {
                            qWarning().nospace() << "process " << nautilusProcess->program()
                                       << ' ' << nautilusProcess->arguments()
                                       << " failed, exitStatus " << exitStatus
                                       << ", exitCode " << exitCode
                                       << ", error string " << nautilusProcess->errorString()
                                       << ", stderr:";
                            qWarning() << QString(nautilusProcess->readAllStandardError());
                            openParentDirectoryForAll();
                        }
                        nautilusProcess->deleteLater();
                    });

                    nautilusProcess->start(QLatin1String("nautilus"), {QLatin1String("--versio")});
                    qDebug() << "executing" << nautilusProcess->program() << nautilusProcess->arguments();

                } else if (fileManager == "nemo.desktop") {
                    qDebug() << "launching nemo";
                    QProcess::startDetached(QLatin1String("nemo"), files);
                } else if (fileManager == "Thunar.desktop" || fileManager == "Thunar-folder-handler.desktop") {
                    qDebug() << "executing thunar dbus calls";

                    for (const QString& filePath : files) {
                        QDBusMessage message(QDBusMessage::createMethodCall(QLatin1String("org.xfce.FileManager"),
                                                                            QLatin1String("/org/xfce/FileManager"),
                                                                            QLatin1String("org.xfce.FileManager"),
                                                                            QLatin1String("DisplayFolderAndSelect")));
                        const QFileInfo info(filePath);
                        message.setArguments({info.path(), info.fileName(), QVariant(QVariant::String), QVariant(QVariant::String)});
                        auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), qApp);
                        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, qApp, [=]() {
                            QDBusPendingReply<void> reply(*watcher);
                            if (reply.isError()) {
                                qWarning() << "thunar dbus call failed, error string" << reply.error();
                                openParentDirectory(filePath);
                            }
                            watcher->deleteLater();
                        });
                    }
                } else {
                    openParentDirectoryForAll();
                }
            } else {
                qWarning().nospace() << "process " << xdgMimeProcess->program()
                           << ' ' << xdgMimeProcess->arguments()
                           << " failed, exitStatus " << exitStatus
                           << ", exitCode " << exitCode
                           << ", error string " << xdgMimeProcess->errorString()
                           << ", stderr:";
                qWarning() << QString(xdgMimeProcess->readAllStandardError());
                openParentDirectoryForAll();
            }
            xdgMimeProcess->deleteLater();
        });

        xdgMimeProcess->start(QLatin1String("xdg-mime"), {QLatin1String("query"),
                                                          QLatin1String("default"),
                                                          QLatin1String("inode/directory")});
        qDebug() << "executing" << xdgMimeProcess->program() << xdgMimeProcess->arguments();

#elif defined(Q_OS_WIN)
        for (const QString& filePath : files) {
            const QString nativePath(QDir::toNativeSeparators(filePath));
            PIDLIST_ABSOLUTE directory = ILCreateFromPathW(reinterpret_cast<PCWSTR>(nativePath.utf16()));
            if (directory) {
                qDebug() << "executing SHOpenFolderAndSelectItems" << nativePath;
                if (SHOpenFolderAndSelectItems(directory, 0, nullptr, 0) != S_OK) {
                    qWarning() << "SHOpenFolderAndSelectItems" << nativePath << "failed";
                    openParentDirectory(filePath);
                }
                ILFree(directory);
            } else {
                qWarning() << "ILCreateFromPathW" << nativePath << "failed";
                openParentDirectory(filePath);
            }
        }
#else
        openParentDirectoryForAll();
#endif
    }
#endif // TREMOTESF_SAILFISHOS
}

#include "utils.moc"
