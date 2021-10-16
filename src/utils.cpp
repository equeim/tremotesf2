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

#include "utils.h"

#include <array>
#include <cmath>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QLocale>

#ifdef TREMOTESF_SAILFISHOS
#include "sailfishos/sailfishosutils.h"
#endif

namespace tremotesf
{
    namespace
    {
        enum ByteUnit
        {
            Byte,
            KibiByte,
            MebiByte,
            GibiByte,
            TebiByte,
            PebiByte,
            ExbiByte,
            ZebiByte,
            YobiByte,
            NumberOfByteUnits
        };

        struct ByteUnitStrings
        {
            enum Type
            {
                Size,
                Speed
            };
            std::array<QString(*)(), 2> strings;
        };

        // Should be kept in sync with `enum ByteUnit`
        const std::array<ByteUnitStrings, NumberOfByteUnits> byteUnits{
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 B"); }, [] { return qApp->translate("tremotesf", "%L1 B/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 KiB"); }, [] { return qApp->translate("tremotesf", "%L1 KiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 MiB"); }, [] { return qApp->translate("tremotesf", "%L1 MiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 GiB"); }, [] { return qApp->translate("tremotesf", "%L1 GiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 TiB"); }, [] { return qApp->translate("tremotesf", "%L1 TiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 PiB"); }, [] { return qApp->translate("tremotesf", "%L1 PiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 EiB"); }, [] { return qApp->translate("tremotesf", "%L1 EiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 ZiB"); }, [] { return qApp->translate("tremotesf", "%L1 ZiB/s"); }},
                //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
                ByteUnitStrings{[] { return qApp->translate("tremotesf", "%L1 YiB"); }, [] { return qApp->translate("tremotesf", "%L1 YiB/s"); }},
        };

        QString formatBytes(long long bytes, ByteUnitStrings::Type stringType)
        {
            size_t unit = 0;
            auto bytes_d = static_cast<double>(bytes);
            while (bytes_d >= 1024.0 && unit <= NumberOfByteUnits) {
                bytes_d /= 1024.0;
                ++unit;
            }

            if (unit == Byte) {
                return byteUnits[Byte].strings[stringType]().arg(bytes_d);
            }
            return byteUnits[unit].strings[stringType]().arg(bytes_d, 0, 'f', 1);
        }
    }

    QString Utils::formatByteSize(long long size)
    {
        return formatBytes(size, ByteUnitStrings::Size);
    }

    QString Utils::formatByteSpeed(long long speed)
    {
        return formatBytes(speed, ByteUnitStrings::Speed);
    }

    QString Utils::formatSpeedLimit(int limit)
    {
        return qApp->translate("tremotesf", "%L1 KiB/s").arg(limit);
    }

    QString Utils::formatProgress(double progress)
    {
        if (qFuzzyCompare(progress, 1.0)) {
            return qApp->translate("tremotesf", "%L1%").arg(100);
        }
        return qApp->translate("tremotesf", "%L1%").arg(std::trunc(progress * 1000.0) / 10.0, 0, 'f', 1);
    }

    QString Utils::formatRatio(double ratio)
    {
        if (ratio < 0) {
            return QString();
        }

        int precision = 2;
        if (ratio >= 100) {
            precision = 0;
        } else if (ratio >= 10) {
            precision = 1;
        }
        return QLocale().toString(ratio, 'f', precision);
    }

    QString Utils::formatRatio(long long downloaded, long long uploaded)
    {
        if (downloaded == 0) {
            return formatRatio(0);
        }
        return formatRatio(static_cast<double>(uploaded) / static_cast<double>(downloaded));
    }

    QString Utils::formatEta(int seconds)
    {
        if (seconds < 0) {
            return u8"\u221E";
        }

        const int days = seconds / 86400;
        seconds %= 86400;
        const int hours = seconds / 3600;
        seconds %= 3600;
        const int minutes = seconds / 60;
        seconds %= 60;

        if (days > 0) {
            return qApp->translate("tremotesf", "%L1 d %L2 h").arg(days).arg(hours);
        }

        if (hours > 0) {
            return qApp->translate("tremotesf", "%L1 h %L2 m").arg(hours).arg(minutes);
        }

        if (minutes > 0) {
            return qApp->translate("tremotesf", "%L1 m %L2 s").arg(minutes).arg(seconds);
        }

        return qApp->translate("tremotesf", "%L1 s").arg(seconds);
    }

    QString Utils::readTextResource(const QString& path)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            return file.readAll();
        }
        qFatal("Failed to read resource with path \"%s\"", path.toUtf8().data());
        return {};
    }

    void Utils::registerTypes()
    {
#ifdef TREMOTESF_SAILFISHOS
        SailfishOSUtils::registerTypes();
#endif
    }
}
