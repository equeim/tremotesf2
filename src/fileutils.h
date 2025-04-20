// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FILEUTILS_H
#define TREMOTESF_FILEUTILS_H

#include <span>

#include <QIODevice>
#include <QString>

class QFile;

namespace tremotesf {
    class QFileError : public std::runtime_error {
    public:
        explicit QFileError(const std::string& what) : std::runtime_error(what) {}
        explicit QFileError(const char* what) : std::runtime_error(what) {}
    };

    void openFile(QFile& file, QIODevice::OpenMode mode);
    void openFileFromFd(QFile& file, int fd, QIODevice::OpenMode mode);

    void readBytes(QFile& file, std::span<char> buffer);
    void skipBytes(QFile& file, qint64 bytes);
    [[nodiscard]] std::span<char> peekBytes(QFile& file, std::span<char> buffer);
    void writeBytes(QFile& file, std::span<const char> data);

    [[nodiscard]] QByteArray readFile(const QString& path);

    void deleteFile(const QString& filePath);
    void moveFileToTrashOrDelete(const QString& filePath);

    [[maybe_unused]]
    QString resolveExternalBundledResourcesPath(QLatin1String path);

    namespace impl {
        [[nodiscard]] QString readFileAsBase64String(QFile& file);
        [[nodiscard]] bool isTransmissionSessionIdFileExists(QByteArrayView sessionId);
    }
}

#endif // TREMOTESF_FILEUTILS_H
