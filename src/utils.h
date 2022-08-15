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

#ifdef Q_OS_WIN
#include <cstdint>
#include <functional>
#endif
#include <QObject>

namespace tremotesf
{
    class Utils : public QObject
    {
        Q_OBJECT
    public:
        Q_INVOKABLE static QString formatByteSize(long long size);
        Q_INVOKABLE static QString formatByteSpeed(long long speed);
        Q_INVOKABLE static QString formatSpeedLimit(int limit);

        Q_INVOKABLE static QString formatProgress(double progress);
        Q_INVOKABLE static QString formatRatio(double ratio);
        Q_INVOKABLE static QString formatRatio(long long downloaded, long long uploaded);

        Q_INVOKABLE static QString formatEta(int seconds);

        Q_INVOKABLE static QString readTextResource(const QString& resourcePath);
        Q_INVOKABLE static QString readTextFile(const QString& filePath);

    #ifdef Q_OS_UNIX
        static void callPosixFunctionWithErrno(std::function<bool()>&& function);
#else
#ifdef Q_OS_WIN
        static void callWinApiFunctionWithLastError(std::function<bool()>&& function);
        static void callCOMFunction(std::function<std::int32_t()>&& function);
#endif
#endif
    };
}

#endif // TREMOTESF_UTILS_H
