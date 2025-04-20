// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_IPCSERVER_H
#define TREMOTESF_IPCSERVER_H

#include <optional>
#include <QObject>

namespace tremotesf {
    class IpcServer : public QObject {
        Q_OBJECT

    public:
        static IpcServer* createInstance(QObject* parent = nullptr);

    protected:
        inline explicit IpcServer(QObject* parent = nullptr) : QObject(parent) {};

    signals:
        void windowActivationRequested(const std::optional<QByteArray>& windowActivationToken);
        void torrentsAddingRequested(
            const QStringList& files, const QStringList& urls, const std::optional<QByteArray>& windowActivationToken
        );
    };
}

#endif // TREMOTESF_IPCSERVER_H
