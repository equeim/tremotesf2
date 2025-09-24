// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
    For formatRelativeDateTime()

    SPDX-FileCopyrightText: 2013 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2013 John Layt <jlayt@kde.org>
    SPDX-FileCopyrightText: 2010 Michael Leupold <lemma@confuego.org>
    SPDX-FileCopyrightText: 2009 Michael Pyne <mpyne@kde.org>
    SPDX-FileCopyrightText: 2008 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "formatutils.h"

#include <array>
#include <cmath>
#include <stdexcept>

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>

#include "settings.h"

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
            ByteUnitStrings{
                //: Size suffix in bytes
                .size = [] { return qApp->translate("tremotesf", "%L1 B"); },
                //: Download speed suffix in bytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 B/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in kibibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 KiB"); },
                //: Download speed suffix in kibibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 KiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in mebibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 MiB"); },
                //: Download speed suffix in mebibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 MiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in gibibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 GiB"); },
                //: Download speed suffix in gibibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 GiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in tebibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 TiB"); },
                //: Download speed suffix in tebibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 TiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in pebibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 PiB"); },
                //: Download speed suffix in pebibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 PiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in exbibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 EiB"); },
                //: Download speed suffix in exbibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 EiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in zebibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 ZiB"); },
                //: Download speed suffix in zebibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 ZiB/s"); }
            },
            //: IEC 80000 binary prefixes, i.e. KiB = 1024 bytes
            ByteUnitStrings{
                //: Size suffix in yobibytes
                .size = [] { return qApp->translate("tremotesf", "%L1 YiB"); },
                //: Download speed suffix in yobibytes per second
                .speed = [] { return qApp->translate("tremotesf", "%L1 YiB/s"); }
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

    namespace {
        // Adapted from KCoreAddons' KFormat

        QString formatRelativeDate(const QDate& date, QLocale::FormatType format, const QLocale& locale) {
            if (!date.isValid()) {
                return {};
            }

            const qint64 daysTo = QDate::currentDate().daysTo(date);
            if (daysTo > 0 || daysTo < -2) {
                return locale.toString(date, format);
            }

            // NOLINTNEXTLINE(bugprone-switch-missing-default-case)
            switch (daysTo) {
            case 0:
                return qApp->translate("tremotesf", "Today");
            case -1:
                return qApp->translate("tremotesf", "Yesterday");
            case -2:
                return qApp->translate("tremotesf", "Two days ago");
            }
            Q_UNREACHABLE();
        }

        QString
        addTimeToDate(const QString& dateString, QTime time, QLocale::FormatType format, const QLocale& locale) {
            //: Relative date & time
            return qApp->translate("tremotesf", "%1 at %2")
                .arg(
                    dateString,
                    locale.toString(
                        time,
                        format == QLocale::FormatType::LongFormat ? QLocale::FormatType::ShortFormat : format
                    )
                );
        }

        QString formatRelativeDateTime(const QDateTime& dateTime, QLocale::FormatType format, const QLocale& locale) {
            const QDateTime now = QDateTime::currentDateTime();

            const auto secsToNow = dateTime.secsTo(now);
            constexpr int secsInAHour = 60 * 60;
            if (secsToNow >= 0 && secsToNow < secsInAHour) {
                const auto minutesToNow = static_cast<int>(secsToNow / 60);
                if (minutesToNow <= 1) {
                    //: Relative time
                    return qApp->translate("tremotesf", "Just now");
                } else {
                    //: @item:intext %1 is a whole number
                    //~ singular %n minute ago
                    //~ plural %n minutes ago
                    return qApp->translate("tremotesf", "%n minute(s) ago", nullptr, minutesToNow);
                }
            }
            return addTimeToDate(formatRelativeDate(dateTime.date(), format, locale), dateTime.time(), format, locale);
        }
    }

    QString formatDateTime(const QDateTime& dateTime, QLocale::FormatType format, bool displayRelativeTime) {
        if (!dateTime.isValid()) {
            return {};
        }
        const QLocale locale{};
        if (displayRelativeTime) {
            return formatRelativeDateTime(dateTime, format, locale);
        }
        return addTimeToDate(locale.toString(dateTime.date(), format), dateTime.time(), format, locale);
    }

    QString formatDateTime(const QDateTime& dateTime, QLocale::FormatType format) {
        return formatDateTime(dateTime, format, Settings::instance()->get_displayRelativeTime());
    }
}
