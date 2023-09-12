// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <guiddef.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.h>

#include <fmt/ranges.h>

#include "log/log.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::System;

namespace tremotesf {
    namespace {
        class WindowsFileManagerLauncher final : public impl::FileManagerLauncher {
            Q_OBJECT

        protected:
            void launchFileManagerAndSelectFiles(
                std::vector<FilesInDirectory> filesToSelect, QPointer<QWidget> parentWidget
            ) override {
                selectFiles(std::move(filesToSelect), parentWidget);
            }

        private:
            winrt::fire_and_forget selectFiles(std::vector<FilesInDirectory> filesToSelect, QPointer<QWidget> parentWidget) {
                for (auto&& [dirPath, dirFiles] : filesToSelect) {
                    co_await selectFilesInFolder(std::move(dirPath), std::move(dirFiles), parentWidget);
                }
                emit done();
            }

            winrt::Windows::Foundation::IAsyncAction selectFilesInFolder(QString dirPath, std::vector<QString> dirFiles, QPointer<QWidget> parentWidget) {
                auto options = FolderLauncherOptions();
                for (const QString& filePath : dirFiles) {
                    const auto nativeFilePath = winrt::hstring(QDir::toNativeSeparators(filePath).toStdWString());
                    if (QFileInfo(filePath).isDir()) {
                        try {
                            options.ItemsToSelect().Append(co_await StorageFolder::GetFolderFromPathAsync(nativeFilePath));
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(
                                e,
                                "WindowsFileManagerLauncher: failed to create StorageFolder from {}",
                                nativeFilePath
                            );
                        }
                    } else {
                        try {
                            options.ItemsToSelect().Append(co_await StorageFile::GetFileFromPathAsync(nativeFilePath));
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(
                                e,
                                "WindowsFileManagerLauncher: failed to create StorageFile from {}",
                                nativeFilePath
                            );
                        }
                    }
                }
                auto nativeDirPath = winrt::hstring(QDir::toNativeSeparators(dirPath).toStdWString());
                logInfo(
                    "WindowsFileManagerLauncher: opening folder {} and selecting {} items",
                    nativeDirPath,
                    options.ItemsToSelect().Size()
                );
                try {
                    co_await Launcher::LaunchFolderPathAsync(nativeDirPath, options);
                } catch (const winrt::hresult_error& e) {
                    logWarningWithException(e, "WindowsFileManagerLauncher: failed to select files");
                    fallbackForDirectory(dirPath, parentWidget);
                }
            }
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() { return new WindowsFileManagerLauncher(); }
    }
}

#include "filemanagerlauncher_windows.moc"
