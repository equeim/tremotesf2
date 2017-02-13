/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include <QObject>

namespace tremotesf
{
    class Utils : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString license READ license CONSTANT)
        Q_PROPERTY(QString translators READ translators CONSTANT)
    public:
        Q_INVOKABLE static QString formatByteSize(double size);
        Q_INVOKABLE static QString formatByteSpeed(double speed);
        Q_INVOKABLE static QString formatSpeedLimit(int limit);

        Q_INVOKABLE static QString formatProgress(float progress);
        Q_INVOKABLE static QString formatRatio(float ratio);

        Q_INVOKABLE static QString formatEta(int seconds);

        static QString kibiBytesPerSecond();

        static QString license();
        static QString translators();

        static void registerTypes();

#ifdef TREMOTESF_SAILFISHOS
        Q_PROPERTY(QString sdcardPath READ sdcardPath)
        static QString sdcardPath();
#else
        enum StatusIcon
        {
            ActiveIcon,
            CheckingIcon,
            DownloadingIcon,
            ErroredIcon,
            PausedIcon,
            QueuedIcon,
            SeedingIcon,
            StalledDownloadingIcon,
            StalledSeedingIcon
        };
        static QString statusIconPath(StatusIcon icon);
#endif
    };
}

#endif // TREMOTESF_UTILS_H
