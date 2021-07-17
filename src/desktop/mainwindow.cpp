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
#include <QClipboard>
#include <QCloseEvent>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QScreen>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolBar>
#include <QWindow>

#ifdef QT_DBUS_LIB
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QX11Info>
#include <KStartupInfo>
#include "org.freedesktop.Notifications.h"
#endif

#include "../libtremotesf/serversettings.h"
#include "../libtremotesf/serverstats.h"
#include "../libtremotesf/torrent.h"
#include "../ipcclient.h"
#include "../ipcserver.h"
#include "../servers.h"
#include "../settings.h"
#include "../torrentsmodel.h"
#include "../torrentsproxymodel.h"
#include "../trpc.h"
#include "../utils.h"
#include "aboutdialog.h"
#include "addtorrentdialog.h"
#include "desktoputils.h"
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
                                       Rpc* rpc,
                                       QWidget* parent = nullptr)
                : QDialog(parent),
                  mDirectoryWidget(new RemoteDirectorySelectionWidget(downloadDirectory, rpc, true, this)),
                  mMoveFilesCheckBox(new QCheckBox(qApp->translate("tremotesf", "Move files from current directory"), this))
            {
                setWindowTitle(qApp->translate("tremotesf", "Set Location"));

                auto layout = new QVBoxLayout(this);
                layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

                auto label = new QLabel(qApp->translate("tremotesf", "Download directory:"), this);
                label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                layout->addWidget(label);

                mDirectoryWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                mDirectoryWidget->updateComboBox(downloadDirectory);
                layout->addWidget(mDirectoryWidget);

                mMoveFilesCheckBox->setChecked(true);
                layout->addWidget(mMoveFilesCheckBox);

                auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
                QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, [=] {
                    Servers::instance()->setCurrentServerAddTorrentDialogDirectories(mDirectoryWidget->textComboBoxItems());
                    accept();
                });
                QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SetLocationDialog::reject);

                QObject::connect(mDirectoryWidget, &FileSelectionWidget::textChanged, this, [=](const auto& text) {
                    dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
                });

                layout->addWidget(dialogButtonBox);

                setMinimumSize(minimumSizeHint());

                QObject::connect(rpc, &Rpc::connectedChanged, this, [=] {
                    if (!rpc->isConnected()) {
                        reject();
                    }
                });
            }

            QSize sizeHint() const override
            {
                return QDialog::sizeHint().expandedTo(QSize(320, 0));
            }

            QString downloadDirectory() const
            {
                return mDirectoryWidget->text();
            }

            bool moveFiles() const
            {
                return mMoveFilesCheckBox->isChecked();
            }

        private:
            RemoteDirectorySelectionWidget *const mDirectoryWidget;
            QCheckBox *const mMoveFilesCheckBox;
        };
    }

    MainWindow::MainWindow(IpcServer* ipcServer, const QStringList& files, const QStringList& urls)
        : mRpc(new Rpc(this)),
          mTorrentsModel(new TorrentsModel(mRpc, this)),
          mTorrentsProxyModel(new TorrentsProxyModel(mTorrentsModel, TorrentsModel::SortRole, this)),
          mSplitter(new QSplitter(this)),
          mSideBar(new MainWindowSideBar(mRpc, mTorrentsProxyModel)),
          mTorrentsView(new TorrentsView(mTorrentsProxyModel, this)),
          mTrayIcon(new QSystemTrayIcon(QIcon::fromTheme(QLatin1String("tremotesf-tray-icon"), windowIcon()), this))
    {
        setWindowTitle(QLatin1String(TREMOTESF_APP_NAME));
        setMinimumSize(minimumSizeHint().expandedTo(QSize(384, 256)));

        setContextMenuPolicy(Qt::NoContextMenu);
        setToolButtonStyle(Settings::instance()->toolButtonStyle());

        if (Servers::instance()->hasServers()) {
            mRpc->setServer(Servers::instance()->currentServer());
        }

        QObject::connect(Servers::instance(), &Servers::currentServerChanged, this, [this] {
            if (Servers::instance()->hasServers()) {
                mRpc->setServer(Servers::instance()->currentServer());
                mRpc->connect();
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
        QObject::connect(mTorrentsView, &TorrentsView::customContextMenuRequested, this, [=](auto point) {
            if (mTorrentsView->indexAt(point).isValid()) {
                mTorrentMenu->popup(QCursor::pos());
            }
        });
        QObject::connect(mTorrentsView, &TorrentsView::activated, this, &MainWindow::showTorrentsPropertiesDialogs);

        mSplitter->restoreState(Settings::instance()->splitterState());

        setCentralWidget(mSplitter);

        setupActions();
        setupToolBar();

        updateRpcActions();
        QObject::connect(mRpc, &Rpc::connectionStateChanged, this, &MainWindow::updateRpcActions);
        QObject::connect(Servers::instance(), &Servers::hasServersChanged, this, &MainWindow::updateRpcActions);

        updateTorrentActions();
        QObject::connect(mRpc, &Rpc::torrentsUpdated, this, &MainWindow::updateTorrentActions);
        QObject::connect(mTorrentsView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateTorrentActions);

        setStatusBar(new MainWindowStatusBar(mRpc));
        if (!Settings::instance()->isStatusBarVisible()) {
            statusBar()->hide();
        }

        setupMenuBar();
        setupTrayIcon();

        if (!restoreGeometry(Settings::instance()->mainWindowGeometry())) {
            const QSize screenSize(qApp->primaryScreen()->size());
            const QSize windowSize(sizeHint());
            move((screenSize.width() - windowSize.width()) / 2, (screenSize.height() - windowSize.height()) / 2);
        }

        if (!restoreState(Settings::instance()->mainWindowState())) {
            addToolBar(Qt::TopToolBarArea, mToolBar);
        }
        mToolBarAction->setChecked(!mToolBar->isHidden());

        QObject::connect(ipcServer, &IpcServer::windowActivationRequested, this, [=](const auto&, const auto& startupNoficationId) {
            showWindow(startupNoficationId);
        });

        QObject::connect(ipcServer, &IpcServer::torrentsAddingRequested, this, [=](const auto& files, const auto& urls) {
            if (mRpc->isConnected()) {
                const bool hidden = isHidden();
                showWindow();
                if (hidden) {
                    runAfterDelay([=] {
                        showAddTorrentFileDialogs(files);
                        showAddTorrentLinkDialogs(urls);
                    });
                } else {
                    showAddTorrentFileDialogs(files);
                    showAddTorrentLinkDialogs(urls);
                }
            }
        });

        QObject::connect(mRpc, &Rpc::connectedChanged, this, [=] {
            if (mRpc->isConnected()) {
                static bool first = true;
                if (first) {
                    if (!files.isEmpty() || !urls.isEmpty()) {
                        setWindowState(windowState() & ~Qt::WindowMinimized);
                        if (isHidden()) {
                            show();
                            runAfterDelay([=] {
                                showAddTorrentFileDialogs(files);
                                showAddTorrentLinkDialogs(urls);
                            });
                        } else {
                            showAddTorrentFileDialogs(files);
                            showAddTorrentLinkDialogs(urls);
                        }
                    }
                    first = false;
                }
            } else {
                if ((mRpc->error() != Rpc::Error::NoError) && Settings::instance()->notificationOnDisconnecting()) {
                    showNotification(qApp->translate("tremotesf", "Disconnected"), mRpc->statusString());
                }
            }
        });

        QObject::connect(mRpc, &Rpc::addedNotificationRequested, this, [=](const auto&, const auto& names) {
            showAddedNotification(names);
        });

        QObject::connect(mRpc, &Rpc::finishedNotificationRequested, this, [=](const auto&, const auto& names) {
            showFinishedNotification(names);
        });

        QObject::connect(mRpc, &Rpc::torrentAddDuplicate, this, [=] {
            QMessageBox::warning(this,
                                 qApp->translate("tremotesf", "Error adding torrent"),
                                 qApp->translate("tremotesf", "This torrent is already added"),
                                 QMessageBox::Close);
        });

        QObject::connect(mRpc, &Rpc::torrentAddError, this, [=] {
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
            runAfterDelay([=] {
                auto dialog = new ServerEditDialog(nullptr, -1, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerEditDialog::accepted, this, [=] {
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
        Settings::instance()->setMainWindowState(saveState());
        Settings::instance()->setSplitterState(mSplitter->saveState());

        mTrayIcon->hide();
    }

    QSize MainWindow::sizeHint() const
    {
        return minimumSizeHint().expandedTo(QSize(896, 640));
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
        mConnectionDependentActions.push_back(mAddTorrentFileAction);

        mAddTorrentLinkAction = new QAction(QIcon::fromTheme(QLatin1String("insert-link")), qApp->translate("tremotesf", "Add Torrent &Link..."), this);
        QObject::connect(mAddTorrentLinkAction, &QAction::triggered, this, [=] {
            setWindowState(windowState() & ~Qt::WindowMinimized);
            auto showDialog = [=] {
                auto dialog = new AddTorrentDialog(mRpc, QString(), AddTorrentDialog::Mode::Url, this);
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
        mConnectionDependentActions.push_back(mAddTorrentLinkAction);

        //
        // Torrent menu
        //
        mTorrentMenu = new QMenu(qApp->translate("tremotesf", "&Torrent"), this);

        QAction* torrentPropertiesAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("document-properties")), qApp->translate("tremotesf", "&Properties"));
        QObject::connect(torrentPropertiesAction, &QAction::triggered, this, &MainWindow::showTorrentsPropertiesDialogs);

        mTorrentMenu->addSeparator();

        mStartTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("media-playback-start")), qApp->translate("tremotesf", "&Start"));
        QObject::connect(mStartTorrentAction, &QAction::triggered, this, [=] {
            mRpc->startTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mStartTorrentNowAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("media-playback-start")), qApp->translate("tremotesf", "Start &Now"));
        QObject::connect(mStartTorrentNowAction, &QAction::triggered, this, [=] {
            mRpc->startTorrentsNow(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mPauseTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("media-playback-pause")), qApp->translate("tremotesf", "P&ause"));
        QObject::connect(mPauseTorrentAction, &QAction::triggered, this, [=] {
            mRpc->pauseTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mTorrentMenu->addSeparator();

        QAction* copyMagnetLinkAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("edit-copy")), qApp->translate("tremotesf", "Copy &Magnet Link"));
        QObject::connect(copyMagnetLinkAction, &QAction::triggered, this, [=] {
            const auto indexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows()));
            QStringList links;
            links.reserve(indexes.size());
            for (const auto& index : indexes) {
                links.push_back(mTorrentsModel->torrentAtIndex(index)->data().magnetLink);
            }
            qApp->clipboard()->setText(links.join(QLatin1Char('\n')));
        });

        mTorrentMenu->addSeparator();

        mRemoveTorrentAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("edit-delete")), qApp->translate("tremotesf", "&Delete"));
        mRemoveTorrentAction->setShortcut(QKeySequence::Delete);
        QObject::connect(mRemoveTorrentAction, &QAction::triggered, this, [=] { removeSelectedTorrents(false); });

        const auto removeTorrentWithFilesShortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Delete), this);
        QObject::connect(removeTorrentWithFilesShortcut, &QShortcut::activated, this, [=] { removeSelectedTorrents(true); });

        QAction* setLocationAction = mTorrentMenu->addAction(qApp->translate("tremotesf", "Set &Location"));
        QObject::connect(setLocationAction, &QAction::triggered, this, [=] {
            if (mTorrentsView->selectionModel()->hasSelection()) {
                QModelIndexList indexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows()));
                auto dialog = new SetLocationDialog(mTorrentsModel->torrentAtIndex(indexes.first())->downloadDirectory(),
                                                    mRpc,
                                                    this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &SetLocationDialog::accepted, this, [=] {
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
        QObject::connect(checkTorrentAction, &QAction::triggered, this, [=] {
            mRpc->checkTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* reannounceAction = mTorrentMenu->addAction(QIcon::fromTheme(QLatin1String("view-refresh")), qApp->translate("tremotesf", "Reanno&unce"));
        QObject::connect(reannounceAction, &QAction::triggered, this, [=] {
            mRpc->reannounceTorrents(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        mTorrentMenu->addSeparator();

        QMenu* queueMenu = mTorrentMenu->addMenu(qApp->translate("tremotesf", "&Queue"));

        QAction* moveTorrentToTopAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-top")), qApp->translate("tremotesf", "Move To &Top"));
        QObject::connect(moveTorrentToTopAction, &QAction::triggered, this, [=] {
            mRpc->moveTorrentsToTop(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* moveTorrentUpAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-up")), qApp->translate("tremotesf", "Move &Up"));
        QObject::connect(moveTorrentUpAction, &QAction::triggered, this, [=] {
            mRpc->moveTorrentsUp(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* moveTorrentDownAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-down")), qApp->translate("tremotesf", "Move &Down"));
        QObject::connect(moveTorrentDownAction, &QAction::triggered, this, [=] {
            mRpc->moveTorrentsDown(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });

        QAction* moveTorrentToBottomAction = queueMenu->addAction(QIcon::fromTheme(QLatin1String("go-bottom")), qApp->translate("tremotesf", "Move To &Bottom"));
        QObject::connect(moveTorrentToBottomAction, &QAction::triggered, this, [=] {
            mRpc->moveTorrentsToBottom(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        });
    }

    void MainWindow::updateRpcActions()
    {
        if (Servers::instance()->hasServers()) {
            const bool disconnected = mRpc->connectionState() == Rpc::ConnectionState::Disconnected;
            mConnectAction->setEnabled(disconnected);
            mDisconnectAction->setEnabled(!disconnected);
        } else {
            mConnectAction->setEnabled(false);
            mDisconnectAction->setEnabled(false);
        }

        const bool connected = mRpc->isConnected();
        for (auto action : mConnectionDependentActions) {
            action->setEnabled(connected);
        }
    }

    void MainWindow::addTorrentsFiles()
    {
        setWindowState(windowState() & ~Qt::WindowMinimized);

        auto showDialog = [=] {
            auto fileDialog = new QFileDialog(this,
                                              qApp->translate("tremotesf", "Select Files"),
                                              QString(),
                                              qApp->translate("tremotesf", "Torrent Files (*.torrent)"));
            fileDialog->setAttribute(Qt::WA_DeleteOnClose);
            fileDialog->setFileMode(QFileDialog::ExistingFiles);

            QObject::connect(fileDialog, &QFileDialog::accepted, this, [=] {
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
        for (const QString& filePath : files) {
            auto dialog = new AddTorrentDialog(mRpc, filePath, AddTorrentDialog::Mode::File, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->activateWindow();
        }
    }

    void MainWindow::showAddTorrentLinkDialogs(const QStringList& urls)
    {
        for (const QString& url : urls) {
            auto dialog = new AddTorrentDialog(mRpc, url, AddTorrentDialog::Mode::Url, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->activateWindow();
        }
    }

    void MainWindow::updateTorrentActions()
    {
        if (mTorrentsView->selectionModel()->hasSelection()) {
            const QList<QAction*> actions(mTorrentMenu->actions());
            for (QAction* action : actions) {
                action->setEnabled(true);
            }
            const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
            if (selectedRows.size() == 1) {
                switch (mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(selectedRows.first()))->status()) {
                case libtremotesf::TorrentData::Paused:
                case libtremotesf::TorrentData::Errored:
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
            const QList<QAction*> actions(mTorrentMenu->actions());
            for (QAction* action : actions) {
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
                QObject::connect(dialog, &TorrentPropertiesDialog::finished, this, [=] {
                    mTorrentsDialogs.erase(mTorrentsDialogs.find(id));
                });

                dialog->show();
            }
        }
    }

    void MainWindow::removeSelectedTorrents(bool deleteFiles)
    {
        const QVariantList ids(mTorrentsModel->idsFromIndexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())));
        if (ids.isEmpty()) {
            return;
        }

        QMessageBox dialog(this);
        dialog.setIcon(QMessageBox::Warning);

        dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialog.setDefaultButton(QMessageBox::Cancel);
        const auto okButton = dialog.button(QMessageBox::Ok);
        if (dialog.style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
            okButton->setIcon(QIcon::fromTheme(QLatin1String("edit-delete")));
        }

        QCheckBox deleteFilesCheckBox(qApp->translate("tremotesf", "Also delete the files on the hard disk"));
        deleteFilesCheckBox.setChecked(deleteFiles);
        dialog.setCheckBox(&deleteFilesCheckBox);

        auto setRemoveText = [&] {
            okButton->setText(deleteFilesCheckBox.isChecked() ? qApp->translate("tremotesf", "Delete with files")
                                                              : qApp->translate("tremotesf", "Delete"));
        };
        setRemoveText();
        QObject::connect(&deleteFilesCheckBox, &QCheckBox::toggled, this, setRemoveText);

        if (ids.size() == 1) {
            dialog.setWindowTitle(qApp->translate("tremotesf", "Delete Torrent"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to delete this torrent?"));
        } else {
            dialog.setWindowTitle(qApp->translate("tremotesf", "Delete Torrents"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to delete %Ln selected torrents?", nullptr, ids.size()));
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
        QObject::connect(invertSelectionAction, &QAction::triggered, this, [=] {
            mTorrentsView->selectionModel()->select(QItemSelection(mTorrentsProxyModel->index(0, 0),
                                                                   mTorrentsProxyModel->index(mTorrentsProxyModel->rowCount() - 1,
                                                                                              mTorrentsProxyModel->columnCount() - 1)),
                                                    QItemSelectionModel::Toggle);
        });

        menuBar()->addMenu(mTorrentMenu);

        QMenu* viewMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&View"));

        mToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Toolbar"));
        mToolBarAction->setCheckable(true);
        QObject::connect(mToolBarAction, &QAction::triggered, mToolBar, &QToolBar::setVisible);

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

        viewMenu->addSeparator();
        QAction* lockToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Lock Toolbar"));
        lockToolBarAction->setCheckable(true);
        lockToolBarAction->setChecked(!mToolBar->isMovable());
        QObject::connect(lockToolBarAction, &QAction::triggered, mToolBar, [=](bool checked) {
            mToolBar->setMovable(!checked);
            Settings::instance()->setToolBarLocked(checked);
        });

        QMenu* toolsMenu = menuBar()->addMenu(qApp->translate("tremotesf", "T&ools"));

        QAction* settingsAction = toolsMenu->addAction(QIcon::fromTheme(QLatin1String("configure"), QIcon::fromTheme(QLatin1String("preferences-system"))), qApp->translate("tremotesf", "&Options"));
        settingsAction->setShortcut(QKeySequence::Preferences);
        QObject::connect(settingsAction, &QAction::triggered, this, [=] {
            static SettingsDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new SettingsDialog(this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &SettingsDialog::destroyed, this, [] {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });

        QAction* serversAction = toolsMenu->addAction(qApp->translate("tremotesf", "&Servers"));
        QObject::connect(serversAction, &QAction::triggered, this, [=] {
            static ServersDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new ServersDialog(this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServersDialog::destroyed, this, [] {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });

        toolsMenu->addSeparator();

        auto serverSettingsAction = new QAction(QIcon::fromTheme(QLatin1String("configure"), QIcon::fromTheme(QLatin1String("preferences-system"))), qApp->translate("tremotesf", "&Server Options"), this);
        QObject::connect(serverSettingsAction, &QAction::triggered, this, [=] {
            static ServerSettingsDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new ServerSettingsDialog(mRpc, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerSettingsDialog::destroyed, this, [] {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });
        mConnectionDependentActions.push_back(serverSettingsAction);
        toolsMenu->addAction(serverSettingsAction);

        auto serverStatsAction = new QAction(qApp->translate("tremotesf", "Server S&tats"), this);
        QObject::connect(serverStatsAction, &QAction::triggered, this, [=] {
            static ServerStatsDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new ServerStatsDialog(mRpc, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerSettingsDialog::destroyed, this, [] {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });
        mConnectionDependentActions.push_back(serverStatsAction);
        toolsMenu->addAction(serverStatsAction);

        auto shutdownServerAction = new QAction(QIcon::fromTheme("system-shutdown"), qApp->translate("tremotesf", "S&hutdown Server"), this);
        QObject::connect(shutdownServerAction, &QAction::triggered, this, [=] {
            auto dialog = new QMessageBox(QMessageBox::Warning,
                                          qApp->translate("tremotesf", "Shutdown Server"),
                                          qApp->translate("tremotesf", "Are you sure you want to shutdown remote Transmission instance?"),
                                          QMessageBox::Cancel | QMessageBox::Ok,
                                          this);
            auto okButton = dialog->button(QMessageBox::Ok);
            okButton->setIcon(QIcon::fromTheme("system-shutdown"));
            okButton->setText(qApp->translate("tremotesf", "Shutdown"));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setModal(true);
            QObject::connect(dialog, &QDialog::accepted, this, [=] { mRpc->shutdownServer(); } );
            dialog->show();
        });
        mConnectionDependentActions.push_back(shutdownServerAction);
        toolsMenu->addAction(shutdownServerAction);

        QMenu* helpMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&Help"));

        QAction* aboutAction = helpMenu->addAction(QIcon::fromTheme(QLatin1String("help-about")), qApp->translate("tremotesf", "&About"));
        QObject::connect(aboutAction, &QAction::triggered, this, [=] {
            static AboutDialog* dialog = nullptr;
            if (dialog) {
                dialog->raise();
                dialog->activateWindow();
            } else {
                dialog = new AboutDialog(this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &AboutDialog::destroyed, this, [] {
                    dialog = nullptr;
                });
                dialog->show();
            }
        });
    }

    void MainWindow::setupToolBar()
    {
        mToolBar = new QToolBar(this);
        mToolBar->setObjectName(QLatin1String("toolBar"));
        mToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
        mToolBar->setMovable(!Settings::instance()->isToolBarLocked());
        addToolBar(mToolBar);

        mToolBar->addAction(mConnectAction);
        mToolBar->addAction(mDisconnectAction);
        mToolBar->addSeparator();
        mToolBar->addAction(mAddTorrentFileAction);
        mToolBar->addAction(mAddTorrentLinkAction);
        mToolBar->addSeparator();
        mToolBar->addAction(mStartTorrentAction);
        mToolBar->addAction(mPauseTorrentAction);
        mToolBar->addAction(mRemoveTorrentAction);

        QObject::connect(mToolBar, &QToolBar::customContextMenuRequested, this, [=] {
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
        auto contextMenu = new QMenu(mFileMenu);
        const QList<QAction*> actions(mFileMenu->actions());
        for (QAction* action : actions) {
            contextMenu->addAction(action);
        }

        mTrayIcon->setContextMenu(contextMenu);
        mTrayIcon->setToolTip(mRpc->statusString());

        QObject::connect(mTrayIcon, &QSystemTrayIcon::activated, this, [=](auto reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                if (isHidden() || isMinimized()) {
                    showWindow();
                } else {
                    hideWindow();
                }
            }
        });

        QObject::connect(mRpc, &Rpc::statusStringChanged, this, [=] {
            mTrayIcon->setToolTip(mRpc->statusString());
        });

        QObject::connect(mRpc->serverStats(), &libtremotesf::ServerStats::updated, this, [=] {
            mTrayIcon->setToolTip(QString(u8"\u25be %1\n\u25b4 %2")
                                      .arg(Utils::formatByteSpeed(mRpc->serverStats()->downloadSpeed()),
                                           Utils::formatByteSpeed(mRpc->serverStats()->uploadSpeed())));
        });

        if (Settings::instance()->showTrayIcon()) {
            mTrayIcon->show();
        }

        QObject::connect(Settings::instance(), &Settings::showTrayIconChanged, this, [=] {
            if (Settings::instance()->showTrayIcon()) {
                mTrayIcon->show();
            } else {
                mTrayIcon->hide();
                showWindow();
            }
        });
    }

    void MainWindow::showWindow(const QByteArray& newStartupNotificationId)
    {
#ifdef QT_DBUS_LIB
        if (!newStartupNotificationId.isEmpty()) {
            if (isHidden() && QX11Info::isPlatformX11()) {
                QX11Info::setNextStartupId(newStartupNotificationId);
            } else {
                KStartupInfo::appStarted(newStartupNotificationId);
            }
        }
#else
        Q_UNUSED(newStartupNotificationId)
#endif

        setWindowState(windowState() & ~Qt::WindowMinimized);
        show();
        raise();
        for (QWindow* window : qApp->topLevelWindows()) {
            if (window->type() == Qt::Dialog) {
                window->show();
            }
        }
        activateWindow();
    }

    void MainWindow::hideWindow()
    {
        // Hide main window and dialogs
        hide();
        for (QWindow* window : qApp->topLevelWindows()) {
            if (window->isVisible() && window->type() == Qt::Dialog) {
                window->hide();
            }
        }
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
                                                   : qApp->translate("tremotesf", "%Ln torrents finished", nullptr, names.size()),
                                 names);
    }

    void MainWindow::showAddedNotification(const QStringList& names)
    {
        showTorrentsNotification(names.size() == 1 ? qApp->translate("tremotesf", "Torrent added")
                                                   : qApp->translate("tremotesf", "%Ln torrents added", nullptr, names.size()),
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
        setupNotificationsInterface();
        const QDBusPendingCall call(mNotificationsInterface->Notify(QLatin1String(TREMOTESF_APP_NAME),
                                                                    0,
                                                                    QLatin1String(TREMOTESF_APP_ID),
                                                                    summary,
                                                                    body,
                                                                    {QLatin1String("default"), qApp->translate("tremotesf", "Show Tremotesf")},
                                                                    {{QLatin1String("desktop-entry"), QLatin1String(TREMOTESF_APP_ID)},
                                                                     {QLatin1String("x-kde-origin-name"), Servers::instance()->currentServerName()}},
                                                                    -1));
        auto watcher = new QDBusPendingCallWatcher(call, this);
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] {
            QDBusPendingReply<quint32> reply(*watcher);
            if (reply.isError()) {
                qWarning() << "Notify error" << reply.error();
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

    void MainWindow::setupNotificationsInterface()
    {
#ifdef QT_DBUS_LIB
        if (!mNotificationsInterface) {
            mNotificationsInterface = new OrgFreedesktopNotificationsInterface(QLatin1String("org.freedesktop.Notifications"),
                                                                               QLatin1String("/org/freedesktop/Notifications"),
                                                                               QDBusConnection::sessionBus(),
                                                                               this);
            mNotificationsInterface->setTimeout(desktoputils::defaultDbusTimeout);
            QObject::connect(mNotificationsInterface, &OrgFreedesktopNotificationsInterface::ActionInvoked, this, [=] { showWindow(); });
        }
#endif
    }

    void MainWindow::openTorrentsFiles()
    {
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
        for (const QModelIndex& index : selectedRows) {
            desktoputils::openFile(mRpc->localTorrentFilesPath(mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index))),
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

        desktoputils::selectFilesInFileManager(files, this);
    }
}
