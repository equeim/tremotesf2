// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
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

#include "literals.h"
#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf::desktoputils {
    QString statusIconPath(StatusIcon icon) {
        switch (icon) {
        case ActiveIcon:
            return ":/active.svg"_l1;
        case CheckingIcon:
            return ":/checking.svg"_l1;
        case DownloadingIcon:
            return ":/downloading.svg"_l1;
        case ErroredIcon:
            return ":/errored.svg"_l1;
        case PausedIcon:
            return ":/paused.svg"_l1;
        case QueuedIcon:
            return ":/queued.svg"_l1;
        case SeedingIcon:
            return ":/seeding.svg"_l1;
        case StalledDownloadingIcon:
            return ":/stalled-downloading.svg"_l1;
        case StalledSeedingIcon:
            return ":/stalled-seeding.svg"_l1;
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
                //: Dialog title
                qApp->translate("tremotesf", "Error"),
                //: File opening error, %1 is a file path
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
