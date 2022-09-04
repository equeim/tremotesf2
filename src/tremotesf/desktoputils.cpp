/*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

namespace tremotesf
{
    namespace desktoputils
    {
        QString statusIconPath(StatusIcon icon)
        {
            switch (icon) {
            case ActiveIcon:
                return QLatin1String(":/active.png");
            case CheckingIcon:
                return QLatin1String(":/checking.png");
            case DownloadingIcon:
                return QLatin1String(":/downloading.png");
            case ErroredIcon:
                return QLatin1String(":/errored.png");
            case PausedIcon:
                return QLatin1String(":/paused.png");
            case QueuedIcon:
                return QLatin1String(":/queued.png");
            case SeedingIcon:
                return QLatin1String(":/seeding.png");
            case StalledDownloadingIcon:
                return QLatin1String(":/stalled-downloading.png");
            case StalledSeedingIcon:
                return QLatin1String(":/stalled-seeding.png");
            }

            return QString();
        }

        void openFile(const QString& filePath, QWidget* parent)
        {
            const auto url = QUrl::fromLocalFile(filePath);
            logInfo("Executing QDesktopServices::openUrl() for {}", url);
            if (!QDesktopServices::openUrl(url)) {
                logWarning("QDesktopServices::openUrl() failed for {}", url);
                auto dialog = new QMessageBox(QMessageBox::Warning,
                                              qApp->translate("tremotesf", "Error"),
                                              qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(filePath)),
                                              QMessageBox::Close,
                                              parent);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            }
        }

        namespace {
            QRegularExpression urlRegex()
            {
                const auto protocol = QLatin1String("(?:(?:[a-z]+:)?//)");
                const auto host = QLatin1String("(?:(?:[a-z\\x{00a1}-\\x{ffff0}-9][-_]*)*[a-z\\x{00a1}-\\x{ffff0}-9]+)");
                const auto domain = QLatin1String("(?:\\.(?:[a-z\\x{00a1}-\\x{ffff0}-9]-*)*[a-z\\x{00a1}-\\x{ffff0}-9]+)*");
                const auto tld = QLatin1String("(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}]{2,}))\\.?");
                const auto port = QLatin1String("(?::\\d{2,5})?");
                const auto path = QLatin1String("(?:[/?#][^\\s\"\\)\']*)?");
                const auto regex = QString(QLatin1String("(?:") % protocol % QLatin1String("|www\\.)(?:") % host % domain % tld % QLatin1String(")") % port % path);
                return QRegularExpression(regex, QRegularExpression::CaseInsensitiveOption);
            }
        }

        void findLinksAndAddAnchors(QTextDocument *document)
        {
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
}
