#include "filemanagerlauncher.h"

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
#include "tremotesf/startup/main_windows.h"

#include "libtremotesf/log.h"

using namespace winrt::Windows::Storage;
using namespace winrt::Windows::System;

namespace tremotesf {
    namespace {
        class WindowsFileManagerLauncher : public impl::FileManagerLauncher {
            Q_OBJECT
        protected:
            void launchFileManagerAndSelectFiles(const std::vector<std::pair<QString, std::vector<QString>>>& directories, QPointer<QWidget> parentWidget) override {
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
                            logWarningWithException(e, "WindowsFileManagerLauncher: winrt::uninit_apartment failed: {}");
                        }
                        emit done();
                    });

                    for (const auto& [dirPath, dirFiles] : directories) {
                        const QString nativePath = QDir::toNativeSeparators(dirPath);
                        logInfo("WindowsFileManagerLauncher: opening folder {} and selecting {} items", nativePath, dirFiles.size());
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

            void fallbackForDirectory(const QString& dirPath, QPointer<QWidget> parentWidget) override {
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
                            logWarningWithException(e, "WindowsFileManagerLauncher: failed to create StorageFolder from {}", winPath);
                        }
                    } else {
                        try {
                            options.ItemsToSelect().Append(StorageFile::GetFileFromPathAsync(winPath).get());
                        } catch (const winrt::hresult_error& e) {
                            logWarningWithException(e, "WindowsFileManagerLauncher: failed to create StorageFile from {}", winPath);
                        }
                    }
                }
                Launcher::LaunchFolderPathAsync(folderPath, options).get();
            }

            void openFolderWindows81(const QString& nativeDirPath, const std::vector<QString>& dirFiles) {
                const auto directory = ILCreateFromPathW(reinterpret_cast<PCWSTR>(nativeDirPath.utf16()));
                if (!directory) {
                    throw std::runtime_error(fmt::format("ILCreateFromPathW failed() for {}", nativeDirPath));
                }
                const auto directoryGuard = QScopeGuard([&] { ILFree(directory); });

                std::vector<PIDLIST_ABSOLUTE> items{};
                const auto itemsGuard = QScopeGuard([&] {
                    std::for_each(items.begin(), items.end(), ILFree);
                });
                items.reserve(dirFiles.size() + 1);
                for (const QString& filePath : dirFiles) {
                    const QString nativeFilePath = QDir::toNativeSeparators(filePath);
                    const auto file = ILCreateFromPathW(reinterpret_cast<PCWSTR>(nativeFilePath.utf16()));
                    if (file) {
                        items.push_back(file);
                    } else {
                        logWarning("WindowsFileManagerLauncher: ILCreateFromPathW failed() for {}", nativeFilePath);
                    }
                }
                if (items.empty()) {
                    throw std::runtime_error("No items to select");
                }
                checkHResult(
                    SHOpenFolderAndSelectItems(
                        directory,
                        static_cast<UINT>(items.size()),
                        const_cast<PCUITEMID_CHILD_ARRAY>(items.data()),
                        0
                    ),
                    "SHOpenFolderAndSelectItems"
                );
            }
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() {
            return new WindowsFileManagerLauncher();
        }
    }
}

#include "filemanagerlauncher_windows.moc"
