// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept>
#include <QRegularExpression>

#include "pathutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    // We can't use QDir::to/fromNativeSeparators because it checks for current OS,
    // and we need it to work regardless of OS we are running on

    static const QRegularExpression schemeUrlRegex(
        R"(^[a-zA-Z][a-zA-Z0-9+.-]+:/(?:/+)(?:[a-zA-Z0-9._~-]+)?(?::[a-zA-Z0-9._~-]*)?@?(?:[a-zA-Z0-9.-]+|\[[a-fA-F0-9:]+\]):?(?:\d+)?)"_L1
    );

    namespace {
        constexpr auto windowsSeparatorChar = '\\';
        constexpr auto unixSeparatorChar = '/';
        constexpr auto unixSeparatorString = "/"_L1;

        enum class PathType { Scheme, Unix, WindowsAbsoluteDOSFilePath, WindowsUNCOrDOSDevicePath };

        bool isWindowsUNCOrDOSDevicePath(QStringView path) {
            static const QRegularExpression regex(R"(^(?:\\|//).*$)"_L1);
            return regex.matchView(path).hasMatch();
        }

        PathType determinePathType(QStringView path, PathOs pathOs) {
            if (isSchemeUrl(QString(path))) {
                return PathType::Scheme;
            }
            switch (pathOs) {
            case PathOs::Unix:
                return PathType::Unix;
            case PathOs::Windows:
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
                case PathType::Scheme:
                    return 6; //  e.g. aa://a
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

        void normalizeSchemePrefix(QString& prefix) {
            // Lowercase the scheme
            const int colonPos = prefix.indexOf("://"_L1);
            if (colonPos != -1) {
                prefix.replace(0, colonPos, prefix.left(colonPos).toLower());
            }

            // Collapse multiple / after : to ://
            while (prefix.contains(":///")) {
                prefix.replace(":///", "://");
            }
        }
    }

    bool isAbsoluteWindowsDOSFilePath(QStringView path) {
        static const QRegularExpression regex(R"(^[A-Za-z]:[\\/]?.*$)"_L1);
        return regex.matchView(path).hasMatch();
    }

    bool isSchemeUrl(const QString& path) { return schemeUrlRegex.matchView(path).hasMatch(); }

    QString normalizePath(const QString& path, PathOs pathOs) {
        if (path.isEmpty()) {
            return path;
        }
        QString normalized = path.trimmed();
        if (normalized.isEmpty()) {
            return normalized;
        }
        // we will fill it and use if path type is a scheme URL
        QString pathPrefix;
        const auto pathType = determinePathType(normalized, pathOs);
        if (pathType == PathType::Scheme) {
            // For scheme URLs, normalize authority and the path part separately
            auto match = schemeUrlRegex.match(normalized);
            if (match.hasMatch()) {
                const int originalPrefixLength = match.capturedLength();
                pathPrefix = match.captured();
                normalizeSchemePrefix(pathPrefix);
                normalized = normalized.mid(originalPrefixLength);
            }
        }
        if (pathType != PathType::Unix && pathType != PathType::Scheme) {
            convertFromNativeWindowsSeparators(normalized);
            if (pathType == PathType::WindowsAbsoluteDOSFilePath) {
                capitalizeWindowsDriveLetter(normalized);
            }
        }
        collapseRepeatingSeparators(normalized, pathType);
        dropOrAddTrailingSeparator(normalized.prepend(pathPrefix), pathType);
        return normalized;
    }

    QString toNativeSeparators(const QString& path, PathOs pathOs) {
        if (path.isEmpty()) {
            return path;
        }
        QString native = path;
        if (determinePathType(native, pathOs) != PathType::Unix) {
            convertToNativeWindowsSeparators(native);
        }
        return native;
    }

    QString lastPathSegment(const QString& path) {
        const auto index = path.lastIndexOf(unixSeparatorChar);
        if (index == -1 || index == (path.size() - 1)) {
            return path;
        }
        return path.sliced(index + 1);
    }

}
