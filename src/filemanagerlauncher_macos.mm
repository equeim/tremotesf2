// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

#include <AppKit/NSWorkspace.h>
#include <QFileInfo>
#include <QWidget>

#include "log/log.h"

namespace tremotesf {
    namespace {
        class MacosFileManagerLauncher final : public impl::FileManagerLauncher {
            Q_OBJECT

        public:
            MacosFileManagerLauncher() = default;

        protected:
            void launchFileManagerAndSelectFiles(
                std::vector<FilesInDirectory> filesToSelect, QPointer<QWidget> parentWidget
            ) override {
                auto* const workspace = [NSWorkspace sharedWorkspace];
                NSMutableArray<NSURL*>* const fileUrls = [NSMutableArray arrayWithCapacity:filesToSelect.size()];
                std::vector<QString> fallbackDirectories{};
                for (const auto& [dirPath, files] : filesToSelect) {
                    for (const auto& filePath : files) {
                        // activateFileViewerSelectingURLs won't work is file does not exist
                        if (QFileInfo::exists(filePath)) {
                            auto* const url = [NSURL fileURLWithPath:filePath.toNSString()];
                            [fileUrls addObject:url];
                        } else {
                            if (std::find(fallbackDirectories.begin(), fallbackDirectories.end(), dirPath) ==
                                fallbackDirectories.end()) {
                                fallbackDirectories.push_back(dirPath);
                            }
                        }
                    }
                }
                if (const auto count = [fileUrls count]; count != 0) {
                    logInfo("Executing NSWorkspace.activateFileViewerSelectingURLs for {} files", count);
                    [workspace activateFileViewerSelectingURLs:fileUrls];
                }
                for (const auto& dirPath : fallbackDirectories) {
                    fallbackForDirectory(dirPath, parentWidget.get());
                }
                emit done();
            }
        };
    }

    namespace impl {
        FileManagerLauncher* FileManagerLauncher::createInstance() { return new MacosFileManagerLauncher(); }
    }
}

#include "filemanagerlauncher_macos.moc"
