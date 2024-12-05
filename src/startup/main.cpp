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
#include "literals.h"
#include "signalhandler.h"
#include "target_os.h"
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

using namespace tremotesf;

namespace {
#ifdef Q_OS_MACOS
    std::pair<QStringList, QStringList> receiveFileOpenEvents(int& argc, char** argv) {
        std::pair<QStringList, QStringList> filesAndUrls{};
        info().log("Waiting for file open events");
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
            info().log("Did not receive file open events");
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
            const auto [files, urls] = receiveFileOpenEvents(argc, argv);
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
#if QT_VERSION_MAJOR >= 6
        debug().log("Territory: {}", locale.territory());
#endif
        debug().log("UI languages: {}", locale.uiLanguages());
    }

    bool
    loadTranslation(QTranslator& translator, const QString& filename, const QString& prefix, const QString& directory) {
        // https://bugreports.qt.io/browse/QTBUG-129434
        static const bool applyWorkaround = [] {
            const bool apply = (QLibraryInfo::version() == QVersionNumber(6, 7, 3));
            debug().log("Applying QTranslator workaround for Qt 6.7.3");
            return apply;
        }();
        QLocale locale{};
        if (applyWorkaround) {
            QString actualFilename = filename + prefix + locale.name();
            return translator.load(actualFilename, directory);
        } else {
            return translator.load(locale, filename, prefix, directory);
        }
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

    // Workaround for application quitting when creating QFileDialog in KDE
    // https://bugs.kde.org/show_bug.cgi?id=471941
    // https://bugs.kde.org/show_bug.cgi?id=483439
    QCoreApplication::setQuitLockEnabled(false);

#ifdef Q_OS_WIN
    windowsInitApplication();
#endif

    setupIconTheme();

    QGuiApplication::setDesktopFileName(TREMOTESF_APP_ID ""_l1);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(TREMOTESF_APP_ID ""_l1));
    //
    // End of QApplication initialization
    //

    logLocaleInfo();

    QTranslator qtTranslator;
    {
        const QString qtTranslationsPath =
#ifdef TREMOTESF_USE_BUNDLED_QT_TRANSLATIONS
            resolveExternalBundledResourcesPath("qt-translations"_l1);
#else
#    if QT_VERSION_MAJOR >= 6
            QLibraryInfo::path(
#    else
            QLibraryInfo::location(
#    endif
                QLibraryInfo::TranslationsPath
            );
#endif
        if (loadTranslation(qtTranslator, "qt"_l1, "_"_l1, qtTranslationsPath)) {
            info().log("Loaded Qt translation {}", qtTranslator.filePath());
            qApp->installTranslator(&qtTranslator);
        } else {
            warning().log("Failed to load Qt translation for {} from {}", QLocale(), qtTranslationsPath);
        }
    }

    QTranslator appTranslator;
    if (loadTranslation(appTranslator, {}, {}, ":/translations/"_l1)) {
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
