// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktoputils.h"

#include <optional>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QStyle>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QUrl>

#ifdef TREMOTESF_UNIX_FREEDESKTOP
#    include <QPointer>
#    include <KWindowSystem>
#    include <fmt/chrono.h>
#    include "coroutines/coroutines.h"
#    include "coroutines/qobjectsignal.h"
#    include "coroutines/scope.h"
#    include "coroutines/timer.h"
#    include "coroutines/waitall.h"
#endif

#include <fmt/format.h>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

using namespace Qt::StringLiterals;

namespace tremotesf::desktoputils {
    const QIcon& statusIcon(StatusIcon icon) {
        using enum StatusIcon;
        switch (icon) {
        case Active: {
            static const QIcon qicon(":/active.svg"_L1);
            return qicon;
        }
        case Checking: {
            static const QIcon qicon(":/checking.svg"_L1);
            return qicon;
        }
        case Downloading: {
            static const QIcon qicon(":/downloading.svg"_L1);
            return qicon;
        }
        case Errored: {
            static const QIcon qicon(":/errored.svg"_L1);
            return qicon;
        }
        case Paused: {
            static const QIcon qicon(":/paused.svg"_L1);
            return qicon;
        }
        case Queued: {
            static const QIcon qicon(":/queued.svg"_L1);
            return qicon;
        }
        case Seeding: {
            static const QIcon qicon(":/seeding.svg"_L1);
            return qicon;
        }
        case StalledDownloading: {
            static const QIcon qicon(":/stalled-downloading.svg"_L1);
            return qicon;
        }

        case StalledSeeding: {
            static const QIcon qicon(":/stalled-seeding.svg"_L1);
            return qicon;
        }
        }

        throw std::logic_error(fmt::format("Unknown StatusIcon value {}", std::to_underlying(icon)));
    }

    const QIcon& priorityIcon(PriorityIcon icon) {
        switch (icon) {
        case HighPriorityIcon: {
            static const QIcon qicon(":/priority-high.svg"_L1);
            return qicon;
        }
        case NormalPriorityIcon: {
            static const QIcon qicon(":/priority-normal.svg"_L1);
            return qicon;
        }
        case LowPriorityIcon: {
            static const QIcon qicon(":/priority-low.svg"_L1);
            return qicon;
        }
        }
        throw std::logic_error(fmt::format("Unknown PriorityIcon value {}", static_cast<int>(icon)));
    }

    const QIcon& standardFileIcon() {
        static const auto icon = qApp->style()->standardIcon(QStyle::SP_FileIcon);
        return icon;
    }

    const QIcon& standardDirIcon() {
        static const auto icon = qApp->style()->standardIcon(QStyle::SP_DirIcon);
        return icon;
    }

    namespace {
        void showOpenFileError(const QString& filePath, std::optional<QString> error, QWidget* parent) {
            auto dialog = new QMessageBox(
                QMessageBox::Warning,
                //: Dialog title
                qApp->translate("tremotesf", "Error"),
                //: File opening error, %1 is a file path
                qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(filePath)),
                QMessageBox::Close,
                parent
            );
            if (error.has_value()) {
                dialog->setText(dialog->text() % "\n\n"_L1 % *error);
            }
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }

        void openFileImpl(const QString& filePath, QWidget* parent) {
            const auto url = QUrl::fromLocalFile(filePath);
            info().log("Executing QDesktopServices::openUrl() for {}", url);
            if (!QDesktopServices::openUrl(url)) {
                warning().log("QDesktopServices::openUrl() failed for {}", url);
                showOpenFileError(filePath, {}, parent);
            }
        }

#ifdef TREMOTESF_UNIX_FREEDESKTOP
        class DelayedUrlOpener : public QObject {
            Q_OBJECT
        public:
            using QObject::QObject;

            static DelayedUrlOpener* instance() {
                static const auto instance = new DelayedUrlOpener(qApp);
                return instance;
            }

            void openUrlAfterFocusWindowChange(QString filePath, QPointer<QWidget> parent) {
                mScope.launch(openUrlAfterFocusWindowChangeImpl(std::move(filePath), std::move(parent)));
            }

        private:
            Coroutine<> openUrlAfterFocusWindowChangeImpl(QString filePath, QPointer<QWidget> parent) {
                co_await waitAny(
                    []() -> Coroutine<> {
                        debug().log("Waiting for focusWindowChanged signal");
                        co_await waitForSignal(qApp, &QGuiApplication::focusWindowChanged);
                        debug().log("Received focusWindowChanged signal");
                    }(),
                    []() -> Coroutine<> {
                        using namespace std::chrono_literals;
                        constexpr auto timeout = 100ms;
                        co_await waitFor(timeout);
                        debug().log("Did not receive focusWindowChanged signal in {}", timeout);
                    }()
                );
                openFileImpl(filePath, parent);
            }

            CoroutineScope mScope;
        };
#endif
    }

    void openFile(const QString& filePath, QWidget* parent) {
        if (!QFile::exists(filePath)) {
            warning().log("Can't open file {}, it does not exist", filePath);
            showOpenFileError(filePath, qApp->translate("tremotesf", "This file/directory does not exist"), parent);
            return;
        }
#ifdef TREMOTESF_UNIX_FREEDESKTOP
        // If focusWindow returns null in this moment (which is possible when we've just closed a context menu) then Qt will not pass activation token to launched app:
        // https://bugreports.qt.io/browse/QTBUG-138892
        // Wait for QGuiApplication::focusWindowChanged signal first so that token is requested and passed along
        if (KWindowSystem::isPlatformWayland() && !QGuiApplication::focusWindow()) {
            DelayedUrlOpener::instance()->openUrlAfterFocusWindowChange(filePath, parent);
        } else {
            openFileImpl(filePath, parent);
        }
#else
        openFileImpl(filePath, parent);
#endif
    }

    namespace {
        QRegularExpression urlRegex() {
            constexpr auto protocol = "(?:(?:[a-z]+:)?//)"_L1;
            constexpr auto host = R"((?:(?:[a-z\x{00a1}-\x{ffff0}-9][-_]*)*[a-z\x{00a1}-\x{ffff0}-9]+))";
            constexpr auto domain = R"((?:\.(?:[a-z\x{00a1}-\x{ffff0}-9]-*)*[a-z\x{00a1}-\x{ffff0}-9]+)*)";
            constexpr auto tld = R"((?:\.(?:[a-z\x{00a1}-\x{ffff}]{2,}))\.?)";
            constexpr auto port = R"((?::\d{2,5})?)";
            constexpr auto path = R"((?:[/?#][^\s"\)']*)?)";
            const auto regex =
                QString("(?:"_L1 % protocol % R"(|www\.)(?:)" % host % domain % tld % ")"_L1 % port % path);
            return QRegularExpression(regex, QRegularExpression::CaseInsensitiveOption);
        }
    }

    void findLinksAndAddAnchors(QTextDocument* document) {
        auto baseFormat = QTextCharFormat();
        baseFormat.setAnchor(true);
        baseFormat.setFontUnderline(true);
        baseFormat.setForeground(qApp->palette().link());
        const auto regex = desktoputils::urlRegex();
        auto cursor = QTextCursor();
        while (true) {
            cursor = document->find(regex, cursor);
            if (cursor.isNull()) {
                break;
            }
            auto format = baseFormat;
            format.setAnchorHref(cursor.selection().toPlainText());
            cursor.setCharFormat(format);
        }
    }
}

#include "desktoputils.moc"
