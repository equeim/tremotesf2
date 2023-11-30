// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QLocale>
#include <QTranslator>

#include "commandlineparser.h"
#include "literals.h"
#include "main_windows.h"
#include "recoloringsvgiconengineplugin.h"
#include "signalhandler.h"
#include "target_os.h"
#include "ipc/ipcclient.h"
#include "log/log.h"
#include "ui/savewindowstatedispatcher.h"
#include "ui/screens/mainwindow/mainwindow.h"

#include <QStringBuilder>

SPECIALIZE_FORMATTER_FOR_QDEBUG(QLocale)

using namespace tremotesf;

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
        // Override QT_LOGGING_RULES env variable if command line option was specified
        QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, *args.enableDebugLogs);
    } else {
        // Disable by default but let QT_LOGGING_RULES override us
        QLoggingCategory::setFilterRules("default.debug=false"_l1);
    }
    if (QLoggingCategory::defaultCategory()->isDebugEnabled()) {
        logDebug("Debug logging is enabled");
    }

#ifdef Q_OS_WIN
    const WindowsLogger logger{};
#endif

    // Setup handler for UNIX signals or Windows console handler
    const SignalHandler signalHandler{};

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

    if constexpr (isTargetOsWindows) {
        windowsInitApplication();
    }
#if defined(TREMOTESF_BUNDLED_ICONS_DIR) && defined(TREMOTESF_BUNDLED_ICON_THEME)
    QIcon::setThemeSearchPaths({QCoreApplication::applicationDirPath() % '/' % TREMOTESF_BUNDLED_ICONS_DIR ""_l1});
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
#ifdef TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR
            QString::fromStdString(
                fmt::format("{}/{}", QCoreApplication::applicationDirPath(), TREMOTESF_BUNDLED_QT_TRANSLATIONS_DIR)
            );
#else
            QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
        if (qtTranslator.load(QLocale(), TREMOTESF_QT_TRANSLATIONS_FILENAME ""_l1, "_"_l1, qtTranslationsPath)) {
            qApp->installTranslator(&qtTranslator);
        } else {
            logWarning("Failed to load Qt translation for {} from {}", QLocale(), qtTranslationsPath);
        }
    }

    QTranslator appTranslator;
    if (!appTranslator.load(QLocale().name(), ":/translations"_l1)) {
        logWarning("Failed to load Tremotesf translation for {}", QLocale());
    }
    qApp->installTranslator(&appTranslator);

    const SaveWindowStateDispatcher saveStateDispatcher{};

    MainWindow window(std::move(args.files), std::move(args.urls));
    window.showMinimized(args.minimized);

    if (signalHandler.isExitRequested()) {
        return EXIT_SUCCESS;
    }

    const int exitStatus = QCoreApplication::exec();
    logDebug("Returning from main with exit status {}", exitStatus);
    return exitStatus;
}
