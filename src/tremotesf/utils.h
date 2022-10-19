// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_UTILS_H
#define TREMOTESF_UTILS_H

#include <QString>

#ifdef Q_OS_UNIX
#    include <functional>
#endif

namespace tremotesf {
    class Utils {
    public:
        static QString formatByteSize(long long size);
        static QString formatByteSpeed(long long speed);
        static QString formatSpeedLimit(int limit);

        static QString formatProgress(double progress);
        static QString formatRatio(double ratio);
        static QString formatRatio(long long downloaded, long long uploaded);

        static QString formatEta(int seconds);

        static QString readTextResource(const QString& resourcePath);
        static QString readTextFile(const QString& filePath);
    };
}

#endif // TREMOTESF_UTILS_H
