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

#include "mainwindow.h"

#include <algorithm>

#include <QDebug>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolBar>

#ifdef QT_DBUS_LIB
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#endif

#include "../libtremotesf/serversettings.h"
#include "../libtremotesf/serverstats.h"
#include "../libtremotesf/torrent.h"
#include "../ipcserver.h"
#include "../localtorrentfilesmodel.h"
#include "../servers.h"
#include "../settings.h"
#include "../torrentfileparser.h"
#include "../torrentsmodel.h"
#include "../torrentsproxymodel.h"
#include "../trpc.h"
#include "../utils.h"
#include "aboutdialog.h"
#include "addtorrentdialog.h"
#include "mainwindowsidebar.h"
#include "mainwindowstatusbar.h"
#include "remotedirectoryselectionwidget.h"
#include "servereditdialog.h"
#include "serversdialog.h"
#include "serversettingsdialog.h"
#include "serverstatsdialog.h"
#include "settingsdialog.h"
#include "torrentpropertiesdialog.h"
#include "torrentsview.h"

namespace tremotesf
{
    namespace
    {
        class SetLocationDialog : public QDialog
        {
        public:
            explicit SetLocationDialog(const QString& downloadDirectory,
                                       bool serverIsLocal,
                                       QWidget* parent = nullptr)
                : QDialog(parent),
                  mDirectoryWidget(new RemoteDirectorySelectionWidget(downloadDirectory, serverIsLocal, this)),
                  mMoveFilesCheckBox(new QCheckBox(qApp->translate("tremotesf", "Move files from current directory"), this))
            {
                setWindowTitle(qApp->translate("tremotesf", "Set Location"));

                auto layout = new QVBoxLayout(this);
                layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

                auto label = new QLabel(qApp->translate("tremotesf", "Download directory:"), this);
                label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                layout->addWidget(label);

                mDirectoryWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                layout->addWidget(mDirectoryWidget);

                mMoveFilesCheckBox->setChecked(true);
                layout->addWidget(mMoveFilesCheckBox);

                auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
                QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &SetLocationDialog::accept);
                QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SetLocationDialog::reject);

                QObject::connect(mDirectoryWidget->lineEdit(), &QLineEdit::textChanged, this, [=](const QString& text) {
                    dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
                });

                layout->addWidget(dialogButtonBox);
            }

            QSize sizeHint() const override
            {
                return QDialog::sizeHint().expandedTo(QSize(320, 0));
            }

            QString downloadDirectory() const
            {
                return mDirectoryWidget->lineEdit()->text();
            }

            bool moveFiles() const
            {
                return mMoveFilesCheckBox->isChecked();
            }

