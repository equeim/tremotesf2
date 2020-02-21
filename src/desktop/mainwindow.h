/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_MAINWINDOW_H
#define TREMOTESF_MAINWINDOW_H

#include <functional>
#include <unordered_map>

#include <QMainWindow>

class QAction;
class QMenu;
class QSplitter;
class QSystemTrayIcon;
class QToolBar;

namespace tremotesf
{
    class IpcServer;
    class MainWindowSideBar;
    class Rpc;
    class TorrentsModel;
    class TorrentPropertiesDialog;
    class TorrentsProxyModel;
    class TorrentsView;

    class MainWindow final : public QMainWindow
    {
    public:
        MainWindow(IpcServer* ipcServer, const QStringList& files, const QStringList& urls);
        ~MainWindow() override;

        QSize sizeHint() const override;
        void showMinimized(bool minimized);

    protected:
        void closeEvent(QCloseEvent* event) override;

    private:
        void setupActions();
        void updateRpcActions();

        void addTorrentsFiles();
        void showAddTorrentFileDialogs(const QStringList& files);
        void showAddTorrentLinkDialogs(const QStringList& urls);

        void updateTorrentActions();
        void showTorrentsPropertiesDialogs();
        void removeSelectedTorrents();

        void setupMenuBar();
        void setupToolBar();
        void setupTrayIcon();

        void showWindow();

        void runAfterDelay(const std::function<void()>& function);

        void showFinishedNotification(const QStringList& names);
        void showAddedNotification(const QStringList& names);
        void showTorrentsNotification(const QString& summary, const QStringList& torrents);
        void showNotification(const QString& summary, const QString& body);

        void openTorrentsFiles();
        void showTorrentsInFileManager();

    private:
        Rpc* mRpc;

        TorrentsModel* mTorrentsModel;
        TorrentsProxyModel* mTorrentsProxyModel;

        QSplitter* mSplitter;

        MainWindowSideBar* mSideBar;

        TorrentsView* mTorrentsView;
        std::unordered_map<int, TorrentPropertiesDialog*> mTorrentsDialogs;

        QAction* mConnectAction = nullptr;
        QAction* mDisconnectAction = nullptr;
        QAction* mAddTorrentFileAction = nullptr;
        QAction* mAddTorrentLinkAction = nullptr;

        QAction* mServerSettingsAction = nullptr;
        QAction* mServerStatsAction = nullptr;

        QMenu* mTorrentMenu = nullptr;
        QAction* mStartTorrentAction = nullptr;
        QAction* mStartTorrentNowAction = nullptr;
        QAction* mPauseTorrentAction = nullptr;
        QAction* mRemoveTorrentAction = nullptr;

        QAction* mOpenTorrentFilesAction = nullptr;
        QAction* mShowInFileManagerAction = nullptr;

        QMenu* mFileMenu = nullptr;

        QToolBar* mToolBar = nullptr;
        QAction* mToolBarAction = nullptr;

        QSystemTrayIcon* mTrayIcon;
    };
}

#endif // TREMOTESF_MAINWINDOW_H
