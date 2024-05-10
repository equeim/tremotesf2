// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <algorithm>

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QUrl>
#include <QWidget>

#include "coroutines/dbus.h"
#include "coroutines/scope.h"
#include "desktoputils.h"
#include "literals.h"
#include "log/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.FileManager1.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

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
                info().log(
                    "FreedesktopFileManagerLauncher: executing org.freedesktop.FileManager1.ShowItems() D-Bus call"
                );
                OrgFreedesktopFileManager1Interface interface(
                    "org.freedesktop.FileManager1"_l1,
                    "/org/freedesktop/FileManager1"_l1,
                    QDBusConnection::sessionBus()
                );
                interface.setTimeout(desktoputils::defaultDbusTimeout);
                QStringList uris{};
                uris.reserve(std::accumulate(
                    filesToSelect.begin(),
                    filesToSelect.end(),
                    0,
                    [](size_t count, const FilesInDirectory& f) { return count + f.files.size(); }
                ));
                for (const auto& [_, files] : filesToSelect) {
                    std::ranges::copy(files, std::back_insert_iterator(uris));
                }

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
