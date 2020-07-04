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

#include <array>
#include <cmath>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QLocale>

#include "torrentfileparser.h"

#ifdef TREMOTESF_SAILFISHOS
#include <QModelIndexList>
#include <QQuickItem>
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

#else
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrl>
#ifdef QT_DBUS_LIB
#include <functional>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QProcess>
#include <QRegularExpression>
#include <QVersionNumber>

#include "org.freedesktop.FileManager1.h"
#include "org.xfce.FileManager.h"

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
            YobiByte,
            NumberOfByteUnits
        };

        struct ByteUnitStrings
        {
            enum Type
            {
                Size,
                Speed
            };
            std::array<QString(*)(), 2> strings;
        };

        // Should be kept in sync with `enum ByteUnit`
        const std::array<ByteUnitStrings, NumberOfByteUnits> byteUnits{
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 B"); }, [] { return qApp->translate("tremotesf", "%L1 B/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 KiB"); }, [] { return qApp->translate("tremotesf", "%L1 KiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 MiB"); }, [] { return qApp->translate("tremotesf", "%L1 MiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 GiB"); }, [] { return qApp->translate("tremotesf", "%L1 GiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 TiB"); }, [] { return qApp->translate("tremotesf", "%L1 TiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 PiB"); }, [] { return qApp->translate("tremotesf", "%L1 PiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 EiB"); }, [] { return qApp->translate("tremotesf", "%L1 EiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 ZiB"); }, [] { return qApp->translate("tremotesf", "%L1 ZiB/s"); }},
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 YiB"); }, [] { return qApp->translate("tremotesf", "%L1 YiB/s"); }},
        };

        QString formatBytes(long long bytes, ByteUnitStrings::Type stringType)
        {
            size_t unit = 0;
            auto bytes_d = static_cast<double>(bytes);
            while (bytes_d >= 1024.0 && unit <= NumberOfByteUnits) {
                bytes_d /= 1024.0;
                ++unit;
            }

            if (unit == Byte) {
                return byteUnits[Byte].strings[stringType]().arg(bytes_d);
            }
            return byteUnits[unit].strings[stringType]().arg(bytes_d, 0, 'f', 1);
        }
    }

    QString Utils::formatByteSize(long long size)
    {
        return formatBytes(size, ByteUnitStrings::Size);
    }

    QString Utils::formatByteSpeed(long long speed)
    {
        return formatBytes(speed, ByteUnitStrings::Speed);
    }

    QString Utils::formatSpeedLimit(int limit)
    {
        return qApp->translate("tremotesf", "%L1 KiB/s").arg(limit);
    }

    QString Utils::formatProgress(double progress)
    {
        if (qFuzzyCompare(progress, 1.0)) {
            return qApp->translate("tremotesf", "%L1%").arg(100);
        } else {
            return qApp->translate("tremotesf", "%L1%").arg(std::trunc(progress * 1000.0) / 10.0, 0, 'f', 1);
        }
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
        return QLocale().toString(ratio, 'f', precision);
    }

    QString Utils::formatRatio(long long downloaded, long long uploaded)
    {
        if (downloaded == 0) {
            return formatRatio(0);
        }
        return formatRatio(static_cast<double>(uploaded) / static_cast<double>(downloaded));
    }

    QString Utils::formatEta(int seconds)
    {
        if (seconds < 0) {
            return u8"\u221E";
        }

        const int days = seconds / 86400;
        seconds %= 86400;
        const int hours = seconds / 3600;
        seconds %= 3600;
        const int minutes = seconds / 60;
        seconds %= 60;

        if (days > 0) {
            return qApp->translate("tremotesf", "%L1 d %L2 h").arg(days).arg(hours);
        }

        if (hours > 0) {
            return qApp->translate("tremotesf", "%L1 h %L2 m").arg(hours).arg(minutes);
        }

        if (minutes > 0) {
            return qApp->translate("tremotesf", "%L1 m %L2 s").arg(minutes).arg(seconds);
        }

        return qApp->translate("tremotesf", "%L1 s").arg(seconds);
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

    bool Utils::fileExists(const QString& filePath)
    {
        return QFile::exists(filePath);
    }

    QStringList Utils::splitByNewlines(const QString& string)
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

    QQuickItem* Utils::nextItemInFocusChainNotLooping(QQuickItem* rootItem, QQuickItem* currentItem)
    {
        bool foundCurrent = false;
        return nextItemInFocusChainNotLoopingRecursive(rootItem, currentItem, foundCurrent);
    }
#else
    QString Utils::statusIconPath(Utils::StatusIcon icon)
    {
#if defined(Q_OS_WIN) && !defined(TEST_BUILD)
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

    namespace
    {
        class FileManagerLauncher
        {
        public:
            explicit FileManagerLauncher(const QStringList& files, QWidget* parentWidget)
                : mFiles(files),
                  mParentWidget(parentWidget)
            {

            }

            void showInFileManager()
            {
#ifdef QT_DBUS_LIB
                executeProcess(QLatin1String("xdg-mime"), {QLatin1String("query"),
                                                           QLatin1String("default"),
                                                           QLatin1String("inode/directory")},
                               [=](QProcess* xdgMimeProcess) {
                    const QByteArray fileManager(xdgMimeProcess->readLine().trimmed());
                    qDebug() << "Detected file manager" << fileManager;
                    if (fileManager == "dolphin.desktop" || fileManager == "org.kde.dolphin.desktop") {
                        launchFileManagerProcess("Dolphin", QLatin1String("dolphin"), mFiles);
                    } else if (fileManager == "nautilus.desktop" || fileManager == "org.gnome.Nautilus.desktop" || fileManager == "nautilus-folder-handler.desktop") {
                        showInNautilusOrOpenParentDirectories();
                    } else if (fileManager == "nemo.desktop") {
                        launchFileManagerProcess("Nemo", QLatin1String("nemo"), mFiles);
                    } else if (fileManager == "Thunar.desktop" || fileManager == "Thunar-folder-handler.desktop") {
                        showInThunarOrOpenParentDirectories();
                    } else {
                        showInFreedesktopFileManagerOrOpenParentDirectories();
                    }
                }, [=] { showInFreedesktopFileManagerOrOpenParentDirectories(); });

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
                openParentDirectoriesForFiles();
#endif
            }

        private:
#ifdef QT_DBUS_LIB
            void showInFreedesktopFileManagerOrOpenParentDirectories()
            {
                qDebug("Executing org.freedesktop.FileManager1 D-Bus call");
                OrgFreedesktopFileManager1Interface interface(QLatin1String("org.freedesktop.FileManager1"),
                                                              QLatin1String("/org/freedesktop/FileManager1"),
                                                              QDBusConnection::sessionBus());
                if (interface.isValid()) {
                    auto pending(interface.ShowItems(mFiles, QString()));
                    auto watcher = new QDBusPendingCallWatcher(pending, qApp);
                    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, qApp, [=] {
                        if (watcher->isError()) {
                            qWarning() << "org.freedesktop.FileManager1 D-Bus call failed" << watcher->error();
                            openParentDirectoriesForFiles();
                        }
                        qDebug("Executed org.freedesktop.FileManager1 D-Bus call");
                        watcher->deleteLater();
                    });
                } else {
                    qDebug("org.freedesktop.FileManager1 is not a valid interface");
                    openParentDirectoriesForFiles();
                }
            }

            void showInThunarOrOpenParentDirectories()
            {
                qDebug("Executing thunar D-Bus calls");
                OrgXfceFileManagerInterface interface(QLatin1String("org.xfce.FileManager"),
                                                      QLatin1String("/org/xfce/FileManager"),
                                                      QDBusConnection::sessionBus());
                if (interface.isValid()) {
                    for (const QString& filePath : mFiles) {
                        const QFileInfo info(filePath);
                        auto pending(interface.DisplayFolderAndSelect(info.path(), info.fileName(), {}, {}));
                        auto watcher = new QDBusPendingCallWatcher(pending, qApp);
                        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, qApp, [=] {
                            if (watcher->isError()) {
                                qWarning() << "Thunar D-Bus call failed, error string" << watcher->error();
                                openParentDirectory(filePath, mParentWidget);
                            }
                            watcher->deleteLater();
                        });
                    }
                    qDebug("Finished executing Thunar D-Bus calls");
                } else {
                    qWarning("org.xfce.FileManager is not a valid interface");
                    openParentDirectoriesForFiles();
                }
            }

            void executeProcess(QLatin1String exeName,
                                const QStringList& arguments,
                                const std::function<void(QProcess*)>& onSuccess,
                                const std::function<void()>& onFailure)
            {
                auto process = new QProcess(qApp);

                const auto onFailedToStart = [=] {
                    qWarning() << "Process" << process->program()
                               << "failed to start, error" << process->error() << process->errorString();
                    process->deleteLater();
                    onFailure();
                };

                QObject::connect(process, &QProcess::errorOccurred, qApp, [=](QProcess::ProcessError error) {
                    if (error == QProcess::FailedToStart) {
                        onFailedToStart();
                    }
                });

                QObject::connect(process,
                                 static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                                 qApp,
                                 [=](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        onSuccess(process);
                    } else {
                        qWarning().nospace() << "Process " << process->program()
                                   << ' ' << process->arguments()
                                   << " failed, exitStatus " << exitStatus
                                   << ", exitCode " << exitCode
                                   << ", error string " << process->errorString()
                                   << ", stderr:";
                        qWarning() << QString(process->readAllStandardError());
                        onFailure();
                    }
                    process->deleteLater();
                });

                qDebug() << "Executing" << exeName << "with arguments" << arguments;
                process->start(exeName, arguments);

                if (process->state() == QProcess::NotRunning) {
                    onFailedToStart();
                }
            }

            void launchFileManagerProcess(const char* fileManagerName, QLatin1String exeName, const QStringList& args)
            {
                qDebug("Launching %s", fileManagerName);
                if (!QProcess::startDetached(exeName, args)) {
                    qWarning("Failed to launch %s", fileManagerName);
                    openParentDirectoriesForFiles();
                }
            }

            void showInNautilusOrOpenParentDirectories()
            {
                executeProcess(QLatin1String("nautilus"), {QLatin1String("--version")}, [=](QProcess* nautilusProcess) {
                    const QByteArray processOutput(nautilusProcess->readLine().trimmed());
                    const QVersionNumber version(QVersionNumber::fromString(QRegularExpression(QLatin1String("[0-9.]+")).match(processOutput).captured()));
                    if (version.isNull()) {
                        qWarning() << "Failed to detect Nautilus version, process output was" << processOutput;
                        openParentDirectoriesForFiles();
                    } else {
                        qDebug() << "Detected Nautilus version" << version;
                        if (version < QVersionNumber(3, 8, 0)) {
                            qDebug("Nautilus version is too old");
                            openParentDirectoriesForFiles();
                        } else {
                            QStringList args{QLatin1String("--select")};
                            if (version < QVersionNumber(3, 28, 0)) {
                                args.push_back(QLatin1String("--no-desktop"));
                            }
                            launchFileManagerProcess("Nautilus", QLatin1String("nautilus"), args + mFiles);
                        }
                    }
                }, [=] { openParentDirectoriesForFiles(); });
            }
#endif

#ifndef Q_OS_WIN
            void openParentDirectoriesForFiles()
            {
                for (const QString& filePath : mFiles) {
                    openParentDirectory(filePath, mParentWidget);
                }
            }
#endif

            void openParentDirectory(const QString& filePath, QWidget* parent)
            {
                const QString directory(QFileInfo(filePath).path());
                qDebug() << "Executing QDesktopServices::openUrl() for" << directory;
                if (!QDesktopServices::openUrl(QUrl::fromLocalFile(directory))) {
                    qWarning("QDesktopServices::openUrl() failed");
                    auto dialog = new QMessageBox(QMessageBox::Warning,
                                                  qApp->translate("tremotesf", "Error"),
                                                  qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(directory)),
                                                  QMessageBox::Close,
                                                  parent);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->show();
                }
            }

            const QStringList mFiles;
            QWidget* mParentWidget;
        };
    }

    void Utils::selectFilesInFileManager(const QStringList& files, QWidget* parent)
    {
        FileManagerLauncher launcher(files, parent);
        launcher.showInFileManager();
    }
#endif // TREMOTESF_SAILFISHOS
}
