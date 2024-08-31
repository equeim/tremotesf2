// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
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

#include "literals.h"
#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf::desktoputils {
    const QIcon& statusIcon(StatusIcon icon) {
        switch (icon) {
        case ActiveIcon: {
            static const QIcon icon(":/active.svg"_l1);
            return icon;
        }
        case CheckingIcon: {
            static const QIcon icon(":/checking.svg"_l1);
            return icon;
        }
        case DownloadingIcon: {
            static const QIcon icon(":/downloading.svg"_l1);
            return icon;
        }
        case ErroredIcon: {
            static const QIcon icon(":/errored.svg"_l1);
            return icon;
        }
        case PausedIcon: {
            static const QIcon icon(":/paused.svg"_l1);
            return icon;
        }
        case QueuedIcon: {
            static const QIcon icon(":/queued.svg"_l1);
            return icon;
        }
        case SeedingIcon: {
            static const QIcon icon(":/seeding.svg"_l1);
            return icon;
        }
        case StalledDownloadingIcon: {
            static const QIcon icon(":/stalled-downloading.svg"_l1);
            return icon;
        }

        case StalledSeedingIcon: {
            static QIcon icon(":/stalled-seeding.svg"_l1);
            return icon;
        }
        }

        throw std::logic_error(
            fmt::format("Unknown StatusIcon value {}", static_cast<std::underlying_type_t<StatusIcon>>(icon))
        );
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
                dialog->setText(dialog->text() % "\n\n"_l1 % *error);
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
            constexpr auto protocol = "(?:(?:[a-z]+:)?//)"_l1;
            constexpr auto host = R"((?:(?:[a-z\x{00a1}-\x{ffff0}-9][-_]*)*[a-z\x{00a1}-\x{ffff0}-9]+))";
            constexpr auto domain = R"((?:\.(?:[a-z\x{00a1}-\x{ffff0}-9]-*)*[a-z\x{00a1}-\x{ffff0}-9]+)*)";
            constexpr auto tld = R"((?:\.(?:[a-z\x{00a1}-\x{ffff}]{2,}))\.?)";
            constexpr auto port = R"((?::\d{2,5})?)";
            constexpr auto path = R"((?:[/?#][^\s"\)']*)?)";
            const auto regex =
                QString("(?:"_l1 % protocol % R"(|www\.)(?:)" % host % domain % tld % ")"_l1 % port % path);
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
