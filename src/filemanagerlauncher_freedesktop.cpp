// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <ranges>
#include <fmt/ranges.h>

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QWidget>

#include "coroutines/dbus.h"
#include "coroutines/scope.h"
#include "log/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.FileManager1.h"
#include "desktoputils.h"
#include "literals.h"
#include "stdutils.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

using namespace std::views;

namespace tremotesf {
    namespace {
        class FreedesktopFileManagerLauncher final : public impl::FileManagerLauncher {
            Q_OBJECT

        public:
            FreedesktopFileManagerLauncher() = default;

        protected:
            void launchFileManagerAndSelectFiles(std::vector<FilesInDirectory> filesToSelect, QWidget* parentWidget)
                override {
                mCoroutineScope.launch(launchFileManagerAndSelectFilesImpl(std::move(filesToSelect), parentWidget));
            }

        private:
            Coroutine<> launchFileManagerAndSelectFilesImpl(
                std::vector<FilesInDirectory> filesToSelect, QPointer<QWidget> parentWidget
            ) {
                OrgFreedesktopFileManager1Interface interface(
                    "org.freedesktop.FileManager1"_l1,
                    "/org/freedesktop/FileManager1"_l1,
                    QDBusConnection::sessionBus()
                );
                interface.setTimeout(desktoputils::defaultDbusTimeout);
                const auto uris = toContainer<QStringList>(
                    filesToSelect | transform(&FilesInDirectory::files) | join | transform([](const QString& path) {
                        return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
                    })
                );
                info().log(
                    "FreedesktopFileManagerLauncher: executing org.freedesktop.FileManager1.ShowItems() D-Bus call "
                    "with: uris = {}",
                    uris
                );
                const auto reply = co_await interface.ShowItems(uris, {});
                if (!reply.isError()) {
                    info().log(
                        "FreedesktopFileManagerLauncher: executed org.freedesktop.FileManager1.ShowItems() D-Bus call"
                    );
                } else {
                    warning().log(
                        "FreedesktopFileManagerLauncher: org.freedesktop.FileManager1.ShowItems() D-Bus call failed: "
                        "{}",
                        reply.error()
                    );
                    for (const auto& [dirPath, dirFiles] : filesToSelect) {
                        fallbackForDirectory(dirPath, parentWidget.data());
                    }
                }
            }

            CoroutineScope mCoroutineScope{};
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() { return new FreedesktopFileManagerLauncher(); }
    }
}

#include "filemanagerlauncher_freedesktop.moc"
