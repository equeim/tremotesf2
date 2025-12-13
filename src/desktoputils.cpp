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

#include <fmt/format.h>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

using namespace Qt::StringLiterals;

namespace tremotesf::desktoputils {
    const QIcon& statusIcon(StatusIcon icon) {
        switch (icon) {
        case ActiveIcon: {
            static const QIcon qicon(":/active.svg"_L1);
            return qicon;
        }
        case CheckingIcon: {
            static const QIcon qicon(":/checking.svg"_L1);
            return qicon;
        }
        case DownloadingIcon: {
            static const QIcon qicon(":/downloading.svg"_L1);
            return qicon;
        }
        case ErroredIcon: {
            static const QIcon qicon(":/errored.svg"_L1);
            return qicon;
        }
        case PausedIcon: {
            static const QIcon qicon(":/paused.svg"_L1);
            return qicon;
        }
        case QueuedIcon: {
            static const QIcon qicon(":/queued.svg"_L1);
            return qicon;
        }
        case SeedingIcon: {
            static const QIcon qicon(":/seeding.svg"_L1);
            return qicon;
        }
        case StalledDownloadingIcon: {
            static const QIcon qicon(":/stalled-downloading.svg"_L1);
            return qicon;
        }

        case StalledSeedingIcon: {
            static QIcon qicon(":/stalled-seeding.svg"_L1);
            return qicon;
        }
        }

        throw std::logic_error(fmt::format("Unknown StatusIcon value {}", std::to_underlying(icon)));
    }

    const QIcon& priorityIcon(Priority priority) {
        switch (priority) {
        case Priority::High: {
            static const QIcon qicon(":/priority-high.svg"_L1);
            return qicon;
        }
        case Priority::Normal: {
            static const QIcon qicon(":/priority-normal.svg"_L1);
            return qicon;
        }
        case Priority::Low: {
            static const QIcon qicon(":/priority-low.svg"_L1);
            return qicon;
        }
        }
        throw std::logic_error(fmt::format("Unknown Priority value {}", std::to_underlying(priority)));
    }

    const QIcon& standardFileIcon() {
        static const auto icon = qApp->style()->standardIcon(QStyle::SP_FileIcon);
        return icon;
    }

    const QIcon& standardDirIcon() {
        static const auto icon = qApp->style()->standardIcon(QStyle::SP_DirIcon);
        return icon;
    }

    void openFile(const QString& filePath, QWidget* parent) {
        const auto showDialogOnError = [&](std::optional<QString> error) {
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
        };

        if (!QFile::exists(filePath)) {
            warning().log("Can't open file {}, it does not exist", filePath);
            showDialogOnError(qApp->translate("tremotesf", "This file/directory does not exist"));
            return;
        }

        const auto url = QUrl::fromLocalFile(filePath);
        info().log("Executing QDesktopServices::openUrl() for {}", url);
        if (!QDesktopServices::openUrl(url)) {
            warning().log("QDesktopServices::openUrl() failed for {}", url);
            showDialogOnError({});
        }
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
