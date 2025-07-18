// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_SERVERSTATS_H
#define TREMOTESF_RPC_SERVERSTATS_H

#include <QObject>

class QJsonObject;

namespace tremotesf {
    class SessionStats {
    public:
        [[nodiscard]] qint64 downloaded() const { return mDownloaded; };
        [[nodiscard]] qint64 uploaded() const { return mUploaded; };
        [[nodiscard]] int duration() const { return mDuration; };
        [[nodiscard]] int sessionCount() const { return mSessionCount; };

        void update(const QJsonObject& stats);

    private:
        qint64 mDownloaded{};
        qint64 mUploaded{};
        int mDuration{};
        int mSessionCount{};
    };

    class ServerStats final : public QObject {
        Q_OBJECT

    public:
        using QObject::QObject;

        [[nodiscard]] qint64 downloadSpeed() const { return mDownloadSpeed; };
        [[nodiscard]] qint64 uploadSpeed() const { return mUploadSpeed; };
        [[nodiscard]] qint64 freeSpace() const { return mFreeSpace; };

        [[nodiscard]] SessionStats currentSession() const { return mCurrentSession; };
        [[nodiscard]] SessionStats total() const { return mTotal; };

        void update(const QJsonObject& serverStats);
        void setFreeSpace(const qint64 freeSpace);

    private:
        qint64 mDownloadSpeed{};
        qint64 mUploadSpeed{};
        qint64 mFreeSpace{};
        SessionStats mCurrentSession{};
        SessionStats mTotal{};
    signals:
        void updated();
    };
}

#endif // TREMOTESF_RPC_SERVERSTATS_H
