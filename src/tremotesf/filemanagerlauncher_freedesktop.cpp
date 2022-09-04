#include "filemanagerlauncher.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QUrl>

#include "libtremotesf/log.h"
#include "tremotesf/desktoputils.h"
#include "tremotesf_dbus_generated/org.freedesktop.FileManager1.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

namespace tremotesf {
    namespace {
        class FreedesktopFileManagerLauncher : public impl::FileManagerLauncher {
            Q_OBJECT

        protected:
            void launchFileManagerAndSelectFiles(const std::vector<std::pair<QString, std::vector<QString>>>& directories, QPointer<QWidget> parentWidget) override {
                logInfo("FreedesktopFileManagerLauncher: executing org.freedesktop.FileManager1.ShowItems() D-Bus call");
                OrgFreedesktopFileManager1Interface interface(QLatin1String("org.freedesktop.FileManager1"),
                    QLatin1String("/org/freedesktop/FileManager1"),
                    QDBusConnection::sessionBus()
                );
                interface.setTimeout(desktoputils::defaultDbusTimeout);
                QStringList uris{};
                for (const auto& [_, dirFiles] : directories) {
                    for (const QString& filePath : dirFiles) {
                        uris.push_back(QUrl::fromLocalFile(filePath).toString());
                    }
                }
                const auto pendingReply = interface.ShowItems(uris, {});
                const auto onFinished = [=] {
                    if (!pendingReply.isError()) {
                        logInfo("FreedesktopFileManagerLauncher: executed org.freedesktop.FileManager1.ShowItems() D-Bus call");
                    } else {
                        logWarning("FreedesktopFileManagerLauncher: org.freedesktop.FileManager1.ShowItems() D-Bus call failed: {}", pendingReply.error());
                        for (const auto& [dirPath, dirFiles] : directories) {
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
                QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QDBusPendingCallWatcher::deleteLater);
            }
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() {
            return new FreedesktopFileManagerLauncher();
        }
    }
}

#include "filemanagerlauncher_freedesktop.moc"
