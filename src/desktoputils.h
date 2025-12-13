// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_DESKTOPUTILS_H
#define TREMOTESF_DESKTOPUTILS_H

class QIcon;
class QString;
class QTextDocument;
class QWidget;

namespace tremotesf::desktoputils {
    constexpr int defaultDbusTimeout = 2000; // 2 seconds

    enum class StatusIcon {
        Active,
        Checking,
        Downloading,
        Errored,
        Paused,
        Queued,
        Seeding,
        StalledDownloading,
        StalledSeeding
    };
    const QIcon& statusIcon(StatusIcon icon);

    enum class Priority { High, Normal, Low };
    const QIcon& priorityIcon(Priority priority);

    const QIcon& standardFileIcon();
    const QIcon& standardDirIcon();

    void openFile(const QString& filePath, QWidget* parent = nullptr);

    void findLinksAndAddAnchors(QTextDocument* document);
}

#endif // TREMOTESF_DESKTOPUTILS_H
