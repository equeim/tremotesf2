// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileutils.h"

#include <span>
#include <stdexcept>

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStringBuilder>

#include <fmt/format.h>

#include "macoshelpers.h"
#include "target_os.h"
#include "log/log.h"

using namespace Qt::StringLiterals;

namespace fmt {
    template<>
    struct formatter<QFile::FileError> : tremotesf::SimpleFormatter {
        format_context::iterator format(QFile::FileError e, format_context& ctx) const {
            const std::string_view string = [e] {
                using namespace std::string_view_literals;
                switch (e) {
                case QFileDevice::NoError:
                    return "NoError"sv;
                case QFileDevice::ReadError:
                    return "ReadError"sv;
                case QFileDevice::WriteError:
                    return "WriteError"sv;
                case QFileDevice::FatalError:
                    return "FatalError"sv;
                case QFileDevice::ResourceError:
                    return "ResourceError"sv;
                case QFileDevice::OpenError:
                    return "OpenError"sv;
                case QFileDevice::AbortError:
                    return "AbortError"sv;
                case QFileDevice::TimeOutError:
                    return "TimeOutError"sv;
                case QFileDevice::UnspecifiedError:
                    return "UnspecifiedError"sv;
                case QFileDevice::RemoveError:
                    return "RemoveError"sv;
                case QFileDevice::RenameError:
                    return "RenameError"sv;
                case QFileDevice::PositionError:
                    return "PositionError"sv;
                case QFileDevice::ResizeError:
                    return "ResizeError"sv;
                case QFileDevice::PermissionsError:
                    return "PermissionsError"sv;
                case QFileDevice::CopyError:
                    return "CopyError"sv;
                }
                return std::string_view{};
            }();
            if (string.empty()) {
                return fmt::format_to(ctx.out(), "QFileDevice::FileError::<unnamed value {}>", std::to_underlying(e));
            }
            return fmt::format_to(ctx.out(), "QFileDevice::FileError::{}", string);
        }
    };
}

namespace tremotesf {
    namespace {
        std::string fileDescription(const QFile& file) {
            if (const QString fileName = file.fileName(); !fileName.isEmpty()) {
                return fmt::format(R"(file "{}")", fileName);
            }
            return fmt::format("file with handle={}", file.handle());
        }

        std::string errorDescription(const QFile& file) {
            return fmt::format("{} ({})", file.errorString(), file.error());
        }

        enum class ReadErrorType { FileError, UnexpectedEndOfFile };
        void throwReadError(const QFile& file, ReadErrorType type) {
            switch (type) {
            case ReadErrorType::UnexpectedEndOfFile:
                throw QFileError(fmt::format("Failed to read from {}: unexpected end of file", fileDescription(file)));
            case ReadErrorType::FileError:
                throw QFileError(
                    fmt::format("Failed to read from {}: {}", fileDescription(file), errorDescription(file))
                );
            }
            throw std::logic_error("Unknown ReadErrorType value");
        }

        struct ReadWholeBuffer {};
        struct ReadUntilEndOfFile {
            qint64 bytesRead{};
        };
        using ReadResult = std::variant<ReadWholeBuffer, ReadUntilEndOfFile>;
        [[nodiscard]] ReadResult readWholeBufferOrUntilEndOfFile(QFile& file, std::span<char> buffer) {
            if (buffer.empty()) {
                // If buffer's size is 0 then file.read() will return 0 which we will confuse with EOF condition
                // Just return early, there is nothing for us to do
                return ReadWholeBuffer{};
            }
            std::span<char> emptyBufferRemainder = buffer;
            while (true) {
                const qint64 bytesRead =
                    file.read(emptyBufferRemainder.data(), static_cast<qint64>(emptyBufferRemainder.size()));
                if (bytesRead == -1) {
                    // Error, throw
                    throwReadError(file, ReadErrorType::FileError);
                }
                if (bytesRead == 0) {
                    // End of file, return
                    const auto filledBufferSize = static_cast<qint64>(buffer.size() - emptyBufferRemainder.size());
                    return ReadUntilEndOfFile{.bytesRead = filledBufferSize};
                }
                if (bytesRead == static_cast<qint64>(emptyBufferRemainder.size())) {
                    // Read whole buffer, return
                    return ReadWholeBuffer{};
                }
                // Read part of buffer, continue
                emptyBufferRemainder = emptyBufferRemainder.subspan(static_cast<size_t>(bytesRead));
            }
        }
    }

    void openFile(QFile& file, QIODevice::OpenMode mode) {
        if (!file.open(mode)) {
            throw QFileError(fmt::format("Failed to open {}: {}", fileDescription(file), errorDescription(file)));
        }
    }

    void openFileFromFd(QFile& file, int fd, QIODevice::OpenMode mode) {
        if (!file.open(fd, mode)) {
            throw QFileError(fmt::format("Failed to open file from handle={}: {}", fd, errorDescription(file)));
        }
    }

    void readBytes(QFile& file, std::span<char> buffer) {
        const auto result = readWholeBufferOrUntilEndOfFile(file, buffer);
        if (std::holds_alternative<ReadUntilEndOfFile>(result)) {
            throwReadError(file, ReadErrorType::UnexpectedEndOfFile);
        }
    }

