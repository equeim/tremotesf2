// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktoputils.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QUrl>

#include "libtremotesf/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf::desktoputils {
    QString statusIconPath(StatusIcon icon) {
        switch (icon) {
        case ActiveIcon:
            return ":/active.png"_l1;
        case CheckingIcon:
            return ":/checking.png"_l1;
        case DownloadingIcon:
            return ":/downloading.png"_l1;
        case ErroredIcon:
            return ":/errored.png"_l1;
        case PausedIcon:
            return ":/paused.png"_l1;
        case QueuedIcon:
            return ":/queued.png"_l1;
        case SeedingIcon:
            return ":/seeding.png"_l1;
        case StalledDownloadingIcon:
            return ":/stalled-downloading.png"_l1;
        case StalledSeedingIcon:
            return ":/stalled-seeding.png"_l1;
        }

        return {};
    }

    void openFile(const QString& filePath, QWidget* parent) {
        const auto url = QUrl::fromLocalFile(filePath);
        logInfo("Executing QDesktopServices::openUrl() for {}", url);
        if (!QDesktopServices::openUrl(url)) {
            logWarning("QDesktopServices::openUrl() failed for {}", url);
            auto dialog = new QMessageBox(
                QMessageBox::Warning,
                qApp->translate("tremotesf", "Error"),
                qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(filePath)),
                QMessageBox::Close,
                parent
            );
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }

    namespace {
        QRegularExpression urlRegex() {
            constexpr auto protocol = "(?:(?:[a-z]+:)?//)"_l1;
            constexpr auto host = "(?:(?:[a-z\\x{00a1}-\\x{ffff0}-9][-_]*)*[a-z\\x{00a1}-\\x{ffff0}-9]+)"_l1;
            constexpr auto domain = "(?:\\.(?:[a-z\\x{00a1}-\\x{ffff0}-9]-*)*[a-z\\x{00a1}-\\x{ffff0}-9]+)*"_l1;
            constexpr auto tld = "(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}]{2,}))\\.?"_l1;
            constexpr auto port = "(?::\\d{2,5})?"_l1;
            constexpr auto path = "(?:[/?#][^\\s\"\\)\']*)?"_l1;
            const auto regex =
                QString("(?:"_l1 % protocol % "|www\\.)(?:"_l1 % host % domain % tld % ")"_l1 % port % path);
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
            if (cursor.isNull()) { break; }
            auto format = baseFormat;
            format.setAnchorHref(cursor.selection().toPlainText());
            cursor.setCharFormat(format);
        }
    }
}
