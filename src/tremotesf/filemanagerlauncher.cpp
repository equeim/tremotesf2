// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
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

#include "libtremotesf/log.h"
#include "libtremotesf/target_os.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

namespace tremotesf {
    namespace impl {
        void FileManagerLauncher::launchFileManagerAndSelectFiles(
            const std::vector<QString>& files, const QPointer<QWidget>& parentWidget
        ) {
            std::vector<std::pair<QString, std::vector<QString>>> directories{};
            for (const QString& filePath : files) {
                QString dirPath = QFileInfo(filePath).path();
                const auto found = std::find_if(directories.begin(), directories.end(), [&](const auto& d) {
                    return d.first == dirPath;
                });
                auto& dirFiles = [&]() -> std::vector<QString>& {
                    if (found != directories.end()) { return found->second; }
                    directories.push_back({std::move(dirPath), {}});
                    return directories.back().second;
                }();
                dirFiles.push_back(filePath);
            }
            launchFileManagerAndSelectFiles(directories, parentWidget);
        }

        void FileManagerLauncher::launchFileManagerAndSelectFiles(
            const std::vector<std::pair<QString, std::vector<QString>>>& directories,
            const QPointer<QWidget>& parentWidget
        ) {
            for (const auto& [dirPath, _] : directories) { fallbackForDirectory(dirPath, parentWidget); }
            emit done();
        }

        void FileManagerLauncher::fallbackForDirectory(const QString& dirPath, const QPointer<QWidget>& parentWidget) {
            const auto url = QUrl::fromLocalFile(dirPath);
            logInfo("FileManagerLauncher: executing QDesktopServices::openUrl() for {}", url);
            if (!QDesktopServices::openUrl(url)) {
                logWarning("FileManagerLauncher: QDesktopServices::openUrl() failed for {}", url);
                auto dialog = new QMessageBox(
                    QMessageBox::Warning,
                    qApp->translate("tremotesf", "Error"),
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
