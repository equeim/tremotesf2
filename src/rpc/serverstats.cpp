// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serverstats.h"

#include <QJsonObject>

#include "literals.h"

namespace tremotesf {
    void SessionStats::update(const QJsonObject& stats) {
        mDownloaded = stats.value("downloadedBytes"_l1).toInteger();
        mUploaded = stats.value("uploadedBytes"_l1).toInteger();
        mDuration = stats.value("secondsActive"_l1).toInt();
        mSessionCount = stats.value("sessionCount"_l1).toInt();
    }

    void ServerStats::update(const QJsonObject& serverStats) {
        mDownloadSpeed = serverStats.value("downloadSpeed"_l1).toInteger();
        mUploadSpeed = serverStats.value("uploadSpeed"_l1).toInteger();
        mCurrentSession.update(serverStats.value("current-stats"_l1).toObject());
        mTotal.update(serverStats.value("cumulative-stats"_l1).toObject());
        emit updated();
    }
}
