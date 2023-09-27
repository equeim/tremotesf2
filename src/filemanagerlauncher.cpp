// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <algorithm>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrl>

#include "log/log.h"
#include "target_os.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf {
    namespace impl {
        void FileManagerLauncher::launchFileManagerAndSelectFiles(
            const std::vector<QString>& files, const QPointer<QWidget>& parentWidget
        ) {
            std::vector<FilesInDirectory> filesToSelect{};
            for (const QString& filePath : files) {
                QString dirPath = QFileInfo(filePath).path();
                const auto found = std::find_if(filesToSelect.begin(), filesToSelect.end(), [&](const auto& d) {
                    return d.directory == dirPath;
                });
                auto& dirFiles = [&]() -> std::vector<QString>& {
                    if (found != filesToSelect.end()) {
                        return found->files;
                    }
                    filesToSelect.push_back({.directory = std::move(dirPath), .files = {}});
                    return filesToSelect.back().files;
                }();
                dirFiles.push_back(filePath);
            }
            launchFileManagerAndSelectFiles(std::move(filesToSelect), parentWidget);
        }

        void FileManagerLauncher::launchFileManagerAndSelectFiles(
            // NOLINTNEXTLINE(performance-unnecessary-value-param)
            std::vector<FilesInDirectory> filesToSelect,
            // NOLINTNEXTLINE(performance-unnecessary-value-param)
            QPointer<QWidget> parentWidget
        ) {
            for (const auto& [directory, _] : filesToSelect) {
                fallbackForDirectory(directory, parentWidget);
            }
            emit done();
        }

        void FileManagerLauncher::fallbackForDirectory(const QString& dirPath, const QPointer<QWidget>& parentWidget) {
            const auto url = QUrl::fromLocalFile(dirPath);
            logInfo("FileManagerLauncher: executing QDesktopServices::openUrl() for {}", url);
            if (!QDesktopServices::openUrl(url)) {
                logWarning("FileManagerLauncher: QDesktopServices::openUrl() failed for {}", url);
                auto dialog = new QMessageBox(
                    QMessageBox::Warning,
                    //: Dialog title
                    qApp->translate("tremotesf", "Error"),
                    //: Directory opening error, %1 is a file path
                    qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(dirPath)),
                    QMessageBox::Close,
                    parentWidget
                );
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            }
        }
    }

    void launchFileManagerAndSelectFiles(const std::vector<QString>& files, QWidget* parentWidget) {
        auto launcher = impl::FileManagerLauncher::createInstance();
        QObject::connect(launcher, &impl::FileManagerLauncher::done, launcher, &impl::FileManagerLauncher::deleteLater);
        launcher->launchFileManagerAndSelectFiles(files, parentWidget);
    }
}
