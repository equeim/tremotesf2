// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <ranges>
#include <fmt/ranges.h>

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QWidget>

#include <KWaylandExtras>
#include <KWindowSystem>

#include "coroutines/dbus.h"
#include "coroutines/scope.h"
#include "log/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.FileManager1.h"
#include "desktoputils.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

using namespace std::views;
using std::ranges::to;
using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        class ActivationTokenAwaitable final {
        public:
            inline explicit ActivationTokenAwaitable(QWindow* window) : mWindow(window) {}
            ~ActivationTokenAwaitable() = default;
            Q_DISABLE_COPY_MOVE(ActivationTokenAwaitable)

            inline bool await_ready() { return false; }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (!startAwaiting(handle)) {
                    return;
                }
#if KWINDOWSYSTEM_VERSION >= QT_VERSION_CHECK(6, 19, 0)
                KWaylandExtras::xdgActivationToken(mWindow, {}).then(&mReceiver, [=, this](QString token) {
                    mToken = std::move(token);
                    resume(handle);
                });
#else
                const auto requestedSerial = KWaylandExtras::lastInputSerial(mWindow);
                QObject::connect(
                    KWaylandExtras::self(),
                    &KWaylandExtras::xdgActivationTokenArrived,
                    &mReceiver,
                    [=, this](int serial, const QString& token) {
                        if (static_cast<quint32>(serial) == requestedSerial) {
                            mToken = token;
                            resume(handle);
                        }
                    }
                );
                KWaylandExtras::requestXdgActivationToken(mWindow, requestedSerial, {});
#endif
            }

            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            inline QString await_resume() { return std::move(mToken).value(); };

        private:
            QWindow* mWindow;
            QObject mReceiver{};
            std::optional<QString> mToken{};
        };

        class FreedesktopFileManagerLauncher final : public impl::FileManagerLauncher {
            Q_OBJECT

        public:
            FreedesktopFileManagerLauncher() = default;

        protected:
            void launchFileManagerAndSelectFiles(
                std::vector<FilesInDirectory> filesToSelect, QWidget* parentWidget
            ) override {
                mCoroutineScope.launch(launchFileManagerAndSelectFilesImpl(std::move(filesToSelect), parentWidget));
            }

        private:
            Coroutine<> launchFileManagerAndSelectFilesImpl(
                std::vector<FilesInDirectory> filesToSelect, QPointer<QWidget> parentWidget
            ) {
                const auto uris = filesToSelect
                                  | transform(&FilesInDirectory::files)
                                  | join
                                  | transform([](const QString& path) {
                                        return QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
                                    })
                                  | to<QStringList>();

                QString startupId{};
                if (KWindowSystem::isPlatformWayland()) {
                    info().log("FreedesktopFileManagerLauncher: requesting XDG activation token");
                    const auto window = parentWidget.data() ? parentWidget->windowHandle() : nullptr;
                    if (window) {
                        startupId = co_await ActivationTokenAwaitable(window);
                        info().log("FreedesktopFileManagerLauncher: received XDG activation token '{}'", startupId);
                    } else {
                        warning().log(
                            "FreedesktopFileManagerLauncher: platform window is null, can't request XDG activation "
                            "token"
                        );
                    }
                }

                info().log(
                    "FreedesktopFileManagerLauncher: executing org.freedesktop.FileManager1.ShowItems() D-Bus call "
                    "with: uris = {}, startupId = {}",
                    uris,
                    startupId
                );

                OrgFreedesktopFileManager1Interface interface(
                    "org.freedesktop.FileManager1"_L1,
                    "/org/freedesktop/FileManager1"_L1,
                    QDBusConnection::sessionBus()
                );
                interface.setTimeout(desktoputils::defaultDbusTimeout);
                const auto reply = co_await interface.ShowItems(uris, startupId);
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
