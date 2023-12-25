// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_IPCSERVER_H
#define TREMOTESF_IPCSERVER_H

#include <optional>
#include <QObject>

namespace tremotesf {
    struct WindowActivationToken {
#ifdef TREMOTESF_UNIX_FREEDESKTOP
        std::optional<QByteArray> x11StartupNotificationId{};
        std::optional<QByteArray> waylandXdfActivationToken{};
#endif
    };

    class IpcServer : public QObject {
        Q_OBJECT

    public:
        static IpcServer* createInstance(QObject* parent = nullptr);

    protected:
        inline explicit IpcServer(QObject* parent = nullptr) : QObject(parent){};

    signals:
        void windowActivationRequested(const QString& torrentHash, const WindowActivationToken& token);
        void
        torrentsAddingRequested(const QStringList& files, const QStringList& urls, const WindowActivationToken& token);
    };
}

#endif // TREMOTESF_IPCSERVER_H
