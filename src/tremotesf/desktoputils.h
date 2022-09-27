#ifndef TREMOTESF_DESKTOPUTILS_H
#define TREMOTESF_DESKTOPUTILS_H

#include <QString>

class QTextDocument;
class QWidget;

namespace tremotesf
{
    namespace desktoputils
    {
        constexpr int defaultDbusTimeout = 2000; // 2 seconds

        enum StatusIcon
        {
            ActiveIcon,
            CheckingIcon,
            DownloadingIcon,
            ErroredIcon,
            PausedIcon,
            QueuedIcon,
            SeedingIcon,
            StalledDownloadingIcon,
            StalledSeedingIcon
        };
        QString statusIconPath(StatusIcon icon);

        void openFile(const QString& filePath, QWidget* parent = nullptr);

        void findLinksAndAddAnchors(QTextDocument* document);
    }
}

#endif // TREMOTESF_DESKTOPUTILS_H
