// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serverstats.h"

#include <QJsonObject>

#include "jsonutils.h"
#include "literals.h"

namespace tremotesf {
    using namespace impl;

    void SessionStats::update(const QJsonObject& stats) {
        mDownloaded = toInt64(stats.value("downloadedBytes"_l1));
        mUploaded = toInt64(stats.value("uploadedBytes"_l1));
        mDuration = stats.value("secondsActive"_l1).toInt();
        mSessionCount = stats.value("sessionCount"_l1).toInt();
    }

    void ServerStats::update(const QJsonObject& serverStats) {
        mDownloadSpeed = toInt64(serverStats.value("downloadSpeed"_l1));
        mUploadSpeed = toInt64(serverStats.value("uploadSpeed"_l1));
        mCurrentSession.update(serverStats.value("current-stats"_l1).toObject());
        mTotal.update(serverStats.value("cumulative-stats"_l1).toObject());
        emit updated();
    }
}
