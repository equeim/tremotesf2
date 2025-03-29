// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FORMATUTILS_H
#define TREMOTESF_FORMATUTILS_H

#include <QLocale>
#include <QString>

class QDateTime;

namespace tremotesf::formatutils {
    QString formatByteSize(long long size);
    QString formatByteSpeed(long long speed);
    QString formatSpeedLimit(int limit);

    QString formatProgress(double progress);
    QString formatRatio(double ratio);
    QString formatRatio(long long downloaded, long long uploaded);

    QString formatEta(int seconds);

    QString formatDateTime(const QDateTime& dateTime, QLocale::FormatType format, bool displayRelativeTime);
    QString formatDateTime(const QDateTime& dateTime, QLocale::FormatType format);
}

#endif // TREMOTESF_FORMATUTILS_H
