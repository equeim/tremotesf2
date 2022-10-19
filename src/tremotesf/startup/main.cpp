// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QScopeGuard>
#include <QTranslator>

#include "libtremotesf/log.h"
#include "libtremotesf/target_os.h"
#include "tremotesf/ipc/ipcclient.h"
#include "tremotesf/ipc/ipcserver.h"
#include "tremotesf/rpc/servers.h"
#include "tremotesf/ui/screens/mainwindow/mainwindow.h"
#include "commandlineparser.h"
#include "main_windows.h"
#include "signalhandler.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QLocale)

using namespace tremotesf;

int main(int argc, char** argv) {
    // This does not need QApplication instance, and we need it in windowsInitPrelude()
    QCoreApplication::setOrganizationName(TREMOTESF_EXECUTABLE_NAME ""_l1);
    QCoreApplication::setApplicationName(QCoreApplication::organizationName());
    QCoreApplication::setApplicationVersion(TREMOTESF_VERSION ""_l1);

    if constexpr (isTargetOsWindows) { windowsInitPrelude(); }
    const auto preludeScopeGuard = QScopeGuard([] {
        if constexpr (isTargetOsWindows) { windowsDeinitPrelude(); }
    });

    // Setup handler for UNIX signals or Windows console handler
    signalhandler::initSignalHandler();
    const auto signalHandlerGuard = QScopeGuard([] { signalhandler::deinitSignalHandler(); });

    //
    // Command line parsing
    //
    CommandLineArgs args{};
    try {
        args = parseCommandLine(argc, argv);
        if (args.exit) { return EXIT_SUCCESS; }
    } catch (const std::runtime_error& e) {
        logWarning("Failed to parse command line arguments: {}", e.what());
        return EXIT_FAILURE;
    }

    // Send command to another instance
    if (const auto client = IpcClient::createInstance(); client->isConnected()) {
        logInfo("Only one instance of Tremotesf can be run at the same time");
        if (args.files.isEmpty() && args.urls.isEmpty()) {
            client->activateWindow();
        } else {
            client->addTorrents(args.files, args.urls);
        }
        return EXIT_SUCCESS;
    }

    if constexpr (isTargetOsWindows) { windowsInitWinrt(); }
    const auto winrtScopeGuard = QScopeGuard([] {
        if constexpr (isTargetOsWindows) { windowsDeinitWinrt(); }
    });

    //
    // QApplication initialization
    //
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    QGuiApplication::setQuitOnLastWindowClosed(false);

    if constexpr (isTargetOsWindows) { windowsInitApplication(); }

    QGuiApplication::setWindowIcon(QIcon::fromTheme(TREMOTESF_APP_ID ""_l1));
    //
    // End of QApplication initialization
    //

    auto ipcServer = IpcServer::createInstance(qApp);

    QTranslator qtTranslator;
    {
        const QString qtTranslationsPath = [] {
            if constexpr (isTargetOsWindows) {
                return QString::fromStdString(
                    fmt::format("{}/{}", QCoreApplication::applicationDirPath(), TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR)
                );
            } else {
                return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
            }
        }();
        if (qtTranslator.load(QLocale(), TREMOTESF_QT_TRANSLATIONS_FILENAME ""_l1, "_"_l1, qtTranslationsPath)) {
            qApp->installTranslator(&qtTranslator);
        } else {
            logWarning("Failed to load Qt translation for {} from {}", QLocale(), qtTranslationsPath);
        }
    }

    QTranslator appTranslator;
    appTranslator.load(QLocale().name(), ":/translations"_l1);
    qApp->installTranslator(&appTranslator);

    Servers::migrate();

    MainWindow window(std::move(args.files), std::move(args.urls), ipcServer);
    window.showMinimized(args.minimized);

    if (signalhandler::isExitRequested()) { return EXIT_SUCCESS; }

    return QCoreApplication::exec();
}
