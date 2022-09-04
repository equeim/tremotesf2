/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_UTILS_H
#define TREMOTESF_UTILS_H

#include <QString>

#ifdef Q_OS_UNIX
#include <functional>
#endif

namespace tremotesf
{
    class Utils
    {
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

#ifdef Q_OS_UNIX
        static void callPosixFunctionWithErrno(std::function<bool()>&& function);
#endif
    };
}

#endif // TREMOTESF_UTILS_H
