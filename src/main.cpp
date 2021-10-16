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

#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#ifdef TREMOTESF_SAILFISHOS
#include <memory>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <sailfishapp.h>
#else
#include <QApplication>
#include <QIcon>
#ifdef Q_OS_WIN
#include <windows.h>
#endif // Q_OS_WIN
#endif // TREMOTESF_SAILFISHOS

#include "commandlineparser.h"
#include "ipcclient.h"
#include "ipcserver.h"
#include "servers.h"
#include "signalhandler.h"
#include "utils.h"

#ifndef TREMOTESF_SAILFISHOS
#include "desktop/mainwindow.h"
#endif

int main(int argc, char** argv)
{
    // Setup handler for UNIX signals or Windows console handler
    tremotesf::SignalHandler::setupHandlers();

    //
    // Command line parsing
    //
    const tremotesf::CommandLineArgs args(tremotesf::parseCommandLine(argc, argv));
    if (args.exit) {
        return args.returnCode;
    }

    // Send command to another instance
    if (const auto client = tremotesf::IpcClient::createInstance(); client->isConnected()) {
        qInfo("Only one instance of Tremotesf can be run at the same time");
        if (args.files.isEmpty() && args.urls.isEmpty()) {
            client->activateWindow();
        } else {
            client->addTorrents(args.files, args.urls);
        }
        return 0;
    }

    //
    // Q(Gui)Application initialization
    //
#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif

#ifdef TREMOTESF_SAILFISHOS
    std::unique_ptr<QGuiApplication> app(SailfishApp::application(argc, argv));
    std::unique_ptr<QQuickView> view(SailfishApp::createView());
#else
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
#endif
    QCoreApplication::setApplicationVersion(QLatin1String(TREMOTESF_VERSION));
    //
    // End of QApplication initialization
    //

    // Setup socket notifier for UNIX signals
    tremotesf::SignalHandler::setupNotifier();

#ifndef TREMOTESF_SAILFISHOS
    qApp->setOrganizationName(qApp->applicationName());
    qApp->setWindowIcon(QIcon::fromTheme(QLatin1String(TREMOTESF_APP_ID)));
    qApp->setQuitOnLastWindowClosed(false);
#ifdef Q_OS_WIN
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    QIcon::setThemeSearchPaths({QLatin1String("icons")});
    QIcon::setThemeName(QLatin1String("breeze"));
#endif
#endif

    auto ipcServer = tremotesf::IpcServer::createInstance(qApp);

    QTranslator qtTranslator;
    qtTranslator.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    qApp->installTranslator(&qtTranslator);

    QTranslator appTranslator;
    appTranslator.load(QLocale().name(), QLatin1String(":/translations"));
    qApp->installTranslator(&appTranslator);

    tremotesf::Utils::registerTypes();

    tremotesf::Servers::migrate();

    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }

#ifdef TREMOTESF_SAILFISHOS
    view->rootContext()->setContextProperty(QLatin1String("ipcServer"), ipcServer);

    view->rootContext()->setContextProperty(QLatin1String("files"), args.files);
    view->rootContext()->setContextProperty(QLatin1String("urls"), args.urls);

    view->setSource(QUrl(QLatin1String("qrc:///main.qml")));
    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }
    view->show();
#else
    tremotesf::MainWindow window(ipcServer, args.files, args.urls);
    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }
    window.showMinimized(args.minimized);
#endif

    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }

    const int exitCode = qApp->exec();
#ifdef Q_OS_WIN
    CoUninitialize();
#endif
    return exitCode;
}
