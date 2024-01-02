// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "formatutils.h"

#include <array>
#include <cmath>
#include <concepts>
#include <stdexcept>

#include <QCoreApplication>
#include <QFile>
#include <QLocale>

#include "fileutils.h"

namespace tremotesf::formatutils {
    namespace {
        enum class StringType { Size, Speed };

        struct ByteUnitStrings {
            QString (*size)();
            QString (*speed)();

            QString string(StringType type) const {
                switch (type) {
                case StringType::Size:
                    return size();
                case StringType::Speed:
                    return speed();
                }
                throw std::logic_error("Unknown StringType value");
            }
        };

        // Should be kept in sync with `enum ByteUnit`
        constexpr auto byteUnits = std::array{
            ByteUnitStrings{//: Size suffix in bytes
                            [] { return qApp->translate("tremotesf", "%L1 B"); },
                            //: Download speed suffix in bytes per second
                            [] { return qApp->translate("tremotesf", "%L1 B/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in kibibytes
                            [] { return qApp->translate("tremotesf", "%L1 KiB"); },
                            //: Download speed suffix in kibibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 KiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in mebibytes
                            [] { return qApp->translate("tremotesf", "%L1 MiB"); },
                            //: Download speed suffix in mebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 MiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in gibibytes
                            [] { return qApp->translate("tremotesf", "%L1 GiB"); },
                            //: Download speed suffix in gibibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 GiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in tebibytes
                            [] { return qApp->translate("tremotesf", "%L1 TiB"); },
                            //: Download speed suffix in tebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 TiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in pebibytes
                            [] { return qApp->translate("tremotesf", "%L1 PiB"); },
                            //: Download speed suffix in pebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 PiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in exbibytes
                            [] { return qApp->translate("tremotesf", "%L1 EiB"); },
                            //: Download speed suffix in exbibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 EiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in zebibytes
                            [] { return qApp->translate("tremotesf", "%L1 ZiB"); },
                            //: Download speed suffix in zebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 ZiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in yobibytes
                            [] { return qApp->translate("tremotesf", "%L1 YiB"); },
                            //: Download speed suffix in yobibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 YiB/s"); }
            },
        };
        constexpr size_t maxByteUnit = byteUnits.size() - 1;

        QString formatBytes(qint64 bytes, StringType stringType) {
            size_t unit = 0;
            auto bytes_floating = static_cast<double>(bytes);
            while (bytes_floating >= 1024.0 && unit < maxByteUnit) {
                bytes_floating /= 1024.0;
                ++unit;
            }
            if (unit == 0) {
                return byteUnits[0].string(stringType).arg(bytes_floating);
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            return byteUnits[unit].string(stringType).arg(bytes_floating, 0, 'f', 1);
        }
    }

    QString formatByteSize(long long size) { return formatBytes(size, StringType::Size); }

    QString formatByteSpeed(long long speed) { return formatBytes(speed, StringType::Speed); }

    QString formatSpeedLimit(int limit) {
        //: Download speed suffix in kibibytes per second
        return qApp->translate("tremotesf", "%L1 KiB/s").arg(limit);
    }

    QString formatProgress(double progress) {
        if (qFuzzyCompare(progress, 1.0)) {
            //: Progress in percents. %L1 must remain unchanged, % after it is a percent character
            return qApp->translate("tremotesf", "%L1%").arg(100);
        }
        //: Progress in percents. %L1 must remain unchanged, % after it is a percent character
        return qApp->translate("tremotesf", "%L1%").arg(std::trunc(progress * 1000.0) / 10.0, 0, 'f', 1);
    }

    QString formatRatio(double ratio) {
        if (ratio < 0) {
            return {};
        }

        int precision = 2;
        if (ratio >= 100) {
            precision = 0;
        } else if (ratio >= 10) {
            precision = 1;
        }
        return QLocale().toString(ratio, 'f', precision);
    }

    QString formatRatio(long long downloaded, long long uploaded) {
        if (downloaded == 0) {
            return formatRatio(0);
        }
        return formatRatio(static_cast<double>(uploaded) / static_cast<double>(downloaded));
    }

    QString formatEta(int seconds) {
        if (seconds < 0) {
            return "\u221E";
        }

        const int days = seconds / 86400;
        seconds %= 86400;
        const int hours = seconds / 3600;
        seconds %= 3600;
        const int minutes = seconds / 60;
        seconds %= 60;

        if (days > 0) {
            //: Remaining time string. %L1 is days, %L2 is hours, e.g. "2 d 5 h"
            return qApp->translate("tremotesf", "%L1 d %L2 h").arg(days).arg(hours);
        }

        if (hours > 0) {
            //: Remaining time string. %L1 is hours, %L2 is minutes, e.g. "2 h 5 m"
            return qApp->translate("tremotesf", "%L1 h %L2 m").arg(hours).arg(minutes);
        }

        if (minutes > 0) {
            //: Remaining time string. %L1 is minutes, %L2 is seconds, e.g. "2 m 5 s"
            return qApp->translate("tremotesf", "%L1 m %L2 s").arg(minutes).arg(seconds);
        }

        //: Remaining time string. %L1 is seconds, "10 s"
        return qApp->translate("tremotesf", "%L1 s").arg(seconds);
    }
}
