// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"

#include <array>
#include <cmath>
#include <concepts>
#include <stdexcept>

#include <QCoreApplication>
#include <QFile>
#include <QLocale>

#include "libtremotesf/log.h"

namespace tremotesf {
    namespace {
        enum ByteUnit {
            Byte,
            KibiByte,
            MebiByte,
            GibiByte,
            TebiByte,
            PebiByte,
            ExbiByte,
            ZebiByte,
            YobiByte,
        };

        struct ByteUnitStrings {
            enum Type { Size, Speed };
            std::array<QString (*)(), 2> strings;
        };

        // Should be kept in sync with `enum ByteUnit`
        constexpr auto byteUnits = std::array{
            ByteUnitStrings{//: Size suffix in bytes
                            [] { return qApp->translate("tremotesf", "%L1 B"); },
                            //: Download speed suffix in bytes per second
                            [] { return qApp->translate("tremotesf", "%L1 B/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in kibibytes
                            [] { return qApp->translate("tremotesf", "%L1 KiB"); },
                            //: Download speed suffix in kibibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 KiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in mebibytes
                            [] { return qApp->translate("tremotesf", "%L1 MiB"); },
                            //: Download speed suffix in mebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 MiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in gibibytes
                            [] { return qApp->translate("tremotesf", "%L1 GiB"); },
                            //: Download speed suffix in gibibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 GiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in tebibytes
                            [] { return qApp->translate("tremotesf", "%L1 TiB"); },
                            //: Download speed suffix in tebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 TiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in pebibytes
                            [] { return qApp->translate("tremotesf", "%L1 PiB"); },
                            //: Download speed suffix in pebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 PiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in exbibytes
                            [] { return qApp->translate("tremotesf", "%L1 EiB"); },
                            //: Download speed suffix in exbibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 EiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in zebibytes
                            [] { return qApp->translate("tremotesf", "%L1 ZiB"); },
                            //: Download speed suffix in zebibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 ZiB/s"); }},
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{//: Size suffix in yobibytes
                            [] { return qApp->translate("tremotesf", "%L1 YiB"); },
                            //: Download speed suffix in yobibytes per second
                            [] { return qApp->translate("tremotesf", "%L1 YiB/s"); }},
        };

        QString formatBytes(long long bytes, ByteUnitStrings::Type stringType) {
            size_t unit = 0;
            auto bytes_d = static_cast<double>(bytes);
            while (bytes_d >= 1024.0 && unit <= byteUnits.size()) {
                bytes_d /= 1024.0;
                ++unit;
            }

            if (unit == Byte) {
                return byteUnits[Byte].strings[stringType]().arg(bytes_d);
            }
            return byteUnits[unit].strings[stringType]().arg(bytes_d, 0, 'f', 1);
        }

        template<std::invocable OnError>
        QString readTextFileImpl(const QString& filePath, OnError&& onError) {
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly)) {
                return file.readAll();
            }
            onError();
            return {};
        }
    }

    QString Utils::formatByteSize(long long size) { return formatBytes(size, ByteUnitStrings::Size); }

    QString Utils::formatByteSpeed(long long speed) { return formatBytes(speed, ByteUnitStrings::Speed); }

    QString Utils::formatSpeedLimit(int limit) {
        //: Download speed suffix in kibibytes per second
        return qApp->translate("tremotesf", "%L1 KiB/s").arg(limit);
    }

    QString Utils::formatProgress(double progress) {
        if (qFuzzyCompare(progress, 1.0)) {
            //: Progress in percents. %L1 must remain unchanged, % after it is a percent character
            return qApp->translate("tremotesf", "%L1%").arg(100);
        }
        //: Progress in percents. %L1 must remain unchanged, % after it is a percent character
        return qApp->translate("tremotesf", "%L1%").arg(std::trunc(progress * 1000.0) / 10.0, 0, 'f', 1);
    }

    QString Utils::formatRatio(double ratio) {
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

    QString Utils::formatRatio(long long downloaded, long long uploaded) {
        if (downloaded == 0) {
            return formatRatio(0);
        }
        return formatRatio(static_cast<double>(uploaded) / static_cast<double>(downloaded));
    }

    QString Utils::formatEta(int seconds) {
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

    QString Utils::readTextResource(const QString& resourcePath) {
        return readTextFileImpl(resourcePath, [&] {
            throw std::runtime_error(fmt::format("Failed to read resource with path '{}'", resourcePath));
        });
    }

    QString Utils::readTextFile(const QString& filePath) {
        return readTextFileImpl(filePath, [&] { logWarning("Failed to read file with path '{}'", filePath); });
    }
}