        private:
            FileSelectionWidget *const mDirectoryWidget;
            QCheckBox *const mMoveFilesCheckBox;
        };
    }

    MainWindow::MainWindow(IpcServer* ipcServer, const QStringList& arguments)
        : mRpc(new Rpc(this)),
          mTorrentsModel(new TorrentsModel(mRpc, this)),
          mTorrentsProxyModel(new TorrentsProxyModel(mTorrentsModel, TorrentsModel::SortRole, this)),
          mSplitter(new QSplitter(this)),
          mSideBar(new MainWindowSideBar(mRpc, mTorrentsProxyModel)),
          mTorrentsView(new TorrentsView(mTorrentsProxyModel, this)),
          mTrayIcon(new QSystemTrayIcon(QIcon::fromTheme(QLatin1String("tremotesf-tray-icon"), windowIcon()), this))
    {
        setWindowTitle(QLatin1String("Tremotesf"));

        setMinimumSize(minimumSizeHint().expandedTo(QSize(384, 256)));
        if (!restoreGeometry(Settings::instance()->mainWindowGeometry())) {
            const QSize screenSize(qApp->primaryScreen()->size());
            const QSize windowSize(sizeHint());
            move((screenSize.width() - windowSize.width()) / 2, (screenSize.height() - windowSize.height()) / 2);
        }

        setContextMenuPolicy(Qt::NoContextMenu);
        setToolButtonStyle(Settings::instance()->toolButtonStyle());

        if (Servers::instance()->hasServers()) {
            mRpc->setServer(Servers::instance()->currentServer());
        }

        QObject::connect(Servers::instance(), &Servers::currentServerChanged, this, [this]() {
            if (Servers::instance()->hasServers()) {
                mRpc->setServer(Servers::instance()->currentServer());
            } else {
                mRpc->resetServer();
            }
        });

        mSplitter->setChildrenCollapsible(false);

        if (!Settings::instance()->isSideBarVisible()) {
            mSideBar->hide();
        }
        mSplitter->addWidget(mSideBar);

        mSplitter->addWidget(mTorrentsView);
        mSplitter->setStretchFactor(1, 1);
        QObject::connect(mTorrentsView, &TorrentsView::customContextMenuRequested, this, [=](const QPoint& point) {
            if (mTorrentsView->indexAt(point).isValid()) {
                mTorrentMenu->popup(QCursor::pos());
            }
        });
        QObject::connect(mTorrentsView, &TorrentsView::activated, this, &MainWindow::showTorrentsPropertiesDialogs);

        mSplitter->restoreState(Settings::instance()->splitterState());

        setCentralWidget(mSplitter);

        setupActions();
        setupToolBar();

        setStatusBar(new MainWindowStatusBar(mRpc));
        if (!Settings::instance()->isStatusBarVisible()) {
            statusBar()->hide();
        }

        setupMenuBar();
        setupTrayIcon();

        QObject::connect(ipcServer, &IpcServer::windowActivationRequested, this, &MainWindow::showWindow);

        QObject::connect(ipcServer, &IpcServer::filesReceived, this, [=](const QStringList& files) {
            if (mRpc->isConnected()) {
                setWindowState(windowState() & ~Qt::WindowMinimized);
                if (isHidden()) {
                    show();
                    runAfterDelay([=]() {
                        showAddTorrentFileDialogs(files);
                    });
                } else {
                    showAddTorrentFileDialogs(files);
                }
            }
        });

        QObject::connect(ipcServer, &IpcServer::urlsReceived, this, [=](const QStringList& urls) {
            if (mRpc->isConnected()) {
                setWindowState(windowState() & ~Qt::WindowMinimized);
                if (isHidden()) {
                    show();
                    runAfterDelay([=]() {
                        showAddTorrentLinkDialogs(urls);
                    });
                } else {
                    showAddTorrentLinkDialogs(urls);
                }
            }
        });

        QObject::connect(mRpc, &Rpc::connectedChanged, this, [=]() {
            if (mRpc->isConnected()) {
                static bool first = true;
                if (first) {
                    const ArgumentsParseResult result(IpcServer::parseArguments(arguments));
                    if (!result.files.isEmpty() || !result.urls.isEmpty()) {
                        setWindowState(windowState() & ~Qt::WindowMinimized);
                        if (isHidden()) {
                            show();
                            runAfterDelay([=]() {
                                showAddTorrentFileDialogs(result.files);
                                showAddTorrentLinkDialogs(result.urls);
                            });
                        } else {
                            showAddTorrentFileDialogs(result.files);
                            showAddTorrentLinkDialogs(result.urls);
                        }
                    }
                    first = false;
                }
            } else {
                if ((mRpc->error() != Rpc::NoError) && Settings::instance()->notificationOnDisconnecting()) {
                    showNotification(qApp->translate("tremotesf", "Disconnected"), mRpc->statusString());
                }
            }
        });

        QObject::connect(mRpc, &Rpc::addedNotificationRequested, this, [=](const QStringList&, const QStringList& names) {
            showAddedNotification(names);
        });

        QObject::connect(mRpc, &Rpc::finishedNotificationRequested, this, [=](const QStringList&, const QStringList& names) {
            showFinishedNotification(names);
        });

        QObject::connect(mRpc, &Rpc::torrentAddDuplicate, this, [=]() {
            QMessageBox::warning(this,
                                 qApp->translate("tremotesf", "Error adding torrent"),
                                 qApp->translate("tremotesf", "This torrent is already added"),
                                 QMessageBox::Close);
        });

        QObject::connect(mRpc, &Rpc::torrentAddError, this, [=]() {
            QMessageBox::warning(this,
                                 qApp->translate("tremotesf", "Error adding torrent"),
                                 qApp->translate("tremotesf", "Error adding torrent"),
                                 QMessageBox::Close);
        });

        if (Servers::instance()->hasServers()) {
            if (Settings::instance()->connectOnStartup()) {
                mRpc->connect();
            }
        } else {
            runAfterDelay([=]() {
                auto dialog = new ServerEditDialog(nullptr, -1, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerEditDialog::accepted, this, [=]() {
                    if (Settings::instance()->connectOnStartup()) {
                        mRpc->connect();
                    }
                });
                dialog->open();
            });
        }
    }

    MainWindow::~MainWindow()
    {
        mRpc->disconnect();

        Settings::instance()->setMainWindowGeometry(saveGeometry());
        Settings::instance()->setToolButtonStyle(toolButtonStyle());
        Settings::instance()->setToolBarArea(toolBarArea(mToolBar));
        Settings::instance()->setSplitterState(mSplitter->saveState());

        mTrayIcon->hide();
    }

    QSize MainWindow::sizeHint() const
    {
        return layout()->totalMinimumSize().expandedTo(QSize(896, 640));
    }

    void MainWindow::showMinimized(bool minimized)
    {
        if (!(minimized && Settings::instance()->showTrayIcon() && QSystemTrayIcon::isSystemTrayAvailable())) {
            show();
        }
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        QMainWindow::closeEvent(event);
        if (!(mTrayIcon->isVisible() && QSystemTrayIcon::isSystemTrayAvailable())) {
            qApp->quit();
        }
    }

    void MainWindow::setupActions()
    {
        mConnectAction = new QAction(qApp->translate("tremotesf", "&Connect"), this);
        QObject::connect(mConnectAction, &QAction::triggered, mRpc, &Rpc::connect);

        mDisconnectAction = new QAction(qApp->translate("tremotesf", "&Disconnect"), this);
        QObject::connect(mDisconnectAction, &QAction::triggered, mRpc, &Rpc::disconnect);

        const QIcon connectIcon(QIcon::fromTheme(QLatin1String("network-connect")));
        const QIcon disconnectIcon(QIcon::fromTheme(QLatin1String("network-disconnect")));
        if (connectIcon.name() != disconnectIcon.name()) {
            mConnectAction->setIcon(connectIcon);
            mDisconnectAction->setIcon(disconnectIcon);
        }

        mAddTorrentFileAction = new QAction(QIcon::fromTheme(QLatin1String("list-add")), qApp->translate("tremotesf", "&Add Torrent File..."), this);
        mAddTorrentFileAction->setShortcuts(QKeySequence::Open);
        QObject::connect(mAddTorrentFileAction, &QAction::triggered, this, &MainWindow::addTorrentsFiles);

        mAddTorrentLinkAction = new QAction(QIcon::fromTheme(QLatin1String("insert-link")), qApp->translate("tremotesf", "Add Torrent &Link..."), this);
        QObject::connect(mAddTorrentLinkAction, &QAction::triggered, this, [=]() {
            setWindowState(windowState() & ~Qt::WindowMinimized);
            auto showDialog = [=]() {
                auto dialog = new AddTorrentDialog(mRpc, QString(), this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            };
            if (isHidden()) {
                show();
                runAfterDelay(showDialog);
            } else {
                showDialog();
            }
        });

        //
        // Torrent menu
        //
        mTorrentMenu = new QMenu(qApp->translate("tremotesf", "&Torrent"), this);

        QAction* torrentPropertiesAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("document-properties")), qApp->translate("tremotesf", "&Properties"));
        QObject::connect(torrentPropertiesAction, &QAction::triggered, this, &MainWindow::showTorrentsPropertiesDialogs);

        mTorrentMenu->addSeparator();

        mStartTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("media-playback-start")), qApp->translate("tremotesf", "&Start"));
        QObject::connect(mStartTorrentAction, &QAction::triggered, this, [=]() {
            mRpc->startTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mStartTorrentNowAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("media-playback-start")), qApp->translate("tremotesf", "Start &Now"));
        QObject::connect(mStartTorrentNowAction, &QAction::triggered, this, [=]() {
            mRpc->startTorrentsNow(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mPauseTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("media-playback-pause")), qApp->translate("tremotesf", "P&ause"));
        QObject::connect(mPauseTorrentAction, &QAction::triggered, this, [=]() {
            mRpc->pauseTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mTorrentMenu->addSeparator();

        mRemoveTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("list-remove")), qApp->translate("tremotesf", "&Remove"));
        mRemoveTorrentAction->setShortcut(QKeySequence::Delete);
        QObject::connect(mRemoveTorrentAction, &QAction::triggered, this, &MainWindow::removeSelectedTorrents);

        QAction* setLocationAction = mTorrentMenu->addAction(qApp->translate("tremotesf", "Set &Location"));
        QObject::connect(setLocationAction, &QAction::triggered, this, [=]() {
            if (mTorrentsView->selectionModel()->hasSelection()) {
                QModelIndexList indexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows()));
                auto dialog = new SetLocationDialog(mTorrentsModel->torrentAtIndex(indexes.first())->downloadDirectory(),
                                                    mRpc->isLocal(),
                                                    this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &SetLocationDialog::accepted, this, [=]() {
                    mRpc->setTorrentsLocation(mTorrentsModel->idsFromIndexes(indexes), dialog->downloadDirectory(), dialog->moveFiles());
                });
                dialog->show();
            }
        });

        mTorrentMenu->addSeparator();

        mOpenTorrentFilesAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("document-open")), qApp->translate("tremotesf", "&Open"));
        QObject::connect(mOpenTorrentFilesAction, &QAction::triggered, this, &MainWindow::openTorrentsFiles);

        mShowInFileManagerAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("go-jump")), qApp->translate("tremotesf", "Show In &File Manager"));
        QObject::connect(mShowInFileManagerAction, &QAction::triggered, this, &MainWindow::showTorrentsInFileManager);

        mTorrentMenu->addSeparator();

        QAction* checkTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("document-preview")), qApp->translate("tremotesf", "&Check Local Data"));
        QObject::connect(checkTorrentAction, &QAction::triggered, this, [=]() {
            mRpc->checkTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* reannounceAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("view-refresh")), qApp->translate("tremotesf", "Reanno&unce"));
        QObject::connect(reannounceAction, &QAction::triggered, this, [=]() {
            mRpc->reannounceTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mTorrentMenu->addSeparator();

        QMenu* queueMenu = mTorrentMenu->addMenu(qApp->translate("tremotesf", "&Queue"));

        QAction* moveTorrentToTopAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-top")), qApp->translate("tremotesf", "Move To &Top"));
        QObject::connect(moveTorrentToTopAction, &QAction::triggered, this, [=]() {
            mRpc->moveTorrentsToTop(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* moveTorrentUpAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-up")), qApp->translate("tremotesf", "Move &Up"));
        QObject::connect(moveTorrentUpAction, &QAction::triggered, this, [=]() {
            mRpc->moveTorrentsUp(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* moveTorrentDownAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-down")), qApp->translate("tremotesf", "Move &Down"));
        QObject::connect(moveTorrentDownAction, &QAction::triggered, this, [=]() {
            mRpc->moveTorrentsDown(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* moveTorrentToBottomAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-bottom")), qApp->translate("tremotesf", "Move To &Bottom"));
        QObject::connect(moveTorrentToBottomAction, &QAction::triggered, this, [=]() {
            mRpc->moveTorrentsToBottom(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mServerSettingsAction = new QAction(QIcon::fromTheme(QLatin1String("configure"), QIcon::fromTheme(QLatin1String("preferences-system"))), qApp->translate("tremotesf", "&Server Options"), this);
        QObject::connect(mServerSettingsAction, &QAction::triggered, this, [=]() {
            static ServerSettingsDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new ServerSettingsDialog(mRpc, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerSettingsDialog::destroyed, this, []() {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });

        mServerStatsAction = new QAction(qApp->translate("tremotesf", "Server S&tats"), this);
        QObject::connect(mServerStatsAction, &QAction::triggered, this, [=]() {
            static ServerStatsDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new ServerStatsDialog(mRpc, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerSettingsDialog::destroyed, this, []() {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });

        updateRpcActions();
        QObject::connect(mRpc, &Rpc::statusChanged, this, &MainWindow::updateRpcActions);
        QObject::connect(Servers::instance(), &Servers::hasServersChanged, this, &MainWindow::updateRpcActions);

        updateTorrentActions();
        QObject::connect(mRpc, &Rpc::torrentsUpdated, this, &MainWindow::updateTorrentActions);
        QObject::connect(mTorrentsView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateTorrentActions);
    }

    void MainWindow::updateRpcActions()
    {
        if (mRpc->status() == Rpc::Disconnected) {
            if (Servers::instance()->hasServers()) {
                mConnectAction->setEnabled(true);
                mDisconnectAction->setEnabled(false);
            } else {
                mConnectAction->setEnabled(false);
                mDisconnectAction->setEnabled(false);
            }
        } else {
            mConnectAction->setEnabled(false);
            mDisconnectAction->setEnabled(true);
        }

        if (mRpc->status() == Rpc::Connected) {
            mAddTorrentFileAction->setEnabled(true);
            mAddTorrentLinkAction->setEnabled(true);
            mServerSettingsAction->setEnabled(true);
            mServerStatsAction->setEnabled(true);
        } else {
            mAddTorrentFileAction->setEnabled(false);
            mAddTorrentLinkAction->setEnabled(false);
            mServerSettingsAction->setEnabled(false);
            mServerStatsAction->setEnabled(false);
        }
    }

    void MainWindow::addTorrentsFiles()
    {
        setWindowState(windowState() & ~Qt::WindowMinimized);

        auto showDialog = [=]() {
            auto fileDialog = new QFileDialog(this,
                                              qApp->translate("tremotesf", "Select Files"),
                                              QString(),
                                              qApp->translate("tremotesf", "Torrent Files (*.torrent)"));
            fileDialog->setAttribute(Qt::WA_DeleteOnClose);
            fileDialog->setFileMode(QFileDialog::ExistingFiles);

            QObject::connect(fileDialog, &QFileDialog::accepted, this, [=]() {
                showAddTorrentFileDialogs(fileDialog->selectedFiles());
            });

#ifdef Q_OS_WIN
            fileDialog->open();
#else
            fileDialog->show();
#endif
        };

        if (isHidden()) {
            show();
            runAfterDelay(showDialog);
        } else {
            showDialog();
        }
    }

    void MainWindow::showAddTorrentFileDialogs(const QStringList& files)
    {
        if (files.isEmpty()) {
            return;
        }

        for (const QString& filePath : files) {
            auto parser = new TorrentFileParser(filePath, this);
            QObject::connect(parser, &TorrentFileParser::done, this, [=]() {
                if (parser->error() == TorrentFileParser::NoError) {
                    auto filesModel = new LocalTorrentFilesModel(parser->parseResult(), this);
                    QObject::connect(filesModel, &LocalTorrentFilesModel::loadedChanged, this, [=]() {
                        auto dialog = new AddTorrentDialog(mRpc, filePath, parser, filesModel, this);
                        dialog->setAttribute(Qt::WA_DeleteOnClose);
                        dialog->show();
                        dialog->activateWindow();
                        QObject::disconnect(filesModel, &LocalTorrentFilesModel::loadedChanged, this, nullptr);
                    });
                } else {
                    auto messageBox = new QMessageBox(QMessageBox::Critical,
                                                      qApp->translate("tremotesf", "Error"),
                                                      parser->errorString(),
                                                      QMessageBox::Close,
                                                      this);
                    messageBox->setAttribute(Qt::WA_DeleteOnClose);
                    messageBox->show();
                }
            });
        }
    }

    void MainWindow::showAddTorrentLinkDialogs(const QStringList& urls)
    {
        for (const QString& url : urls) {
            auto dialog = new AddTorrentDialog(mRpc, url, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->activateWindow();
        }
    }

    void MainWindow::updateTorrentActions()
    {
        if (mTorrentsView->selectionModel()->hasSelection()) {
            for (QAction* action : mTorrentMenu->actions()) {
                action->setEnabled(true);
            }
            const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
            if (selectedRows.size() == 1) {
                switch (mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(selectedRows.first()))->status()) {
                case libtremotesf::Torrent::Paused:
                case libtremotesf::Torrent::Errored:
                    mPauseTorrentAction->setEnabled(false);
                    break;
                default:
                    mStartTorrentAction->setEnabled(false);
                    mStartTorrentNowAction->setEnabled(false);
                }
            }

            if (mRpc->isLocal() || Servers::instance()->currentServerHasMountedDirectories()) {
                bool disableOpen = false;
                bool disableBoth = false;
                for (const QModelIndex& index : selectedRows) {
                    libtremotesf::Torrent* torrent = mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index));
                    if (mRpc->isTorrentLocalMounted(torrent) && QFile::exists(mRpc->localTorrentDownloadDirectoryPath(torrent))) {
                        if (!disableOpen && !QFile::exists(mRpc->localTorrentFilesPath(torrent))) {
                            disableOpen = true;
                        }
                    } else {
                        disableBoth = true;
                        break;
                    }
                }

                if (disableBoth) {
                    mOpenTorrentFilesAction->setEnabled(false);
                    mShowInFileManagerAction->setEnabled(false);
                } else if (disableOpen) {
                    mOpenTorrentFilesAction->setEnabled(false);
                }
            } else {
                mOpenTorrentFilesAction->setEnabled(false);
                mShowInFileManagerAction->setEnabled(false);
            }
        } else {
            for (QAction* action : mTorrentMenu->actions()) {
                action->setEnabled(false);
            }
        }
    }

    void MainWindow::showTorrentsPropertiesDialogs()
    {
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());

        for (int i = 0, max = selectedRows.size(); i < max; i++) {
            libtremotesf::Torrent* torrent = mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(selectedRows.at(i)));
            const int id = torrent->id();
            if (mTorrentsDialogs.find(id) != mTorrentsDialogs.end()) {
                if (i == (max - 1)) {
                    TorrentPropertiesDialog* dialog = mTorrentsDialogs[id];
                    dialog->raise();
                    dialog->activateWindow();
                }
            } else {
                auto dialog = new TorrentPropertiesDialog(torrent,
                                                          mRpc,
                                                          this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                mTorrentsDialogs.insert({id, dialog});
                QObject::connect(dialog, &TorrentPropertiesDialog::finished, this, [=]() {
                    mTorrentsDialogs.erase(mTorrentsDialogs.find(id));
                });

                dialog->show();
            }
        }
    }

    void MainWindow::removeSelectedTorrents()
    {
        QMessageBox dialog(this);
        dialog.setIcon(QMessageBox::Warning);

        dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialog.button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Remove"));
        dialog.setDefaultButton(QMessageBox::Cancel);

        QCheckBox deleteFilesCheckBox(qApp->translate("tremotesf", "Also delete the files on the hard disk"));
        dialog.setCheckBox(&deleteFilesCheckBox);

        const QVariantList ids(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        if (ids.size() == 1) {
            dialog.setWindowTitle(qApp->translate("tremotesf", "Remove Torrent"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to remove this torrent?"));
        } else {
            dialog.setWindowTitle(qApp->translate("tremotesf", "Remove Torrents"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to remove %n selected torrents?", nullptr, ids.size()));
        }

        if (dialog.exec() == QMessageBox::Ok) {
            mRpc->removeTorrents(ids, deleteFilesCheckBox.checkState() == Qt::Checked);
        }
    }

    void MainWindow::setupMenuBar()
    {
        mFileMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&File"));
        mFileMenu->addAction(mConnectAction);
        mFileMenu->addAction(mDisconnectAction);
        mFileMenu->addSeparator();
        mFileMenu->addAction(mAddTorrentFileAction);
        mFileMenu->addAction(mAddTorrentLinkAction);
        mFileMenu->addSeparator();

        QAction* quitAction = mFileMenu->addAction(QIcon::fromTheme(QLatin1String("application-exit")), qApp->translate("tremotesf", "&Quit"));
#ifdef Q_OS_WIN
        quitAction->setShortcut(QKeySequence(QLatin1String("Ctrl+Q")));
#else
        quitAction->setShortcuts(QKeySequence::Quit);
#endif
        QObject::connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);

        QMenu* editMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&Edit"));

        QAction* selectAllAction = editMenu->addAction(QIcon::fromTheme(QLatin1String("edit-select-all")), qApp->translate("tremotesf", "Select &All"));
        selectAllAction->setShortcut(QKeySequence::SelectAll);
        QObject::connect(selectAllAction, &QAction::triggered, mTorrentsView, &TorrentsView::selectAll);

        QAction* invertSelectionAction = editMenu->addAction(qApp->translate("tremotesf", "&Invert Selection"));
        QObject::connect(invertSelectionAction, &QAction::triggered, this, [=]() {
            mTorrentsView->selectionModel()->select(QItemSelection(mTorrentsProxyModel->index(0, 0),
                                                                   mTorrentsProxyModel->index(mTorrentsProxyModel->rowCount() - 1,
                                                                                              mTorrentsProxyModel->columnCount() - 1)),
                                                    QItemSelectionModel::Toggle);
        });

        menuBar()->addMenu(mTorrentMenu);

        QMenu* viewMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&View"));

        QAction* toolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Toolbar"));
        toolBarAction->setCheckable(true);
        toolBarAction->setChecked(Settings::instance()->isToolBarVisible());
        QObject::connect(toolBarAction, &QAction::triggered, this, [=](bool checked) {
            mToolBar->setVisible(checked);
            Settings::instance()->setToolBarVisible(checked);
        });

        QAction* sideBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Sidebar"));
        sideBarAction->setCheckable(true);
        sideBarAction->setChecked(Settings::instance()->isSideBarVisible());
        QObject::connect(sideBarAction, &QAction::triggered, this, [=](bool checked) {
            mSideBar->setVisible(checked);
            Settings::instance()->setSideBarVisible(checked);
        });

        QAction* statusBarAction = viewMenu->addAction(qApp->translate("tremotesf", "St&atusbar"));
        statusBarAction->setCheckable(true);
        statusBarAction->setChecked(Settings::instance()->isStatusBarVisible());
        QObject::connect(statusBarAction, &QAction::triggered, this, [=](bool checked) {
            statusBar()->setVisible(checked);
            Settings::instance()->setStatusBarVisible(checked);
        });

        QMenu* toolsMenu = menuBar()->addMenu(qApp->translate("tremotesf", "T&ools"));

        QAction* settingsAction = toolsMenu->addAction(QIcon::fromTheme(QLatin1String("configure"), QIcon::fromTheme(QLatin1String("preferences-system"))), qApp->translate("tremotesf", "&Options"));
        settingsAction->setShortcut(QKeySequence::Preferences);
        QObject::connect(settingsAction, &QAction::triggered, this, [=]() {
            static SettingsDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new SettingsDialog(this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &SettingsDialog::destroyed, this, []() {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });

        QAction* serversAction = toolsMenu->addAction(qApp->translate("tremotesf", "&Servers"));
        QObject::connect(serversAction, &QAction::triggered, this, [=]() {
            static ServersDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new ServersDialog(this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServersDialog::destroyed, this, []() {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });

        toolsMenu->addSeparator();
        toolsMenu->addAction(mServerSettingsAction);
        toolsMenu->addAction(mServerStatsAction);

        QMenu* helpMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&Help"));

        QAction* aboutAction = helpMenu->addAction(QIcon::fromTheme(QLatin1String("help-about")), qApp->translate("tremotesf", "&About"));
        QObject::connect(aboutAction, &QAction::triggered, this, [=]() {
            static AboutDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new AboutDialog(this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &AboutDialog::destroyed, this, []() {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });
    }

    void MainWindow::setupToolBar()
    {
        mToolBar = new QToolBar(this);
        mToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
        if (!Settings::instance()->isToolBarVisible()) {
            mToolBar->hide();
        }
        addToolBar(Settings::instance()->toolBarArea(), mToolBar);

        mToolBar->addAction(mConnectAction);
        mToolBar->addAction(mDisconnectAction);
        mToolBar->addSeparator();
        mToolBar->addAction(mAddTorrentFileAction);
        mToolBar->addAction(mAddTorrentLinkAction);
        mToolBar->addSeparator();
        mToolBar->addAction(mStartTorrentAction);
        mToolBar->addAction(mPauseTorrentAction);
        mToolBar->addAction(mRemoveTorrentAction);

        QObject::connect(mToolBar, &QToolBar::customContextMenuRequested, this, [=]() {
            QMenu contextMenu;
            QActionGroup group(this);

            group.addAction(qApp->translate("tremotesf", "Icon Only"))->setCheckable(true);
            group.addAction(qApp->translate("tremotesf", "Text Only"))->setCheckable(true);
            group.addAction(qApp->translate("tremotesf", "Text Beside Icon"))->setCheckable(true);
            group.addAction(qApp->translate("tremotesf", "Text Under Icon"))->setCheckable(true);
            group.addAction(qApp->translate("tremotesf", "Follow System Style"))->setCheckable(true);

            group.actions().at(toolButtonStyle())->setChecked(true);

            contextMenu.addActions(group.actions());

            QAction* action = contextMenu.exec(QCursor::pos());
            if (action) {
                setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(contextMenu.actions().indexOf(action)));
            }
        });
    }

    void MainWindow::setupTrayIcon()
    {
        QMenu* contextMenu = new QMenu(mFileMenu);
        for (QAction* action : mFileMenu->actions()) {
            contextMenu->addAction(action);
        }

        mTrayIcon->setContextMenu(contextMenu);
        mTrayIcon->setToolTip(mRpc->statusString());

        QObject::connect(mTrayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                if (isHidden() || isMinimized()) {
                    showWindow();
                } else {
                    hide();
                }
            }
        });

        QObject::connect(mRpc, &Rpc::statusStringChanged, this, [=]() {
            mTrayIcon->setToolTip(mRpc->statusString());
        });

        QObject::connect(mRpc->serverStats(), &libtremotesf::ServerStats::updated, this, [=]() {
            mTrayIcon->setToolTip(QString(u8"\u25be %1\n\u25b4 %2")
                                      .arg(Utils::formatByteSpeed(mRpc->serverStats()->downloadSpeed()))
                                      .arg(Utils::formatByteSpeed(mRpc->serverStats()->uploadSpeed())));
        });

        if (Settings::instance()->showTrayIcon()) {
            mTrayIcon->show();
        }

        QObject::connect(Settings::instance(), &Settings::showTrayIconChanged, this, [=]() {
            if (Settings::instance()->showTrayIcon()) {
                mTrayIcon->show();
            } else {
                mTrayIcon->hide();
                showWindow();
            }
        });
    }

    void MainWindow::showWindow()
    {
        show();
        setWindowState(windowState() & ~Qt::WindowMinimized);
        raise();
        activateWindow();
    }

    void MainWindow::runAfterDelay(const std::function<void()>& function)
    {
        auto timer = new QTimer(this);
        timer->setInterval(100);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, this, function);
        QObject::connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
        timer->start();
    }

    void MainWindow::showFinishedNotification(const QStringList& names)
    {
        showTorrentsNotification(names.size() == 1 ? qApp->translate("tremotesf", "Torrent finished")
                                                   : qApp->translate("tremotesf", "%n torrents finished", nullptr, names.size()),
                                 names);
    }

    void MainWindow::showAddedNotification(const QStringList& names)
    {
        showTorrentsNotification(names.size() == 1 ? qApp->translate("tremotesf", "Torrent added")
                                                   : qApp->translate("tremotesf", "%n torrents added", nullptr, names.size()),
                                 names);
    }

    void MainWindow::showTorrentsNotification(const QString& summary, const QStringList& torrents)
    {
        if (torrents.size() == 1) {
            showNotification(summary, torrents.first());
        } else {
            QStringList join(torrents);
            for (QString& torrent : join) {
                torrent.prepend(u8"\u2022 ");
            }
            showNotification(summary, join.join('\n'));
        }
    }

    void MainWindow::showNotification(const QString& summary, const QString& body)
    {
#ifdef QT_DBUS_LIB
        //
        // https://developer.gnome.org/notification-spec
        //
        QDBusMessage message(QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.Notifications"),
                                                            QLatin1String("/org/freedesktop/Notifications"),
                                                            QLatin1String("org.freedesktop.Notifications"),
                                                            QLatin1String("Notify")));
        message.setArguments({QLatin1String("Tremotesf"),
                              uint(0),
                              QLatin1String("tremotesf"),
                              summary,
                              body,
                              QVariant(QVariant::StringList),
                              QVariant(QVariant::Map),
                              -1});
        auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=]() {
            QDBusPendingReply<uint> reply(*watcher);
            if (reply.isError()) {
                qWarning() << reply.error();
                if (mTrayIcon->isVisible()) {
                    mTrayIcon->showMessage(summary, body, QSystemTrayIcon::Information, 0);
                }
            }
            watcher->deleteLater();
        });
#else
        if (mTrayIcon->isVisible()) {
            mTrayIcon->showMessage(summary, body, QSystemTrayIcon::Information, 0);
        }
#endif
    }

    void MainWindow::openTorrentsFiles()
    {
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
        for (const QModelIndex& index : selectedRows) {
            Utils::openFile(mRpc->localTorrentFilesPath(mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index))),
                            this);
        }
    }

    void MainWindow::showTorrentsInFileManager()
    {
        QStringList files;
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
        files.reserve(selectedRows.size());

        for (const QModelIndex& index : selectedRows) {
            libtremotesf::Torrent* torrent = mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index));
            files.push_back(mRpc->localTorrentFilesPath(torrent));
        }

        Utils::selectFilesInFileManager(files, this);
    }
}
