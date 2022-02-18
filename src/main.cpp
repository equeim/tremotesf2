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

#include <csignal>
#include <iostream>

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QStringBuilder>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif // Q_OS_WIN

#include "commandlineparser.h"
#include "ipcclient.h"
#include "ipcserver.h"
#include "servers.h"
#include "signalhandler.h"
#include "utils.h"

#include "desktop/mainwindow.h"

#ifdef Q_OS_WIN
namespace {
    void on_terminate() {
        const auto exception_ptr = std::current_exception();
        if (exception_ptr) {
            try {
                std::rethrow_exception(exception_ptr);
            } catch (const std::exception& e) {
                qWarning() << "Unhandled exception:" << e.what();
            }
        }
    }
}
#endif

int main(int argc, char** argv)
{
#ifdef Q_OS_WIN
    std::set_terminate(on_terminate);
    try {
        tremotesf::Utils::callWinApiFunctionWithLastError([] { return SetConsoleOutputCP(GetACP()); });
    } catch (const std::exception& e) {
        qWarning() << "SetConsoleOutputCP failed:" << e.what();
        return EXIT_FAILURE;
    }
#endif

    // Setup handler for UNIX signals or Windows console handler
    try {
        tremotesf::SignalHandler::setupHandlers();
    } catch (const std::exception& e) {
        qWarning() << e.what();
        return EXIT_FAILURE;
    }

    //
    // Command line parsing
    //
    tremotesf::CommandLineArgs args{};
    try {
        args = tremotesf::parseCommandLine(argc, argv);
        if (args.exit) {
            return EXIT_SUCCESS;
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to parse command line arguments:" << e.what();
        return EXIT_FAILURE;
    }

    // Send command to another instance
    if (const auto client = tremotesf::IpcClient::createInstance(); client->isConnected()) {
        qInfo("Only one instance of Tremotesf can be run at the same time");
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
#ifdef Q_OS_WIN
    try {
        tremotesf::Utils::callWinApiFunctionWithLastError([] { return AllowSetForegroundWindow(ASFW_ANY); });
    } catch (const std::exception& e) {
        qWarning() << "AllowSetForegroundWindow failed:" << e.what();
        return EXIT_FAILURE;
    }
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(TREMOTESF_VERSION));
    //
    // End of QApplication initialization
    //

    // Setup socket notifier for UNIX signals
    tremotesf::SignalHandler::setupNotifier();

    QCoreApplication::setOrganizationName(qApp->applicationName());
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QLatin1String(TREMOTESF_APP_ID)));
    QGuiApplication::setQuitOnLastWindowClosed(false);
#ifdef Q_OS_WIN
    if (const auto ret = CoInitializeEx(nullptr, COINIT_MULTITHREADED); ret != S_OK && ret != S_FALSE && ret != RPC_E_CHANGED_MODE ) {
        qWarning() << "CoInitializeEx failed with error code" << ret;
        return EXIT_FAILURE;
    }

    QIcon::setThemeSearchPaths({QCoreApplication::applicationDirPath() % QLatin1Char('/') % QLatin1String(TREMOTESF_BUNDLED_ICONS_DIR)});
    QIcon::setThemeName(QLatin1String(TREMOTESF_BUNDLED_ICON_THEME));
#endif

    auto ipcServer = tremotesf::IpcServer::createInstance(qApp);

    QTranslator qtTranslator;
    {
#ifdef Q_OS_WIN
        const QString qtTranslationsPath = QCoreApplication::applicationDirPath() % QLatin1Char('/') % QLatin1String(TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR);
#else
        const auto qtTranslationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
        if (qtTranslator.load(QLocale(), QLatin1String(TREMOTESF_QT_TRANSLATIONS_FILENAME), QLatin1String("_"), qtTranslationsPath)) {
            qApp->installTranslator(&qtTranslator);
        } else {
            qWarning() << "Failed to load Qt translation for" << QLocale() << "from" << qtTranslationsPath;
        }
    }

    QTranslator appTranslator;
    appTranslator.load(QLocale().name(), QLatin1String(":/translations"));
    qApp->installTranslator(&appTranslator);

    tremotesf::Servers::migrate();

    if (tremotesf::SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    tremotesf::MainWindow window(ipcServer, args.files, args.urls);
    if (tremotesf::SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }
    window.showMinimized(args.minimized);

    if (tremotesf::SignalHandler::exitRequested) {
        return EXIT_SUCCESS;
    }

    const int exitCode = qApp->exec();
#ifdef Q_OS_WIN
    CoUninitialize();
#endif
    return exitCode;
}
