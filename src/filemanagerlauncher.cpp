// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <algorithm>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringBuilder>
#include <QUrl>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace impl {
        void
        FileManagerLauncher::launchFileManagerAndSelectFiles(const std::vector<QString>& files, QWidget* parentWidget) {
            std::vector<FilesInDirectory> filesToSelect{};
            std::vector<QString> nonExistentDirectories{};
            for (const QString& filePath : files) {
                QString dirPath = QFileInfo(filePath).path();

                if (std::ranges::contains(nonExistentDirectories, dirPath)) {
                    continue;
                }
                if (!QFileInfo::exists(dirPath)) {
                    warning().log("FileManagerLauncher: directory {} does not exist", dirPath);
                    nonExistentDirectories.push_back(dirPath);
                    continue;
                }

                const auto found = std::ranges::find(filesToSelect, dirPath, &FilesInDirectory::directory);
                auto& dirFiles = [&]() -> std::vector<QString>& {
                    if (found != filesToSelect.end()) {
                        return found->files;
                    }
                    filesToSelect.push_back({.directory = std::move(dirPath), .files = {}});
                    return filesToSelect.back().files;
                }();
                dirFiles.push_back(filePath);
            }
            if (!nonExistentDirectories.empty()) {
                const auto error = qApp->translate("tremotesf", "This directory does not exist");
                for (const auto& dirPath : nonExistentDirectories) {
                    showErrorDialog(dirPath, error, parentWidget);
                }
            }
            if (!filesToSelect.empty()) {
                launchFileManagerAndSelectFiles(std::move(filesToSelect), parentWidget);
            } else {
                emit done();
            }
        }

        void FileManagerLauncher::launchFileManagerAndSelectFiles(
            // NOLINTNEXTLINE(performance-unnecessary-value-param)
            std::vector<FilesInDirectory> filesToSelect,
            QWidget* parentWidget
        ) {
            for (const auto& [directory, _] : filesToSelect) {
                fallbackForDirectory(directory, parentWidget);
            }
            emit done();
        }

        void FileManagerLauncher::fallbackForDirectory(const QString& dirPath, QWidget* parentWidget) {
            const auto url = QUrl::fromLocalFile(dirPath);
            info().log("FileManagerLauncher: executing QDesktopServices::openUrl() for {}", url);
            if (!QDesktopServices::openUrl(url)) {
                warning().log("FileManagerLauncher: QDesktopServices::openUrl() failed for {}", url);
                showErrorDialog(dirPath, {}, parentWidget);
            }
        }

        void FileManagerLauncher::showErrorDialog(
            const QString& dirPath, const std::optional<QString>& error, QWidget* parentWidget
        ) {
            auto dialog = new QMessageBox(
                QMessageBox::Warning,
                //: Dialog title
                qApp->translate("tremotesf", "Error"),
                //: Directory opening error, %1 is a file path
                qApp->translate("tremotesf", "Error opening %1").arg(QDir::toNativeSeparators(dirPath)),
                QMessageBox::Close,
                parentWidget
            );
            if (error.has_value()) {
                dialog->setText(dialog->text() % "\n\n"_L1 % *error);
            }
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        }
    }

    void launchFileManagerAndSelectFiles(const std::vector<QString>& files, QWidget* parentWidget) {
        auto launcher = impl::FileManagerLauncher::createInstance();
        QObject::connect(launcher, &impl::FileManagerLauncher::done, launcher, &impl::FileManagerLauncher::deleteLater);
        launcher->launchFileManagerAndSelectFiles(files, parentWidget);
    }
}
