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

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QUrl>

#include "libtremotesf/println.h"
#include "../utils.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include "org.freedesktop.FileManager1.h"
SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)
#elif defined(Q_OS_WIN)
#include <shlobj.h>
#endif

namespace tremotesf
{
    namespace desktoputils
    {
        QString statusIconPath(StatusIcon icon)
        {
            switch (icon) {
            case ActiveIcon:
                return QLatin1String(":/active.png");
            case CheckingIcon:
                return QLatin1String(":/checking.png");
            case DownloadingIcon:
                return QLatin1String(":/downloading.png");
            case ErroredIcon:
                return QLatin1String(":/errored.png");
            case PausedIcon:
                return QLatin1String(":/paused.png");
            case QueuedIcon:
                return QLatin1String(":/queued.png");
            case SeedingIcon:
                return QLatin1String(":/seeding.png");
            case StalledDownloadingIcon:
                return QLatin1String(":/stalled-downloading.png");
            case StalledSeedingIcon:
                return QLatin1String(":/stalled-seeding.png");
            }

            return QString();
        }

        void openFile(const QString& filePath, QWidget* parent)
        {
            const QString nativePath(QDir::toNativeSeparators(filePath));
            if (const auto url = QUrl::fromLocalFile(nativePath); !QDesktopServices::openUrl(url)) {
                printlnWarning("QDesktopServices::openUrl() failed for {}", url);
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

                void showInFileManager() {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
                    printlnDebug("Executing org.freedesktop.FileManager1 D-Bus call");
                    OrgFreedesktopFileManager1Interface interface(QLatin1String("org.freedesktop.FileManager1"),
                                                                  QLatin1String("/org/freedesktop/FileManager1"),
                                                                  QDBusConnection::sessionBus());
                    interface.setTimeout(defaultDbusTimeout);
                    QStringList uris{};
                    uris.reserve(mFiles.size());
                    for (const QString& filePath : mFiles) {
                        uris.push_back(QUrl::fromLocalFile(filePath).toString());
                    }
                    const auto pendingReply = interface.ShowItems(uris, {});
                    auto watcher = new QDBusPendingCallWatcher(pendingReply, this);
                    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] {
                        if (!watcher->isError()) {
                            printlnInfo("Executed org.freedesktop.FileManager1 D-Bus call");
                        } else {
                            printlnWarning("org.freedesktop.FileManager1 D-Bus call failed: {}", watcher->error());
                            openParentDirectoriesForFiles();
                        }
                        watcher->deleteLater();
                        emit done();
                    });
#elif defined(Q_OS_WIN)
                    for (const QString& filePath : mFiles) {
                        const QString nativePath(QDir::toNativeSeparators(filePath));
                        PIDLIST_ABSOLUTE directory = ILCreateFromPathW(reinterpret_cast<PCWSTR>(nativePath.utf16()));
                        if (directory) {
                            printlnDebug("Executing SHOpenFolderAndSelectItems() for {}", nativePath);
                            try {
                                Utils::callCOMFunction([&] {
                                    return SHOpenFolderAndSelectItems(directory, 0, nullptr, 0);
                                });
                            } catch (const std::exception& e) {
                                printlnWarning("SHOpenFolderAndSelectItems failed for {}: {}", nativePath, e.what());
                                openParentDirectory(filePath, mParentWidget);
                            }
                            ILFree(directory);
                        } else {
                            printlnWarning("ILCreateFromPathW failed() for {}", nativePath);
                            openParentDirectory(filePath, mParentWidget);
                        }
                    }
                    emit done();
#else
                    openParentDirectoriesForFiles();
                    emit done();
#endif
                }

            private:
                [[maybe_unused]]
                void openParentDirectoriesForFiles()
                {
                    for (const QString& filePath : mFiles) {
                        openParentDirectory(filePath, mParentWidget);
                    }
                }

                static void openParentDirectory(const QString& filePath, QWidget* parent)
                {
                    const QString directory(QFileInfo(filePath).path());
                    printlnDebug("Executing QDesktopServices::openUrl() for {}", directory);
                    if (const auto url = QUrl::fromLocalFile(directory); !QDesktopServices::openUrl(url)) {
                        printlnWarning("QDesktopServices::openUrl() failed for {}", url);
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
                QWidget *const mParentWidget;

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

        namespace {
            QRegularExpression urlRegex()
            {
                const auto protocol = QLatin1String("(?:(?:[a-z]+:)?//)");
                const auto host = QLatin1String("(?:(?:[a-z\\x{00a1}-\\x{ffff0}-9][-_]*)*[a-z\\x{00a1}-\\x{ffff0}-9]+)");
                const auto domain = QLatin1String("(?:\\.(?:[a-z\\x{00a1}-\\x{ffff0}-9]-*)*[a-z\\x{00a1}-\\x{ffff0}-9]+)*");
                const auto tld = QLatin1String("(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}]{2,}))\\.?");
                const auto port = QLatin1String("(?::\\d{2,5})?");
                const auto path = QLatin1String("(?:[/?#][^\\s\"\\)\']*)?");
                const auto regex = QString(QLatin1String("(?:") % protocol % QLatin1String("|www\\.)(?:") % host % domain % tld % QLatin1String(")") % port % path);
                return QRegularExpression(regex, QRegularExpression::CaseInsensitiveOption);
            }
        }

        void findLinksAndAddAnchors(QTextDocument *document)
        {
            auto baseFormat = QTextCharFormat();
            baseFormat.setAnchor(true);
            baseFormat.setFontUnderline(true);
            baseFormat.setForeground(qApp->palette().link());
            const auto regex = desktoputils::urlRegex();
            auto cursor = QTextCursor();
            while (true) {
                cursor = document->find(regex, cursor);
                if (cursor.isNull()) {
                    break;
                }
                auto format = baseFormat;
                format.setAnchorHref(cursor.selection().toPlainText());
                cursor.setCharFormat(format);
            }
        }

    }
}

#include "desktoputils.moc"
