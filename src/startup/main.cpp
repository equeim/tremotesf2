// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <utility>

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QLocale>
#include <QTranslator>

#include <fmt/ranges.h>

#include "commandlineparser.h"
#include "signalhandler.h"
#include "ipc/ipcclient.h"
#include "log/log.h"
#include "ui/iconthemesetup.h"
#include "ui/savewindowstatedispatcher.h"
#include "ui/screens/mainwindow/mainwindow.h"

#ifdef Q_OS_WIN
#    include "main_windows.h"
#    include "windowsfatalerrorhandlers.h"
#endif

#ifdef Q_OS_MACOS
#    include <chrono>
#    include <QTimer>
#    include "ipc/fileopeneventhandler.h"
using namespace std::chrono_literals;
#endif

#ifdef TREMOTESF_USE_BUNDLED_QT_TRANSLATIONS
#    include "fileutils.h"
#endif

SPECIALIZE_FORMATTER_FOR_QDEBUG(QLocale)

using namespace Qt::StringLiterals;
using namespace tremotesf;

namespace {
#ifdef Q_OS_MACOS
    std::pair<QStringList, QStringList> receiveFileOpenEvents() {
        std::pair<QStringList, QStringList> filesAndUrls{};
        info().log("Waiting for file open events");
        const FileOpenEventHandler handler{};
        QObject::connect(
            &handler,
            &FileOpenEventHandler::filesOpeningRequested,
            qApp,
            [&](const auto& files, const auto& urls) {
                filesAndUrls = {files, urls};
                QCoreApplication::quit();
            }
        );
        QTimer::singleShot(500ms, qApp, [] {
            info().log("Did not receive file open events");
            QCoreApplication::quit();
        });
        QCoreApplication::exec();
        return filesAndUrls;
    }
#endif

    bool shouldExitBecauseAnotherInstanceIsRunning(const CommandLineArgs& args) {
        const auto client = IpcClient::createInstance();
        if (!client->isConnected()) {
            return false;
        }
        info().log("Only one instance of Tremotesf can be run at the same time");
        const auto activateOtherInstance = [&client](const QStringList& files, const QStringList& urls) {
            if (files.isEmpty() && urls.isEmpty()) {
                info().log("Activating other instance");
                client->activateWindow();
            } else {
                info().log("Activating other instance and requesting torrent adding");
                info().log("files = {}", files);
                info().log("urls = {}", urls);
                client->addTorrents(files, urls);
            }
        };
#ifdef Q_OS_MACOS
        if (args.files.isEmpty() && args.urls.isEmpty()) {
            const auto [files, urls] = receiveFileOpenEvents();
            activateOtherInstance(files, urls);
            return true;
        }
#endif
        activateOtherInstance(args.files, args.urls);
        return true;
    }

    void logLocaleInfo() {
        if (!tremotesfLoggingCategory().isDebugEnabled()) {
            return;
        }
        QLocale locale{};
        debug().log("Current locale is: {}", locale.name());
        debug().log("Language: {}", locale.language());
        debug().log("Script: {}", locale.script());
        debug().log("Territory: {}", locale.territory());
        debug().log("UI languages: {}", locale.uiLanguages());
    }
}

int main(int argc, char** argv) {
    // This does not need QApplication instance, and we need it in windowsInitPrelude()
    QCoreApplication::setOrganizationName(TREMOTESF_EXECUTABLE_NAME ""_L1);
    QCoreApplication::setApplicationName(QCoreApplication::organizationName());
    QCoreApplication::setApplicationVersion(TREMOTESF_VERSION ""_L1);

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
        warning().log("Failed to parse command line arguments: {}", e.what());
        return EXIT_FAILURE;
    }

    if (args.enableDebugLogs.has_value()) {
        overrideDebugLogs(*args.enableDebugLogs);
    }

#ifdef Q_OS_WIN
    windowsSetUpFatalErrorHandlers();
    const WindowsLogger logger{};
#endif

    if (tremotesfLoggingCategory().isDebugEnabled()) {
        debug().log("Debug logging is enabled");
    }

    // Setup handler for UNIX signals or Windows console handler
    const SignalHandler signalHandler{};

#ifdef Q_OS_WIN
    const WinrtApartment apartment{};
#endif

    //
    // QApplication initialization
    //
    const QApplication app(argc, argv);

    if (shouldExitBecauseAnotherInstanceIsRunning(args)) {
        return EXIT_SUCCESS;
    }

    QGuiApplication::setQuitOnLastWindowClosed(false);

#ifdef Q_OS_WIN
    windowsInitApplication();
#endif

    setupIconTheme();

    QGuiApplication::setDesktopFileName(TREMOTESF_APP_ID ""_L1);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(TREMOTESF_APP_ID ""_L1));
    //
    // End of QApplication initialization
    //

    logLocaleInfo();

    QTranslator qtTranslator;
    {
        const QString qtTranslationsPath =
#ifdef TREMOTESF_USE_BUNDLED_QT_TRANSLATIONS
            resolveExternalBundledResourcesPath("qt-translations"_L1);
#else
            QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#endif
        if (qtTranslator.load(QLocale{}, "qt"_L1, "_"_L1, qtTranslationsPath)) {
            info().log("Loaded Qt translation {}", qtTranslator.filePath());
            qApp->installTranslator(&qtTranslator);
        } else {
            warning().log("Failed to load Qt translation for {} from {}", QLocale(), qtTranslationsPath);
        }
    }

    QTranslator appTranslator;
    if (appTranslator.load(QLocale{}, {}, {}, ":/translations/"_L1)) {
        info().log("Loaded Tremotesf translation {}", appTranslator.filePath());
        qApp->installTranslator(&appTranslator);
    } else {
        warning().log("Failed to load Tremotesf translation for {}", QLocale{});
    }

    const SaveWindowStateDispatcher saveStateDispatcher{};

    MainWindow window(std::move(args.files), std::move(args.urls));
    window.initialShow(args.minimized);

    if (signalHandler.isExitRequested()) {
        return EXIT_SUCCESS;
    }

    const int exitStatus = QCoreApplication::exec();
    debug().log("Returning from main with exit status {}", exitStatus);
    return exitStatus;
}
