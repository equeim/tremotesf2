// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWVIEWMODEL_H
#define TREMOTESF_MAINWINDOWVIEWMODEL_H

#include <vector>

#include <QObject>

class QByteArray;
class QDragEnterEvent;
class QDropEvent;
class QString;

namespace tremotesf {
    class IpcServer;
    class Rpc;

    class MainWindowViewModel : public QObject {
        Q_OBJECT
    public:
        MainWindowViewModel(
            QStringList&& commandLineFiles,
            QStringList&& commandLineUrls,
            Rpc* rpc,
            IpcServer* ipcServer,
            QObject* parent = nullptr
        );

        static void processDragEnterEvent(QDragEnterEvent* event);
        void processDropEvent(QDropEvent* event);

    private:
        Rpc* mRpc{};
        QStringList mPendingFilesToOpen{};
        QStringList mPendingUrlsToOpen{};

    signals:
        void showWindow(const QByteArray& newStartupNotificationId);
        void showAddTorrentDialogs(const QStringList& files, const QStringList& urls);
        void showDelayedTorrentAddMessage(const QStringList& torrents);
    };
}

#endif // TREMOTESF_MAINWINDOWVIEWMODEL_H
