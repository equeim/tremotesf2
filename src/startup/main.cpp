// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <utility>

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

#include <fmt/ranges.h>

#include "commandlineparser.h"
#include "fileutils.h"
#include "literals.h"
#include "main_windows.h"
#include "recoloringsvgiconengineplugin.h"
#include "signalhandler.h"
#include "target_os.h"
#include "ipc/ipcclient.h"
#include "log/log.h"
#include "ui/savewindowstatedispatcher.h"
#include "ui/screens/mainwindow/mainwindow.h"

#ifdef Q_OS_MACOS
#    include "ipc/fileopeneventhandler.h"
#endif

SPECIALIZE_FORMATTER_FOR_QDEBUG(QLocale)

using namespace std::chrono_literals;
using namespace tremotesf;

namespace {
#ifdef Q_OS_MACOS
    std::pair<QStringList, QStringList> receiveFileOpenEvents(int& argc, char** argv) {
        std::pair<QStringList, QStringList> filesAndUrls{};
        logInfo("Waiting for file open events");
        const QGuiApplication app(argc, argv);
        const FileOpenEventHandler handler{};
        QObject::connect(
            &handler,
            &FileOpenEventHandler::filesOpeningRequested,
            &app,
            [&](const auto& files, const auto& urls) {
                filesAndUrls = {files, urls};
                QCoreApplication::quit();
            }
        );
        QTimer::singleShot(500ms, &app, [] {
            logInfo("Did not receive file open events");
            QCoreApplication::quit();
        });
        QCoreApplication::exec();
        return filesAndUrls;
    }
#endif

    bool shouldExitBecauseAnotherInstanceIsRunning(
        [[maybe_unused]] int& argc, [[maybe_unused]] char** argv, const CommandLineArgs& args
    ) {
        const auto client = IpcClient::createInstance();
        if (!client->isConnected()) {
            return false;
        }
        logInfo("Only one instance of Tremotesf can be run at the same time");
        const auto activateOtherInstance = [&client](const QStringList& files, const QStringList& urls) {
            if (files.isEmpty() && urls.isEmpty()) {
                logInfo("Activating other instance");
                client->activateWindow();
            } else {
                logInfo("Activating other instance and requesting torrent adding");
                logInfo("files = {}", files);
                logInfo("urls = {}", urls);
                client->addTorrents(files, urls);
            }
        };
#ifdef Q_OS_MACOS
        if (args.files.isEmpty() && args.urls.isEmpty()) {
            const auto [files, urls] = receiveFileOpenEvents(argc, argv);
            activateOtherInstance(files, urls);
            return true;
        }
#endif
        activateOtherInstance(args.files, args.urls);
        return true;
    }
}

int main(int argc, char** argv) {
    // This does not need QApplication instance, and we need it in windowsInitPrelude()
    QCoreApplication::setOrganizationName(TREMOTESF_EXECUTABLE_NAME ""_l1);
    QCoreApplication::setApplicationName(QCoreApplication::organizationName());
    QCoreApplication::setApplicationVersion(TREMOTESF_VERSION ""_l1);

    //
    // Command line parsing
    //
    CommandLineArgs args{};
    try {
        args = parseCommandLine(argc, argv);
        if (args.exit) {
            return EXIT_SUCCESS;
        }
    } catch (const std::runtime_error& e) {
        logWarning("Failed to parse command line arguments: {}", e.what());
        return EXIT_FAILURE;
    }

    if (args.enableDebugLogs.has_value()) {
        overrideDebugLogs(*args.enableDebugLogs);
    }
    if (tremotesfLoggingCategory().isDebugEnabled()) {
        logDebug("Debug logging is enabled");
    }

#ifdef Q_OS_WIN
    const WindowsLogger logger{};
#endif

    // Setup handler for UNIX signals or Windows console handler
    const SignalHandler signalHandler{};

    if (shouldExitBecauseAnotherInstanceIsRunning(argc, argv, args)) {
        return EXIT_SUCCESS;
    }

#ifdef Q_OS_WIN
    const WinrtApartment apartment{};
#endif

    //
    // QApplication initialization
    //
#if QT_VERSION_MAJOR < 6
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    const QApplication app(argc, argv);
    QGuiApplication::setQuitOnLastWindowClosed(false);

    if constexpr (targetOs == TargetOs::Windows) {
        windowsInitApplication();
    }
#if defined(TREMOTESF_BUNDLED_ICON_THEME)
    QIcon::setThemeSearchPaths({resolveExternalBundledResourcesPath("icons"_l1)});
    QIcon::setThemeName(TREMOTESF_BUNDLED_ICON_THEME ""_l1);
    QApplication::setStyle(new RecoloringSvgIconStyle(qApp));
#endif

    QGuiApplication::setDesktopFileName(TREMOTESF_APP_ID ""_l1);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(TREMOTESF_APP_ID ""_l1));
    //
    // End of QApplication initialization
    //

    QTranslator qtTranslator;
    {
        const QString qtTranslationsPath =
#ifdef TREMOTESF_USE_BUNDLED_QT_TRANSLATIONS
            resolveExternalBundledResourcesPath("qt-translations"_l1);
#else
            QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
        if (qtTranslator.load(QLocale{}, "qt"_l1, "_"_l1, qtTranslationsPath)) {
            qApp->installTranslator(&qtTranslator);
        } else {
            logWarning("Failed to load Qt translation for {} from {}", QLocale(), qtTranslationsPath);
        }
    }

    QTranslator appTranslator;
    if (appTranslator.load(QLocale{}, {}, {}, ":/translations/"_l1)) {
        qApp->installTranslator(&appTranslator);
    } else {
        logWarning("Failed to load Tremotesf translation for {}", QLocale{});
    }

    const SaveWindowStateDispatcher saveStateDispatcher{};

    MainWindow window(std::move(args.files), std::move(args.urls));
    window.initialShow(args.minimized);

    if (signalHandler.isExitRequested()) {
        return EXIT_SUCCESS;
    }

    const int exitStatus = QCoreApplication::exec();
    logDebug("Returning from main with exit status {}", exitStatus);
    return exitStatus;
}
