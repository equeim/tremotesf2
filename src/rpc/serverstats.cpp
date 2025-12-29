// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serverstats.h"

#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace tremotesf {
    void SessionStats::update(const QJsonObject& stats) {
        mDownloaded = stats.value("downloadedBytes"_L1).toInteger();
        mUploaded = stats.value("uploadedBytes"_L1).toInteger();
        mDuration = stats.value("secondsActive"_L1).toInt();
        mSessionCount = stats.value("sessionCount"_L1).toInt();
    }

    void ServerStats::update(std::optional<QJsonObject> serverStats, std::optional<qint64> freeSpace) {
        if (serverStats.has_value()) {
            mDownloadSpeed = serverStats->value("downloadSpeed"_L1).toInteger();
            mUploadSpeed = serverStats->value("uploadSpeed"_L1).toInteger();
            mCurrentSession.update(serverStats->value("current-stats"_L1).toObject());
            mTotal.update(serverStats->value("cumulative-stats"_L1).toObject());
        }
        if (freeSpace.has_value()) {
            mFreeSpace = *freeSpace;
        }
        emit updated();
    }

    void ServerStats::setFreeSpace(const qint64 freeSpace) { mFreeSpace = freeSpace; }
}
