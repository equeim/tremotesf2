// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_UTILS_H
#define TREMOTESF_UTILS_H

#include <QString>

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
    };
}

#endif // TREMOTESF_UTILS_H
