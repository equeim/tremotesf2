// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FILEMANAGERLAUNCHER_H
#define TREMOTESF_FILEMANAGERLAUNCHER_H

#include <optional>
#include <vector>

#include <QObject>
#include <QString>

class QWidget;

namespace tremotesf {
    namespace impl {
        class FileManagerLauncher : public QObject {
            Q_OBJECT

        public:
            static FileManagerLauncher* createInstance();
            void launchFileManagerAndSelectFiles(const std::vector<QString>& files, QWidget* parentWidget);

        protected:
            FileManagerLauncher() = default;

            struct FilesInDirectory {
                QString directory{};
                std::vector<QString> files{};
            };

            virtual void
            launchFileManagerAndSelectFiles(std::vector<FilesInDirectory> filesToSelect, QWidget* parentWidget);
            void fallbackForDirectory(const QString& dirPath, QWidget* parentWidget);

            void showErrorDialog(const QString& dirPath, const std::optional<QString>& error, QWidget* parentWidget);

        signals:
            void done();
        };
    }

    void launchFileManagerAndSelectFiles(const std::vector<QString>& files, QWidget* parentWidget);
}

#endif
