// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_PATHUTILS_H
#define TREMOTESF_RPC_PATHUTILS_H

#include <optional>
#include <QString>
#include <QUrl>

namespace tremotesf {
    bool isAbsoluteWindowsDOSFilePath(QStringView path);
    std::optional<QUrl> parsePathAsUrl(const QString& path);

    /**
     * We need to pass PathOs explicitly because we can't determing whether given path is Unix or Windows path from its string alone:
     * There is no way to distinguish whether '//foo' is Unix path with duplicate directory separator, or Windows UNC path
     * (we need to handle Windows paths with both '\' and '/' separators)
     */

    enum class PathOs { Unix, Windows };

    QString normalizePath(const QString& path, PathOs pathOs);
    QString normalizeLocalPathOrNetworkShareUrl(const QString& path);
    QString toNativeSeparators(const QString& path, PathOs pathOs);
    QString lastPathSegment(const QString& path);
}

#endif // TREMOTESF_RPC_PATHUTILS_H
