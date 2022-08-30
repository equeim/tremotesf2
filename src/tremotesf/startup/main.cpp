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

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
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

int main(int argc, char** argv)
{
    if constexpr (isTargetOsWindows) {
        windowsInitPreApplication();
    }

    // Setup handler for UNIX signals or Windows console handler
    try {
        SignalHandler::setupHandlers();
    } catch (const std::exception& e) {
        logWarning("Failed to setup signal handlers: {}", e.what());
    }

    //
    // Command line parsing
    //
    CommandLineArgs args{};
    try {
        args = parseCommandLine(argc, argv);
        if (args.exit) {
            return EXIT_SUCCESS;
        }
    } catch (const std::exception& e) {
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

    //
    // QApplication initialization
    //
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(TREMOTESF_VERSION));
    QCoreApplication::setOrganizationName(qApp->applicationName());
    QGuiApplication::setQuitOnLastWindowClosed(false);

    if constexpr (isTargetOsWindows) {
        windowsInitPostApplication();
    }

    QGuiApplication::setWindowIcon(QIcon::fromTheme(QLatin1String(TREMOTESF_APP_ID)));
    //
    // End of QApplication initialization
    //

    // Setup socket notifier for UNIX signals
    SignalHandler::setupNotifier();

    auto ipcServer = IpcServer::createInstance(qApp);

    QTranslator qtTranslator;
    {
        const QString qtTranslationsPath = [] {
            if constexpr (isTargetOsWindows) {
                return QString::fromStdString(fmt::format("{}/{}", QCoreApplication::applicationDirPath(), TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR));
            } else {
                return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
            }
        }();
        if (qtTranslator.load(QLocale(), QLatin1String(TREMOTESF_QT_TRANSLATIONS_FILENAME), QLatin1String("_"), qtTranslationsPath)) {
            qApp->installTranslator(&qtTranslator);
        } else {
            logWarning("Failed to load Qt translation for {} from {}", QLocale(), qtTranslationsPath);
        }
    }

    QTranslator appTranslator;
    appTranslator.load(QLocale().name(), QLatin1String(":/translations"));
    qApp->installTranslator(&appTranslator);

    Servers::migrate();

    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    MainWindow window(ipcServer, args.files, args.urls);
    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }
    window.showMinimized(args.minimized);

    if (SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    const int exitCode = qApp->exec();
    if constexpr (isTargetOsWindows) {
        windowsDeinit();
    }
    return exitCode;
}
