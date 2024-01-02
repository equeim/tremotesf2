// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QUrl>
#include <QWidget>

#include "literals.h"
#include "log/log.h"
#include "desktoputils.h"
#include "tremotesf_dbus_generated/org.freedesktop.FileManager1.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

namespace tremotesf {
    namespace {
        class FreedesktopFileManagerLauncher final : public impl::FileManagerLauncher {
            Q_OBJECT

        public:
            FreedesktopFileManagerLauncher() = default;

        protected:
            void launchFileManagerAndSelectFiles(
                std::vector<FilesInDirectory> filesToSelect, QPointer<QWidget> parentWidget
            ) override {
                logInfo("FreedesktopFileManagerLauncher: executing org.freedesktop.FileManager1.ShowItems() D-Bus call"
                );
                OrgFreedesktopFileManager1Interface interface(
                    "org.freedesktop.FileManager1"_l1,
                    "/org/freedesktop/FileManager1"_l1,
                    QDBusConnection::sessionBus()
                );
                interface.setTimeout(desktoputils::defaultDbusTimeout);
                QStringList uris{};
                for (const auto& [_, dirFiles] : filesToSelect) {
                    for (const QString& filePath : dirFiles) {
                        uris.push_back(QUrl::fromLocalFile(filePath).toString());
                    }
                }
                const auto pendingReply = interface.ShowItems(uris, {});
                const auto onFinished = [=, this] {
                    if (!pendingReply.isError()) {
                        logInfo("FreedesktopFileManagerLauncher: executed org.freedesktop.FileManager1.ShowItems() "
                                "D-Bus call");
                    } else {
                        logWarning(
                            "FreedesktopFileManagerLauncher: org.freedesktop.FileManager1.ShowItems() D-Bus call "
                            "failed: {}",
                            pendingReply.error()
                        );
                        for (const auto& [dirPath, dirFiles] : filesToSelect) {
                            fallbackForDirectory(dirPath, parentWidget);
                        }
                    }
                    emit done();
                };
                if (pendingReply.isFinished()) {
                    onFinished();
                }
                auto watcher = new QDBusPendingCallWatcher(pendingReply, this);
                QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, onFinished);
                QObject::connect(
                    watcher,
                    &QDBusPendingCallWatcher::finished,
                    watcher,
                    &QDBusPendingCallWatcher::deleteLater
                );
            }
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() { return new FreedesktopFileManagerLauncher(); }
    }
}

#include "filemanagerlauncher_freedesktop.moc"
