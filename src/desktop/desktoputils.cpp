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

#include "desktoputils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QUrl>

#ifdef QT_DBUS_LIB
#include <functional>
#include <unordered_set>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QProcess>
#include <QRegularExpression>
#include <QVersionNumber>

#include "org.freedesktop.FileManager1.h"
#include "org.xfce.FileManager.h"

#elif defined(Q_OS_WIN)
#include <shlobj.h>
#endif

namespace tremotesf
{
    namespace desktoputils
    {
        QString statusIconPath(StatusIcon icon)
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

        void openFile(const QString& filePath, QWidget* parent)
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
            class FileManagerLauncher : public QObject
            {
                Q_OBJECT
            public:
                explicit FileManagerLauncher(const QStringList& files, QWidget* parentWidget)
                    : QObject(qApp),
                      mFiles(files),
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
                    }, [=] {
                        showInFreedesktopFileManagerOrOpenParentDirectories();
                    });

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
                    emit done();
#else
                    openParentDirectoriesForFiles();
                    emit done();
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
                        auto watcher = new QDBusPendingCallWatcher(pending, this);
                        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] {
                            if (watcher->isError()) {
                                qWarning() << "org.freedesktop.FileManager1 D-Bus call failed" << watcher->error();
                                openParentDirectoriesForFiles();
                            } else {
                                qDebug("Executed org.freedesktop.FileManager1 D-Bus call");
                                emit done();
                            }
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
                            auto watcher = new QDBusPendingCallWatcher(pending, this);
                            QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] {
                                if (watcher->isError()) {
                                    qWarning() << "Thunar D-Bus call failed, error string" << watcher->error();
                                    openParentDirectory(filePath, mParentWidget);
                                }
                                mPendingWatchers.erase(watcher);
                                watcher->deleteLater();
                                if (mPendingWatchers.empty()) {
                                    qDebug("Finished executing Thunar D-Bus calls");
                                    emit done();
                                }
                            });
                            mPendingWatchers.insert(watcher);
                        }
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
                    auto process = new QProcess(this);

                    const auto onFailedToStart = [=] {
                        qWarning() << "Process" << process->program()
                                   << "failed to start, error" << process->error() << process->errorString();
                        onFailure();
                    };

                    QObject::connect(process, &QProcess::errorOccurred, this, [=](QProcess::ProcessError error) {
                        if (error == QProcess::FailedToStart) {
                            onFailedToStart();
                        }
                    });

                    QObject::connect(process,
                                     static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                                     this,
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
                    if (QProcess::startDetached(exeName, args)) {
                        emit done();
                    } else {
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
                    emit done();
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

#ifdef QT_DBUS_LIB
                std::unordered_set<QDBusPendingCallWatcher*> mPendingWatchers;
#endif

            signals:
                void done();
            };
        }

        void selectFilesInFileManager(const QStringList& files, QWidget* parent)
        {
            const auto launcher = new FileManagerLauncher(files, parent);
            QObject::connect(launcher, &FileManagerLauncher::done, launcher, &QObject::deleteLater);
            launcher->showInFileManager();
        }
    }
}

#include "desktoputils.moc"
