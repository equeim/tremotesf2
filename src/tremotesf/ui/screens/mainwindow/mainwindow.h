// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOW_H
#define TREMOTESF_MAINWINDOW_H

#include <functional>
#include <unordered_map>

#include <QMainWindow>

class QAction;
class QDragEnterEvent;
class QDropEvent;
class QMenu;
class QSplitter;
class QSystemTrayIcon;
class QToolBar;

namespace tremotesf
{
    class IpcServer;
    class MainWindowSideBar;
    class MainWindowViewModel;
    class NotificationsController;
    class Rpc;
    class TorrentsModel;
    class TorrentPropertiesDialog;
    class TorrentsProxyModel;
    class TorrentsView;

    class MainWindow final : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(
            QStringList&& commandLineFiles,
            QStringList&& commandLineUrls,
            IpcServer* ipcServer,
            QWidget* parent = nullptr
        );
        ~MainWindow() override;

        QSize sizeHint() const override;
        void showMinimized(bool minimized);

    protected:
        void closeEvent(QCloseEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    private:
        void setupActions();
        void updateRpcActions();

        void addTorrentsFiles();
        void showAddTorrentFileDialogs(const QStringList& files);
        void showAddTorrentLinkDialogs(const QStringList& urls);

        void updateTorrentActions();
        void showTorrentsPropertiesDialogs();
        void removeSelectedTorrents(bool deleteFiles);

        void setupMenuBar();
        void setupToolBar();
        void setupTrayIcon();

        void showWindow(const QByteArray& newStartupNotificationId = {});
        void hideWindow();

        void runAfterDelay(const std::function<void()>& function);

        void openTorrentsFiles();
        void showTorrentsInFileManager();

    private:
        Rpc* mRpc;
        MainWindowViewModel* mViewModel{};

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

        QMenu* mTorrentMenu = nullptr;
        QAction* mStartTorrentAction = nullptr;
        QAction* mStartTorrentNowAction = nullptr;
        QAction* mPauseTorrentAction = nullptr;
        QAction* mRemoveTorrentAction = nullptr;

        QAction* mRenameTorrentAction = nullptr;

        QAction* mOpenTorrentFilesAction = nullptr;
        QAction* mShowInFileManagerAction = nullptr;

        std::vector<QAction*> mConnectionDependentActions;

        QMenu* mFileMenu = nullptr;

        QToolBar* mToolBar = nullptr;
        QAction* mToolBarAction = nullptr;

        QSystemTrayIcon* mTrayIcon{};
        NotificationsController* mNotificationsController{};
    };
}

#endif // TREMOTESF_MAINWINDOW_H