    void skipBytes(QFile& file, qint64 bytes) {
        if (bytes < 0) {
            throw std::invalid_argument(fmt::format("Argument bytes has invalid value {}, can't be negative", bytes));
        }
        if (bytes == 0) {
            // Nothing to do
            return;
        }
        auto remainingBytes = bytes;
        while (remainingBytes > 0) {
            const auto bytesSkipped = file.skip(remainingBytes);
            if (bytesSkipped == -1) {
                // Error, throw
                throwReadError(file, ReadErrorType::FileError);
            }
            if (bytesSkipped == 0) {
                // End of file, throw
                throwReadError(file, ReadErrorType::UnexpectedEndOfFile);
            }
            remainingBytes -= bytesSkipped;
        }
    }

    std::span<char> peekBytes(QFile& file, std::span<char> buffer) {
        if (buffer.empty()) {
            return buffer;
        }
        const auto peeked = file.peek(buffer.data(), static_cast<qint64>(buffer.size()));
        if (peeked == -1) {
            throwReadError(file, ReadErrorType::FileError);
        }
        if (peeked == 0) {
            throwReadError(file, ReadErrorType::UnexpectedEndOfFile);
        }
        return buffer.subspan(0, static_cast<size_t>(peeked));
    }

    void writeBytes(QFile& file, std::span<const char> data) {
        std::span<const char> remainingData = data;
        while (true) {
            const qint64 bytesWritten = file.write(remainingData.data(), static_cast<qint64>(remainingData.size()));
            if (bytesWritten == -1) {
                // Error, throw
                throw QFileError(
                    fmt::format("Failed to write to {}: {}", fileDescription(file), errorDescription(file))
                );
            }
            if (bytesWritten == static_cast<qint64>(remainingData.size())) {
                // Written whole buffer, return
                break;
            }
            // Written part of buffer, continue
            remainingData = remainingData.subspan(static_cast<size_t>(bytesWritten));
        }
    }

    QByteArray readFile(const QString& path) {
        QFile file(path);
        openFile(file, QIODevice::ReadOnly);
        auto data = file.readAll();
        if (file.error() != QFileDevice::NoError) {
            throwReadError(file, ReadErrorType::FileError);
        }
        return data;
    }

    namespace {
        void deleteFileImpl(QFile& file) {
            info().log("Deleting {}", fileDescription(file));
            if (file.remove()) {
                info().log("Succesfully deleted file");
            } else {
                throw QFileError(fmt::format("Failed to delete {}: {}", fileDescription(file), errorDescription(file)));
            }
        }
    }

    void deleteFile(const QString& filePath) {
        QFile file(filePath);
        deleteFileImpl(file);
    }

    void moveFileToTrashOrDelete(const QString& filePath) {
        QFile file(filePath);
        info().log("Moving {} to trash", fileDescription(file));
        if (file.moveToTrash()) {
            if (const auto newPath = file.fileName(); !newPath.isEmpty()) {
                info().log("Successfully moved file to trash, new path is {}", newPath);
            } else {
                info().log("Successfully moved file to trash");
            }
        } else {
            warning().log("Failed to move {} to trash: {}", fileDescription(file), errorDescription(file));
            deleteFileImpl(file);
        }
    }

    QString resolveExternalBundledResourcesPath(QLatin1String path) {
        const QString root = [&] {
            if constexpr (targetOs == TargetOs::UnixMacOS) {
                return bundleResourcesPath();
            } else {
                return QCoreApplication::applicationDirPath();
            }
        }();
        return root % '/' % path;
    }

    namespace impl {
        QString readFileAsBase64String(QFile& file) {
            QString string{};
            string.reserve(static_cast<QString::size_type>(((4 * file.size() / 3) + 3) & ~3));

            static constexpr qint64 bufferSize = 1024 * 1024 - 1; // 1 MiB minus 1 byte (dividable by 3)
            QByteArray buffer(bufferSize, '\0');

            while (true) {
                const auto result = readWholeBufferOrUntilEndOfFile(file, buffer);
                if (std::holds_alternative<ReadWholeBuffer>(result)) {
                    string.append(QLatin1String(buffer.toBase64()));
                    continue;
                }
                if (const auto readUntilEndOfFile = std::get_if<ReadUntilEndOfFile>(&result); readUntilEndOfFile) {
                    buffer.resize(static_cast<QByteArray::size_type>(readUntilEndOfFile->bytesRead));
                    string.append(QLatin1String(buffer.toBase64()));
                    break;
                }
            }

            return string;
        }

        namespace {
            constexpr auto sessionIdFileLocation = [] {
                if constexpr (targetOs == TargetOs::Windows) {
                    return QStandardPaths::GenericDataLocation;
                } else {
                    return QStandardPaths::TempLocation;
                }
            }();

            constexpr QLatin1String sessionIdFilePrefix = [] {
                if constexpr (targetOs == TargetOs::Windows) {
                    return "Transmission/tr_session_id_"_L1;
                } else {
                    return "tr_session_id_"_L1;
                }
            }();
        }

        bool isTransmissionSessionIdFileExists(QByteArrayView sessionId) {
            const auto file = QStandardPaths::locate(sessionIdFileLocation, sessionIdFilePrefix % sessionId);
            if (!file.isEmpty()) {
                info().log(
                    "isSessionIdFileExists: found transmission-daemon session id file {}",
                    QDir::toNativeSeparators(file)
                );
                return true;
            }
            info().log("isSessionIdFileExists: did not find transmission-daemon session id file");
            return false;
        }
    }
}
