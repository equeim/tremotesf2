// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <memory>
#include <type_traits>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QScopeGuard>
#include <QUrl>
#include <QtConcurrentRun>

#include <guiddef.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.h>
#include <shlobj.h>

#include <fmt/ranges.h>

#include "tremotesf/windowshelpers.h"

#include "libtremotesf/log.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::System;

namespace tremotesf {
    namespace {
        std::unique_ptr<std::remove_pointer_t<LPITEMIDLIST>, decltype(&ILFree)>
        createItemIdList(const QString& nativePath) {
            LPITEMIDLIST value{};
            checkHResult(
                SHParseDisplayName(getCWString(nativePath), nullptr, &value, 0, nullptr),
                "SHParseDisplayName"
            );
            return {value, &ILFree};
        }

        struct ItemIdListList {
            explicit ItemIdListList(size_t capacity) { values.reserve(capacity); }
            ~ItemIdListList() { std::for_each(values.begin(), values.end(), &ILFree); }

            void add(const QString& nativePath) { values.push_back(createItemIdList(nativePath).release()); }

            std::vector<LPITEMIDLIST> values{};
        };

        class WindowsFileManagerLauncher : public impl::FileManagerLauncher {
            Q_OBJECT
        protected:
            void launchFileManagerAndSelectFiles(
                const std::vector<std::pair<QString, std::vector<QString>>>& directories,
                const QPointer<QWidget>& parentWidget
            ) override {
                QtConcurrent::run([=] {
                    try {
                        winrt::init_apartment(winrt::apartment_type::multi_threaded);
                    } catch (const winrt::hresult_error& e) {
                        logWarningWithException(e, "WindowsFileManagerLauncher: winrt::init_apartment failed");
                    }
                    const auto guard = QScopeGuard([&] {
                        try {
                            winrt::uninit_apartment();
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(
                                e,
                                "WindowsFileManagerLauncher: winrt::uninit_apartment failed: {}"
                            );
                        }
                        emit done();
                    });

                    for (const auto& [dirPath, dirFiles] : directories) {
                        const QString nativePath = QDir::toNativeSeparators(dirPath);
                        logInfo(
                            "WindowsFileManagerLauncher: attempting to select {} items in folder {}",
                            dirFiles.size(),
                            nativePath
                        );
                        try {
                            if (isRunningOnWindows10OrGreater()) {
                                openFolderWindows10(nativePath, dirFiles);
                            } else {
                                openFolderWindows81(nativePath, dirFiles);
                            }
                        } catch (const std::runtime_error& e) {
                            logWarningWithException(e, "WindowsFileManagerLauncher: failed to select files");
                            fallbackForDirectory(dirPath, parentWidget);
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(e, "WindowsFileManagerLauncher: failed to select files");
                            fallbackForDirectory(dirPath, parentWidget);
                        }
                    }
                });
            }

            void fallbackForDirectory(const QString& dirPath, const QPointer<QWidget>& parentWidget) override {
                // Execute on main thread, blocking current thread
                QMetaObject::invokeMethod(
                    qApp,
                    [=] { impl::FileManagerLauncher::fallbackForDirectory(dirPath, parentWidget); },
                    Qt::BlockingQueuedConnection
                );
            }

        private:
            void openFolderWindows10(const QString& nativeDirPath, const std::vector<QString>& dirFiles) {
                const winrt::hstring folderPath(nativeDirPath.toStdWString());
                auto options = FolderLauncherOptions();
                for (const QString& filePath : dirFiles) {
                    const winrt::hstring winPath(QDir::toNativeSeparators(filePath).toStdWString());
                    if (QFileInfo(filePath).isDir()) {
                        try {
                            options.ItemsToSelect().Append(StorageFolder::GetFolderFromPathAsync(winPath).get());
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(
                                e,
                                "WindowsFileManagerLauncher: failed to create StorageFolder from {}",
                                winPath
                            );
                        }
                    } else {
                        try {
                            options.ItemsToSelect().Append(StorageFile::GetFileFromPathAsync(winPath).get());
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(
                                e,
                                "WindowsFileManagerLauncher: failed to create StorageFile from {}",
                                winPath
                            );
                        }
                    }
                }
                logInfo(
                    "WindowsFileManagerLauncher: opening folder {} and selecting {} items",
                    nativeDirPath,
                    options.ItemsToSelect().Size()
                );
                Launcher::LaunchFolderPathAsync(folderPath, options).get();
            }

            void openFolderWindows81(const QString& nativeDirPath, const std::vector<QString>& dirFiles) {
                auto directory = createItemIdList(nativeDirPath);
                ItemIdListList items(dirFiles.size());
                for (const QString& filePath : dirFiles) {
                    const QString nativeFilePath = QDir::toNativeSeparators(filePath);
                    try {
                        items.add(nativeFilePath);
                    } catch (const winrt::hresult_error& e) {
                        logWarningWithException(
                            e,
                            "WindowsFileManagerLauncher: failed to create LPITEMIDLIST from {}",
                            nativeFilePath
                        );
                    }
                }
                if (items.values.empty()) { throw std::runtime_error("No items to select"); }
                logInfo(
                    "WindowsFileManagerLauncher: opening folder {} and selecting {} items",
                    nativeDirPath,
                    items.values.size()
                );
                checkHResult(
                    SHOpenFolderAndSelectItems(
                        directory.get(),
                        static_cast<UINT>(items.values.size()),
                        const_cast<PCUITEMID_CHILD_ARRAY>(items.values.data()),
                        0
                    ),
                    "SHOpenFolderAndSelectItems"
                );
            }
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() { return new WindowsFileManagerLauncher(); }
    }
}

#include "filemanagerlauncher_windows.moc"
