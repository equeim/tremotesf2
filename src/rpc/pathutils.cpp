// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept>
#include <optional>
#include <QRegularExpression>
#include <QUrl>

#include "pathutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    // We can't use QDir::to/fromNativeSeparators because it checks for current OS,
    // and we need it to work regardless of OS we are running on

    namespace {
        constexpr auto windowsSeparatorChar = '\\';
        constexpr auto unixSeparatorChar = '/';
        constexpr auto unixSeparatorString = "/"_L1;

        constexpr PathOs localPathOs = targetOs == TargetOs::Windows ? PathOs::Windows : PathOs::Unix;

        enum class PathType { Unix, WindowsAbsoluteDOSFilePath, WindowsUNCOrDOSDevicePath };

        bool isWindowsUNCOrDOSDevicePath(QStringView path) {
            static const QRegularExpression regex(R"(^(?:\\|//).*$)"_L1);
            return regex.matchView(path).hasMatch();
        }

        std::optional<QUrl> parseUrl(const QString& path) {
            const QUrl url = path;
            if (url.isValid() && !url.isRelative() && !url.scheme().isEmpty() && !url.path().isEmpty()) {
                return url;
            }
            return std::nullopt;
        }

        PathType determinePathType(const QString& path, PathOs pathOs) {
            switch (pathOs) {
            case PathOs::Unix: {
                return PathType::Unix;
            }
            case PathOs::Windows:
                // There can be ambiguity here so DOS file path takes precedence
                if (isAbsoluteWindowsDOSFilePath(path)) {
                    return PathType::WindowsAbsoluteDOSFilePath;
                }
                if (isWindowsUNCOrDOSDevicePath(path)) {
                    return PathType::WindowsUNCOrDOSDevicePath;
                }
                return PathType::WindowsAbsoluteDOSFilePath;
            }
            throw std::logic_error("Unknown PathOs value");
        }

        void convertFromNativeWindowsSeparators(QString& path) {
            path.replace(windowsSeparatorChar, unixSeparatorChar);
        }

        void convertToNativeWindowsSeparators(QString& path) { path.replace(unixSeparatorChar, windowsSeparatorChar); }

        void capitalizeWindowsDriveLetter(QString& path) {
            if (path.size() >= 2 && path[1] == ':') {
                const auto drive = path[0];
                if (drive.isLower()) {
                    path[0] = drive.toUpper();
                }
            }
        }

        void collapseRepeatingSeparators(QString& path, PathType pathType) {
            const auto& regex = [pathType]() -> const QRegularExpression& {
                if (pathType == PathType::WindowsUNCOrDOSDevicePath) {
                    // Don't collapse leading '//'
                    static const QRegularExpression regex(R"((?!^)//+)"_L1);
                    return regex;
                }
                static const QRegularExpression regex(R"(//+)"_L1);
                return regex;
            }();
            path.replace(regex, unixSeparatorString);
        }

        void dropOrAddTrailingSeparator(QString& path, PathType pathType) {
            const auto minimumLength = [pathType] {
                switch (pathType) {
                case PathType::Unix:
                    return 1; // e.g. '/'
                case PathType::WindowsAbsoluteDOSFilePath:
                    return 3; // e.g. 'C:/'
                case PathType::WindowsUNCOrDOSDevicePath:
                    return 2; // e.g. '//'
                }
                throw std::logic_error("Unknown PathType value");
            }();
            if (path.size() <= minimumLength) {
                if (pathType == PathType::WindowsAbsoluteDOSFilePath && path.size() == 2) {
                    path.append(unixSeparatorChar);
                }
                return;
            }
            if (path.back() == unixSeparatorChar) {
                path.chop(1);
            }
        }

        void normalizePathImpl(QString& path, PathType pathType) {
            if (pathType != PathType::Unix) {
                convertFromNativeWindowsSeparators(path);
                if (pathType == PathType::WindowsAbsoluteDOSFilePath) {
                    capitalizeWindowsDriveLetter(path);
                }
            }
            collapseRepeatingSeparators(path, pathType);
            dropOrAddTrailingSeparator(path, pathType);
        }
    }

    bool isAbsoluteWindowsDOSFilePath(QStringView path) {
        static const QRegularExpression regex(R"(^[A-Za-z]:[\\/]?.*$)"_L1);
        return regex.matchView(path).hasMatch();
    }

    QString normalizePath(const QString& path, PathOs pathOs) {
        if (path.isEmpty()) {
            return {};
        }
        QString result = path.trimmed();
        if (result.isEmpty()) {
            return {};
        }
        normalizePathImpl(result, determinePathType(path, pathOs));
        return result;
    }

    QString normalizeLocalPathOrNetworkShareUrl(const QString& path) {
        if (path.isEmpty()) {
            return {};
        }
        QString result = path.trimmed();
        if (result.isEmpty()) {
            return {};
        }
        if constexpr (localPathOs == PathOs::Windows) {
            // Windows path can be parsed as url, handle it first
            if (isAbsoluteWindowsDOSFilePath(result)) {
                normalizePathImpl(result, PathType::WindowsAbsoluteDOSFilePath);
                return result;
            }
        }
        if (auto url = parseUrl(result); url.has_value()) {
            if (url->isLocalFile()) {
                // We shouldn't have file:// urls here, but handle them just in case
                auto localPath = url->toLocalFile();
                normalizePathImpl(localPath, determinePathType(localPath, localPathOs));
                return localPath;
            }
            auto urlPath = url->path();
            normalizePathImpl(urlPath, PathType::Unix);
            url->setPath(urlPath);
            return url->toString();
        }
        normalizePathImpl(result, determinePathType(result, localPathOs));
        return result;
    }

    QString toNativeSeparators(const QString& path, PathOs pathOs) {
        if (path.isEmpty() || pathOs != PathOs::Windows) {
            return path;
        }
        QString result = path;
        convertToNativeWindowsSeparators(result);
        return result;
    }

    QString lastPathSegment(const QString& path) {
        const auto index = path.lastIndexOf(unixSeparatorChar);
        if (index == -1 || index == (path.size() - 1)) {
            return path;
        }
        return path.sliced(index + 1);
    }

}
