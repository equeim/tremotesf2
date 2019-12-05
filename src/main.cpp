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

#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "ipcserver.h"
#include "servers.h"
#include "signalhandler.h"
#include "utils.h"

#ifdef TREMOTESF_SAILFISHOS
#include <memory>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <sailfishapp.h>
#else
#include <QApplication>
#include <QIcon>
#include "desktop/mainwindow.h"
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char** argv)
{
    // Setup handler for UNIX signals or Windows console handler
    tremotesf::SignalHandler::setupHandlers();

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

    //
    // Command line parsing
    //
    QCommandLineParser parser;
#ifdef TREMOTESF_SAILFISHOS
    parser.addPositionalArgument(QLatin1String("torrent"), QLatin1String("Torrent file or URL"));
#else
    parser.addPositionalArgument(QLatin1String("torrents"), QLatin1String("Torrent files or URLs"));
    parser.addOption(QCommandLineOption(QLatin1String("minimized"), QLatin1String("Start minimized in notification area")));
#endif
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(qApp->arguments());
    const QStringList arguments(parser.positionalArguments());
    //
    // End of command line parsing
    //

    // Send command to another instance
    if (tremotesf::IpcServer::tryToConnect()) {
        qWarning() << "Only one instance of Tremotesf can be run at the same time";
        if (arguments.isEmpty()) {
            tremotesf::IpcServer::activateWindow();
        } else {
            tremotesf::IpcServer::sendArguments(arguments);
        }
        return 0;
    }

#ifndef TREMOTESF_SAILFISHOS
    qApp->setOrganizationName(qApp->applicationName());
    qApp->setWindowIcon(QIcon::fromTheme(QLatin1String("org.equeim.Tremotesf")));
    qApp->setQuitOnLastWindowClosed(false);
#ifdef Q_OS_WIN
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    QIcon::setThemeSearchPaths({QLatin1String("icons")});
    QIcon::setThemeName(QLatin1String("breeze"));
#endif
#endif

    tremotesf::IpcServer ipcServer;

    QTranslator qtTranslator;
    qtTranslator.load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    qApp->installTranslator(&qtTranslator);

    QTranslator appTranslator;
#if defined(Q_OS_WIN) && !defined(TEST_BUILD)
    appTranslator.load(QLocale().name(), QString::fromLatin1("%1/translations").arg(qApp->applicationDirPath()));
#else
    appTranslator.load(QLocale().name(), QLatin1String(TRANSLATIONS_PATH));
#endif
    qApp->installTranslator(&appTranslator);

    tremotesf::Utils::registerTypes();

    tremotesf::Servers::migrate();

    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }

#ifdef TREMOTESF_SAILFISHOS
    view->rootContext()->setContextProperty(QLatin1String("ipcServer"), &ipcServer);
    view->rootContext()->setContextProperty(QLatin1String("ipcServerServiceName"), tremotesf::IpcServer::serviceName);
    view->rootContext()->setContextProperty(QLatin1String("ipcServerObjectPath"), tremotesf::IpcServer::objectPath);
    view->rootContext()->setContextProperty(QLatin1String("ipcServerInterfaceName"), tremotesf::IpcServer::interfaceName);

    tremotesf::ArgumentsParseResult result(tremotesf::IpcServer::parseArguments(arguments));
    view->rootContext()->setContextProperty(QLatin1String("files"), result.files);
    view->rootContext()->setContextProperty(QLatin1String("urls"), result.urls);

    view->setSource(SailfishApp::pathTo(QLatin1String("qml/main.qml")));
    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }
    view->show();
#else
    tremotesf::MainWindow window(&ipcServer, arguments);
    if (tremotesf::SignalHandler::exitRequested) {
        return 0;
    }
    window.showMinimized(parser.isSet(QLatin1String("minimized")));
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
