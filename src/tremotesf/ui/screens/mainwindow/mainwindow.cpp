// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <algorithm>
#include <functional>
#include <cmath>

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
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QScreen>
#include <QSplitter>
#include <QStackedLayout>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolBar>
#include <QWindow>

#include <KMessageWidget>

#ifdef TREMOTESF_UNIX_FREEDESKTOP
#    include <KStartupInfo>
#endif

#include "libtremotesf/log.h"
#include "libtremotesf/serverstats.h"
#include "libtremotesf/target_os.h"
#include "libtremotesf/torrent.h"

#include "tremotesf/ipc/ipcserver.h"
#include "tremotesf/rpc/servers.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/filemanagerlauncher.h"
#include "tremotesf/desktoputils.h"
#include "tremotesf/settings.h"
#include "tremotesf/utils.h"

#include "tremotesf/ui/notificationscontroller.h"
#include "tremotesf/ui/screens/aboutdialog.h"
#include "tremotesf/ui/screens/addtorrent/addtorrentdialog.h"
#include "tremotesf/ui/screens/connectionsettings/servereditdialog.h"
#include "tremotesf/ui/screens/connectionsettings/connectionsettingsdialog.h"
#include "tremotesf/ui/screens/serversettings/serversettingsdialog.h"
#include "tremotesf/ui/screens/serverstatsdialog.h"
#include "tremotesf/ui/screens/settingsdialog.h"
#include "tremotesf/ui/screens/torrentproperties/torrentpropertiesdialog.h"
#include "tremotesf/ui/widgets/remotedirectoryselectionwidget.h"
#include "tremotesf/ui/widgets/torrentfilesview.h"

#include "torrentsmodel.h"
#include "torrentsproxymodel.h"
#include "mainwindowsidebar.h"
#include "mainwindowstatusbar.h"
#include "torrentsview.h"
#include "mainwindowviewmodel.h"

namespace tremotesf {
    namespace {
        class SetLocationDialog : public QDialog {
            Q_OBJECT

        public:
            explicit SetLocationDialog(const QString& downloadDirectory, Rpc* rpc, QWidget* parent = nullptr)
                : QDialog(parent),
                  mDirectoryWidget(new TorrentDownloadDirectoryDirectorySelectionWidget(downloadDirectory, rpc, this)),
                  mMoveFilesCheckBox(
                      //: Check box label
                      new QCheckBox(qApp->translate("tremotesf", "Move files from current directory"), this)
                  ) {
                //: Dialog title for changing torrent's download directory
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
                QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, [=, this] {
                    mDirectoryWidget->saveDirectories();
                    accept();
                });
                QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SetLocationDialog::reject);

                QObject::connect(mDirectoryWidget, &DirectorySelectionWidget::pathChanged, this, [=, this] {
                    dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!mDirectoryWidget->path().isEmpty());
                });

                layout->addWidget(dialogButtonBox);

                setMinimumSize(minimumSizeHint());

                QObject::connect(rpc, &Rpc::connectedChanged, this, [=, this] {
                    if (!rpc->isConnected()) {
                        reject();
                    }
                });
            }

            [[nodiscard]] QSize sizeHint() const override { return QDialog::sizeHint().expandedTo(QSize(320, 0)); }

            [[nodiscard]] QString downloadDirectory() const { return mDirectoryWidget->path(); }

            [[nodiscard]] bool moveFiles() const { return mMoveFilesCheckBox->isChecked(); }

        private:
            TorrentDownloadDirectoryDirectorySelectionWidget* const mDirectoryWidget;
            QCheckBox* const mMoveFilesCheckBox;
        };

        constexpr auto kdePlatformFileDialogClassName = "KDEPlatformFileDialog";

        [[nodiscard]] std::vector<QPointer<QWidget>> toQPointers(QWidgetList&& widgets) {
            return {widgets.begin(), widgets.end()};
        }

        void showAndRaiseWindow(QWidget* window, bool activate = true) {
            if (window->isHidden()) {
                window->show();
            }
            if (window->isMinimized()) {
                window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
            }
            window->raise();
            if (activate) {
                window->activateWindow();
            }
        }

        template<std::derived_from<QDialog> Dialog, typename CreateDialogFunction>
            requires std::is_invocable_r_v<Dialog*, CreateDialogFunction>
        void showSingleInstanceDialog(QWidget* mainWindow, CreateDialogFunction&& createDialog) {
            auto existingDialog = mainWindow->findChild<Dialog*>({}, Qt::FindDirectChildrenOnly);
            if (existingDialog) {
                showAndRaiseWindow(existingDialog);
            } else {
                auto dialog = createDialog();
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            }
        }
    }

    MainWindow::MainWindow(
        QStringList&& commandLineFiles, QStringList&& commandLineUrls, IpcServer* ipcServer, QWidget* parent
    )
        : QMainWindow(parent),
          mRpc(new Rpc(this)),
          mViewModel(
              new MainWindowViewModel(std::move(commandLineFiles), std::move(commandLineUrls), mRpc, ipcServer, this)
          ),
          mTorrentsModel(new TorrentsModel(mRpc, this)),
          mTorrentsProxyModel(new TorrentsProxyModel(mTorrentsModel, static_cast<int>(TorrentsModel::Role::Sort), this)
          ),
          mSplitter(new QSplitter(this)),
          mSideBar(new MainWindowSideBar(mRpc, mTorrentsProxyModel)),
          mTorrentsView(new TorrentsView(mTorrentsProxyModel, this)),
          mTrayIcon(new QSystemTrayIcon(QIcon::fromTheme("tremotesf-tray-icon"_l1, windowIcon()), this)),
          mNotificationsController(NotificationsController::createInstance(mTrayIcon, this)) {
        setWindowTitle(TREMOTESF_APP_NAME ""_l1);
        setMinimumSize(minimumSizeHint().expandedTo(QSize(384, 256)));

        setContextMenuPolicy(Qt::NoContextMenu);
        setToolButtonStyle(Settings::instance()->toolButtonStyle());

        setAcceptDrops(true);

        if (Servers::instance()->hasServers()) {
            mRpc->setConnectionConfiguration(Servers::instance()->currentServer().connectionConfiguration);
        }

        QObject::connect(Servers::instance(), &Servers::currentServerChanged, this, [this] {
            if (Servers::instance()->hasServers()) {
                mRpc->setConnectionConfiguration(Servers::instance()->currentServer().connectionConfiguration);
                mRpc->connect();
            } else {
                mRpc->resetConnectionConfiguration();
            }
        });

        mSplitter->setChildrenCollapsible(false);

        if (!Settings::instance()->isSideBarVisible()) {
            mSideBar->hide();
        }
        mSplitter->addWidget(mSideBar);

        auto mainWidgetContainer = new QWidget(this);
        mSplitter->addWidget(mainWidgetContainer);
        mSplitter->setStretchFactor(1, 1);
        auto mainWidgetLayout = new QVBoxLayout(mainWidgetContainer);
        mainWidgetLayout->setContentsMargins(
            0,
            mainWidgetLayout->contentsMargins().top(),
            0,
            mainWidgetLayout->contentsMargins().bottom()
        );

        auto messageWidget = new KMessageWidget(this);
        messageWidget->setWordWrap(true);
        messageWidget->hide();
        mainWidgetLayout->addWidget(messageWidget);

        auto torrentsViewContainer = new QWidget(this);
        mainWidgetLayout->addWidget(torrentsViewContainer);
        auto torrentsViewLayout = new QStackedLayout(torrentsViewContainer);
        torrentsViewLayout->setStackingMode(QStackedLayout::StackAll);

        torrentsViewLayout->addWidget(mTorrentsView);
        QObject::connect(mTorrentsView, &TorrentsView::customContextMenuRequested, this, [=, this](auto point) {
            if (mTorrentsView->indexAt(point).isValid()) {
                mTorrentMenu->popup(QCursor::pos());
            }
        });
        QObject::connect(mTorrentsView, &TorrentsView::activated, this, &MainWindow::showTorrentsPropertiesDialogs);

        setupTorrentsPlaceholder(torrentsViewLayout);

        mSplitter->restoreState(Settings::instance()->splitterState());

        setCentralWidget(mSplitter);

        setupActions();
        setupToolBar();

        updateRpcActions();
        QObject::connect(mRpc, &Rpc::connectionStateChanged, this, &MainWindow::updateRpcActions);
        QObject::connect(Servers::instance(), &Servers::hasServersChanged, this, &MainWindow::updateRpcActions);

        updateTorrentActions();
        QObject::connect(mRpc, &Rpc::torrentsUpdated, this, &MainWindow::updateTorrentActions);
        QObject::connect(
            mTorrentsView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::updateTorrentActions
        );

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

        QObject::connect(mViewModel, &MainWindowViewModel::showWindow, this, &MainWindow::showWindow);
        QObject::connect(
            mViewModel,
            &MainWindowViewModel::showAddTorrentDialogs,
            this,
            [=, this](const auto& files, const auto& urls) {
                if (messageWidget->isVisible()) {
                    messageWidget->animatedHide();
                }
                const bool wasHidden = isHidden();
                showWindow();
                if (wasHidden) {
                    runAfterDelay([=, this] {
                        showAddTorrentFileDialogs(files);
                        showAddTorrentLinkDialogs(urls);
                    });
                } else {
                    showAddTorrentFileDialogs(files);
                    showAddTorrentLinkDialogs(urls);
                }
            }
        );

        QObject::connect(
            mViewModel,
            &MainWindowViewModel::showDelayedTorrentAddMessage,
            this,
            [messageWidget](const QStringList& torrents) {
                logDebug("MainWindow: showing delayed torrent add message");
                messageWidget->setMessageType(KMessageWidget::Information);
                //: Message shown when user attempts to add torrent while disconnect from server. After that will be list of added torrents
                QString text = qApp->translate("tremotesf", "Torrents will be added after connection to server:");
                constexpr QStringList::size_type maxCount = 5;
                const auto count = std::min(torrents.size(), maxCount);
                const auto subList = torrents.mid(0, count);
                for (const auto& torrent : subList) {
                    text += "\n \u2022 ";
                    text += torrent;
                }
                if (auto remaining = torrents.size() - count; remaining > 0) {
                    text += "\n \u2022 ";
                    //: Shown when list of items exceeds maximum size. %n is a number of remaining items
                    text += qApp->translate("tremotesf", "And %n more", nullptr, remaining);
                }
                messageWidget->setText(text);
                messageWidget->animatedShow();
            }
        );

        QObject::connect(mRpc, &Rpc::connectedChanged, this, [=, this] {
            if (!mRpc->isConnected()) {
                if ((mRpc->error() != Rpc::Error::NoError) && Settings::instance()->notificationOnDisconnecting()) {
                    mNotificationsController->showNotification(
                        //: Notification title when disconnected from server
                        qApp->translate("tremotesf", "Disconnected"),
                        mRpc->statusString()
                    );
                }
            }
        });

        auto pasteShortcut = new QShortcut(QKeySequence::Paste, this);
        QObject::connect(
            pasteShortcut,
            &QShortcut::activated,
            mViewModel,
            &MainWindowViewModel::pasteShortcutActivated
        );

        QObject::connect(mRpc, &Rpc::addedNotificationRequested, this, [=, this](const auto&, const auto& names) {
            mNotificationsController->showAddedTorrentsNotification(names);
        });

        QObject::connect(mRpc, &Rpc::finishedNotificationRequested, this, [=, this](const auto&, const auto& names) {
            mNotificationsController->showFinishedTorrentsNotification(names);
        });

        QObject::connect(mRpc, &Rpc::torrentAddDuplicate, this, [=, this] {
            QMessageBox::warning(
                this,
                qApp->translate("tremotesf", "Error adding torrent"),
                qApp->translate("tremotesf", "This torrent is already added"),
                QMessageBox::Close
            );
        });

        QObject::connect(mRpc, &Rpc::torrentAddError, this, [=, this] {
            QMessageBox::warning(
                this,
                qApp->translate("tremotesf", "Error adding torrent"),
                qApp->translate("tremotesf", "Error adding torrent"),
                QMessageBox::Close
            );
        });

        QObject::connect(mNotificationsController, &NotificationsController::notificationClicked, this, [this] {
            showWindow();
        });

        if (Servers::instance()->hasServers()) {
            if (Settings::instance()->connectOnStartup()) {
                mRpc->connect();
            }
        } else {
            runAfterDelay([=, this] {
                auto dialog = new ServerEditDialog(nullptr, -1, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &ServerEditDialog::accepted, this, [=, this] {
                    if (Settings::instance()->connectOnStartup()) {
                        mRpc->connect();
                    }
                });
                dialog->open();
            });
        }
    }

    MainWindow::~MainWindow() {
        mRpc->disconnect();

        Settings::instance()->setMainWindowGeometry(saveGeometry());
        Settings::instance()->setMainWindowState(saveState());
        Settings::instance()->setSplitterState(mSplitter->saveState());

        mTrayIcon->hide();
    }

    QSize MainWindow::sizeHint() const { return minimumSizeHint().expandedTo(QSize(896, 640)); }

    void MainWindow::showMinimized(bool minimized) {
        if (!(minimized && Settings::instance()->showTrayIcon() && QSystemTrayIcon::isSystemTrayAvailable())) {
            show();
        }
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        QMainWindow::closeEvent(event);
        if (!(mTrayIcon->isVisible() && QSystemTrayIcon::isSystemTrayAvailable())) {
            qApp->quit();
        }
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) { MainWindowViewModel::processDragEnterEvent(event); }

    void MainWindow::dropEvent(QDropEvent* event) { mViewModel->processDropEvent(event); }

    void MainWindow::setupActions() {
        //: Button / menu item to connect to server
        mConnectAction = new QAction(qApp->translate("tremotesf", "&Connect"), this);
        QObject::connect(mConnectAction, &QAction::triggered, mRpc, &Rpc::connect);

        //: Button / menu item to disconnect from server
        mDisconnectAction = new QAction(qApp->translate("tremotesf", "&Disconnect"), this);
        QObject::connect(mDisconnectAction, &QAction::triggered, mRpc, &Rpc::disconnect);

        const QIcon connectIcon(QIcon::fromTheme("network-connect"_l1));
        const QIcon disconnectIcon(QIcon::fromTheme("network-disconnect"_l1));
        if (connectIcon.name() != disconnectIcon.name()) {
            mConnectAction->setIcon(connectIcon);
            mDisconnectAction->setIcon(disconnectIcon);
        }

        mAddTorrentFileAction = new QAction(
            QIcon::fromTheme("list-add"_l1),
            //: Menu item
            qApp->translate("tremotesf", "&Add Torrent File..."),
            this
        );
        mAddTorrentFileAction->setShortcuts(QKeySequence::Open);
        QObject::connect(mAddTorrentFileAction, &QAction::triggered, this, &MainWindow::addTorrentsFiles);
        mConnectionDependentActions.push_back(mAddTorrentFileAction);

        mAddTorrentLinkAction = new QAction(
            QIcon::fromTheme("insert-link"_l1),
            //: Menu item
            qApp->translate("tremotesf", "Add Torrent &Link..."),
            this
        );
        QObject::connect(mAddTorrentLinkAction, &QAction::triggered, this, [=, this] {
            setWindowState(windowState() & ~Qt::WindowMinimized);
            auto showDialog = [=, this] {
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
        //: Menu bar item
        mTorrentMenu = new QMenu(qApp->translate("tremotesf", "&Torrent"), this);

        QAction* torrentPropertiesAction = mTorrentMenu->addAction(
            QIcon::fromTheme("document-properties"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Properties")
        );
        QObject::connect(
            torrentPropertiesAction,
            &QAction::triggered,
            this,
            &MainWindow::showTorrentsPropertiesDialogs
        );

        mTorrentMenu->addSeparator();

        mStartTorrentAction = mTorrentMenu->addAction(
            QIcon::fromTheme("media-playback-start"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Start")
        );
        QObject::connect(mStartTorrentAction, &QAction::triggered, this, [=, this] {
            mRpc->startTorrents(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        mStartTorrentNowAction = mTorrentMenu->addAction(
            QIcon::fromTheme("media-playback-start"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Start &Now")
        );
        QObject::connect(mStartTorrentNowAction, &QAction::triggered, this, [=, this] {
            mRpc->startTorrentsNow(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        mPauseTorrentAction = mTorrentMenu->addAction(
            QIcon::fromTheme("media-playback-pause"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "P&ause")
        );
        QObject::connect(mPauseTorrentAction, &QAction::triggered, this, [=, this] {
            mRpc->pauseTorrents(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        mTorrentMenu->addSeparator();

        QAction* copyMagnetLinkAction = mTorrentMenu->addAction(
            QIcon::fromTheme("edit-copy"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Copy &Magnet Link")
        );
        QObject::connect(copyMagnetLinkAction, &QAction::triggered, this, [=, this] {
            const auto indexes(mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows()));
            QStringList links;
            links.reserve(indexes.size());
            for (const auto& index : indexes) {
                links.push_back(mTorrentsModel->torrentAtIndex(index)->data().magnetLink);
            }
            qApp->clipboard()->setText(links.join('\n'));
        });

        mTorrentMenu->addSeparator();

        mRemoveTorrentAction = mTorrentMenu->addAction(
            QIcon::fromTheme("edit-delete"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Delete")
        );
        mRemoveTorrentAction->setShortcut(QKeySequence::Delete);
        QObject::connect(mRemoveTorrentAction, &QAction::triggered, this, [=, this] { removeSelectedTorrents(false); });

        const auto removeTorrentWithFilesShortcut =
            new QShortcut(QKeySequence(static_cast<int>(Qt::SHIFT) | static_cast<int>(Qt::Key_Delete)), this);
        QObject::connect(removeTorrentWithFilesShortcut, &QShortcut::activated, this, [=, this] {
            removeSelectedTorrents(true);
        });

        QAction* setLocationAction = mTorrentMenu->addAction(
            QIcon::fromTheme("mark-location"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Set &Location")
        );
        QObject::connect(setLocationAction, &QAction::triggered, this, [=, this] {
            if (mTorrentsView->selectionModel()->hasSelection()) {
                QModelIndexList indexes(
                    mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
                );
                auto dialog = new SetLocationDialog(
                    mTorrentsModel->torrentAtIndex(indexes.first())->data().downloadDirectory,
                    mRpc,
                    this
                );
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(dialog, &SetLocationDialog::accepted, this, [=, this] {
                    mRpc->setTorrentsLocation(
                        mTorrentsModel->idsFromIndexes(indexes),
                        dialog->downloadDirectory(),
                        dialog->moveFiles()
                    );
                });
                dialog->show();
            }
        });

        mRenameTorrentAction = mTorrentMenu->addAction(
            QIcon::fromTheme("edit-rename"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Rename")
        );
        QObject::connect(mRenameTorrentAction, &QAction::triggered, this, [=, this] {
            const auto indexes = mTorrentsView->selectionModel()->selectedRows();
            if (indexes.size() == 1) {
                const auto torrent = mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(indexes.first()));
                const auto id = torrent->data().id;
                const auto name = torrent->data().name;
                TorrentFilesView::showFileRenameDialog(name, this, [=, this](const auto& newName) {
                    mRpc->renameTorrentFile(id, name, newName);
                });
            }
        });

        mTorrentMenu->addSeparator();

        mOpenTorrentFilesAction = mTorrentMenu->addAction(
            QIcon::fromTheme("document-open"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Open")
        );
        QObject::connect(mOpenTorrentFilesAction, &QAction::triggered, this, &MainWindow::openTorrentsFiles);

        mShowInFileManagerAction = mTorrentMenu->addAction(
            QIcon::fromTheme("go-jump"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Show In &File Manager")
        );
        QObject::connect(mShowInFileManagerAction, &QAction::triggered, this, &MainWindow::showTorrentsInFileManager);

        mTorrentMenu->addSeparator();

        QAction* checkTorrentAction = mTorrentMenu->addAction(
            QIcon::fromTheme("document-preview"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Check Local Data")
        );
        QObject::connect(checkTorrentAction, &QAction::triggered, this, [=, this] {
            mRpc->checkTorrents(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        QAction* reannounceAction = mTorrentMenu->addAction(
            QIcon::fromTheme("view-refresh"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Reanno&unce")
        );
        QObject::connect(reannounceAction, &QAction::triggered, this, [=, this] {
            mRpc->reannounceTorrents(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        mTorrentMenu->addSeparator();

        QMenu* queueMenu = mTorrentMenu->addMenu(
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Queue")
        );

        QAction* moveTorrentToTopAction = queueMenu->addAction(
            QIcon::fromTheme("go-top"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Move To &Top")
        );
        QObject::connect(moveTorrentToTopAction, &QAction::triggered, this, [=, this] {
            mRpc->moveTorrentsToTop(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        QAction* moveTorrentUpAction = queueMenu->addAction(
            QIcon::fromTheme("go-up"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Move &Up")
        );
        QObject::connect(moveTorrentUpAction, &QAction::triggered, this, [=, this] {
            mRpc->moveTorrentsUp(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        QAction* moveTorrentDownAction = queueMenu->addAction(
            QIcon::fromTheme("go-down"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Move &Down")
        );
        QObject::connect(moveTorrentDownAction, &QAction::triggered, this, [=, this] {
            mRpc->moveTorrentsDown(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        QAction* moveTorrentToBottomAction = queueMenu->addAction(
            QIcon::fromTheme("go-bottom"_l1),
            //: Torrent's context menu item
            qApp->translate("tremotesf", "Move To &Bottom")
        );
        QObject::connect(moveTorrentToBottomAction, &QAction::triggered, this, [=, this] {
            mRpc->moveTorrentsToBottom(mTorrentsModel->idsFromIndexes(
                mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
            ));
        });

        const auto priorityMenu = mTorrentMenu->addMenu(
            //: Torrent's context menu item
            qApp->translate("tremotesf", "&Priority")
        );
        const auto priorityGroup = new QActionGroup(priorityMenu);
        priorityGroup->setExclusive(true);
        const auto setTorrentsPriority = [this](libtremotesf::TorrentData::Priority priority) {
            for (const auto& index :
                 mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())) {
                mTorrentsModel->torrentAtIndex(index)->setBandwidthPriority(priority);
            }
        };
        //: Torrent's loading priority
        mHighPriorityAction = priorityMenu->addAction(qApp->translate("tremotesf", "High"));
        QObject::connect(
            mHighPriorityAction,
            &QAction::triggered,
            this,
            std::bind_front(setTorrentsPriority, libtremotesf::TorrentData::Priority::High)
        );
        mHighPriorityAction->setCheckable(true);
        priorityGroup->addAction(mHighPriorityAction);

        //: Torrent's loading priority
        mNormalPriorityAction = priorityMenu->addAction(qApp->translate("tremotesf", "Normal"));
        QObject::connect(
            mNormalPriorityAction,
            &QAction::triggered,
            this,
            std::bind_front(setTorrentsPriority, libtremotesf::TorrentData::Priority::Normal)
        );
        mNormalPriorityAction->setCheckable(true);
        priorityGroup->addAction(mNormalPriorityAction);

        //: Torrent's loading priority
        mLowPriorityAction = priorityMenu->addAction(qApp->translate("tremotesf", "Low"));
        QObject::connect(
            mLowPriorityAction,
            &QAction::triggered,
            this,
            std::bind_front(setTorrentsPriority, libtremotesf::TorrentData::Priority::Low)
        );
        mLowPriorityAction->setCheckable(true);
        priorityGroup->addAction(mLowPriorityAction);
    }

    void MainWindow::updateRpcActions() {
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

    void MainWindow::addTorrentsFiles() {
        setWindowState(windowState() & ~Qt::WindowMinimized);

        auto settings = Settings::instance();
        auto showDialog = [=, this] {
            auto fileDialog = new QFileDialog(
                this,
                //: File chooser dialog title
                qApp->translate("tremotesf", "Select Files"),
                settings->rememberOpenTorrentDir() ? settings->lastOpenTorrentDirectory() : QString(),
                //: Torrent file type. Parentheses and text within them must remain unchanged
                qApp->translate("tremotesf", "Torrent Files (*.torrent)")
            );
            fileDialog->setAttribute(Qt::WA_DeleteOnClose);
            fileDialog->setFileMode(QFileDialog::ExistingFiles);

            QObject::connect(fileDialog, &QFileDialog::accepted, this, [=, this] {
                showAddTorrentFileDialogs(fileDialog->selectedFiles());
            });

            if constexpr (isTargetOsWindows) {
                fileDialog->open();
            } else {
                fileDialog->show();
            }
        };

        if (isHidden()) {
            show();
            runAfterDelay(showDialog);
        } else {
            showDialog();
        }
    }

    void MainWindow::showAddTorrentFileDialogs(const QStringList& files) {
        for (const QString& filePath : files) {
            auto dialog = new AddTorrentDialog(mRpc, filePath, AddTorrentDialog::Mode::File, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->activateWindow();
        }
    }

    void MainWindow::showAddTorrentLinkDialogs(const QStringList& urls) {
        for (const QString& url : urls) {
            auto dialog = new AddTorrentDialog(mRpc, url, AddTorrentDialog::Mode::Url, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            dialog->activateWindow();
        }
    }

    void MainWindow::updateTorrentActions() {
        const auto actions = mTorrentMenu->actions();

        if (!mTorrentsView->selectionModel()->hasSelection()) {
            for (QAction* action : actions) {
                action->setEnabled(false);
            }
            return;
        }

        for (QAction* action : actions) {
            action->setEnabled(true);
        }
        mHighPriorityAction->setChecked(false);
        mNormalPriorityAction->setChecked(false);
        mLowPriorityAction->setChecked(false);

        const QModelIndexList selectedRows = mTorrentsView->selectionModel()->selectedRows();
        if (selectedRows.size() == 1) {
            const auto torrent = mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(selectedRows.first()));
            if (torrent->data().status == libtremotesf::TorrentData::Status::Paused) {
                mPauseTorrentAction->setEnabled(false);
            } else {
                mStartTorrentAction->setEnabled(false);
                mStartTorrentNowAction->setEnabled(false);
            }
            switch (torrent->data().bandwidthPriority) {
            case libtremotesf::TorrentData::Priority::High:
                mHighPriorityAction->setChecked(true);
                break;
            case libtremotesf::TorrentData::Priority::Normal:
                mNormalPriorityAction->setChecked(true);
                break;
            case libtremotesf::TorrentData::Priority::Low:
                mLowPriorityAction->setChecked(true);
                break;
            }
        } else {
            mRenameTorrentAction->setEnabled(false);
        }

        if (mRpc->isLocal() || Servers::instance()->currentServerHasMountedDirectories()) {
            bool disableOpen = false;
            bool disableBoth = false;
            for (const QModelIndex& index : selectedRows) {
                libtremotesf::Torrent* torrent =
                    mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index));
                if (mRpc->isTorrentLocalMounted(torrent) &&
                    QFile::exists(mRpc->localTorrentDownloadDirectoryPath(torrent))) {
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
    }

    void MainWindow::showTorrentsPropertiesDialogs() {
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());

        for (int i = 0, max = selectedRows.size(); i < max; i++) {
            libtremotesf::Torrent* torrent =
                mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(selectedRows.at(i)));
            const int id = torrent->data().id;
            if (mTorrentsDialogs.find(id) != mTorrentsDialogs.end()) {
                if (i == (max - 1)) {
                    TorrentPropertiesDialog* dialog = mTorrentsDialogs[id];
                    showAndRaiseWindow(dialog);
                }
            } else {
                auto dialog = new TorrentPropertiesDialog(torrent, mRpc, this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                mTorrentsDialogs.insert({id, dialog});
                QObject::connect(dialog, &TorrentPropertiesDialog::finished, this, [=, this] {
                    mTorrentsDialogs.erase(mTorrentsDialogs.find(id));
                });

                dialog->show();
            }
        }
    }

    void MainWindow::removeSelectedTorrents(bool deleteFiles) {
        const auto ids = mTorrentsModel->idsFromIndexes(
            mTorrentsProxyModel->sourceIndexes(mTorrentsView->selectionModel()->selectedRows())
        );
        if (ids.empty()) {
            return;
        }

        QMessageBox dialog(this);
        dialog.setIcon(QMessageBox::Warning);

        dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        dialog.setDefaultButton(QMessageBox::Cancel);
        const auto okButton = dialog.button(QMessageBox::Ok);
        if (dialog.style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
            okButton->setIcon(QIcon::fromTheme("edit-delete"_l1));
        }

        //: Check box label
        QCheckBox deleteFilesCheckBox(qApp->translate("tremotesf", "Also delete the files on the hard disk"));
        deleteFilesCheckBox.setChecked(deleteFiles);
        dialog.setCheckBox(&deleteFilesCheckBox);

        auto setRemoveText = [&] {
            okButton->setText(
                deleteFilesCheckBox.isChecked()
                    //: Check box label
                    ? qApp->translate("tremotesf", "Delete with files")
                    //: Check box label
                    : qApp->translate("tremotesf", "Delete")
            );
        };
        setRemoveText();
        QObject::connect(&deleteFilesCheckBox, &QCheckBox::toggled, this, setRemoveText);

        if (ids.size() == 1) {
            //: Dialog title
            dialog.setWindowTitle(qApp->translate("tremotesf", "Delete Torrent"));
            dialog.setText(qApp->translate("tremotesf", "Are you sure you want to delete this torrent?"));
        } else {
            //: Dialog title
            dialog.setWindowTitle(qApp->translate("tremotesf", "Delete Torrents"));
            // Don't put static_cast in qApp->translate() - lupdate doesn't like it
            const auto count = static_cast<int>(ids.size());
            //: %Ln is a number of torrents selected for deletion
            dialog.setText(
                qApp->translate("tremotesf", "Are you sure you want to delete %Ln selected torrents?", nullptr, count)
            );
        }

        if (dialog.exec() == QMessageBox::Ok) {
            mRpc->removeTorrents(ids, deleteFilesCheckBox.checkState() == Qt::Checked);
        }
    }

    void MainWindow::setupTorrentsPlaceholder(QStackedLayout* parentLayout) {
        auto container = new QWidget(this);
        parentLayout->addWidget(container);
        parentLayout->setCurrentWidget(container);
        container->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto layout = new QVBoxLayout(container);
        layout->setAlignment(Qt::AlignHCenter);

        const auto setupPlaceholderLabel = [](QLabel* label) {
            label->setAlignment(Qt::AlignHCenter);
            label->setForegroundRole(QPalette::PlaceholderText);
            label->setTextInteractionFlags(Qt::NoTextInteraction);
            const auto setPalette = [label] {
#if QT_VERSION_MAJOR < 6
                auto palette = label->palette();
                auto brush = QGuiApplication::palette().placeholderText();
                brush.setStyle(Qt::SolidPattern);
                palette.setBrush(QPalette::PlaceholderText, brush);
                label->setPalette(palette);
#else
                static_assert(false, "Do we need this?");
#endif
            };
            setPalette();
            QObject::connect(qApp, &QGuiApplication::paletteChanged, label, setPalette);
        };

        layout->addStretch();

        auto status = new QLabel(this);
        layout->addWidget(status);
        setupPlaceholderLabel(status);
        {
            auto font = status->font();
            constexpr int minFontSize = 12;
            font.setPointSize(std::max(minFontSize, static_cast<int>(std::round(font.pointSize() * 1.3))));
            status->setFont(font);
        }

        auto error = new QLabel(this);
        layout->addWidget(error);
        setupPlaceholderLabel(error);
        {
            auto font = status->font();
            constexpr int minFontSize = 10;
            font.setPointSize(std::max(minFontSize, font.pointSize()));
            status->setFont(font);
        }

        layout->addStretch();

        const auto updatePlaceholder = [=, this] {
            QString statusText{};
            QString errorText{};
            if (mRpc->isConnected()) {
                if (mTorrentsProxyModel->rowCount() == 0) {
                    if (mTorrentsModel->rowCount() == 0) {
                        //: Torrents list placeholder
                        statusText = qApp->translate("tremotesf", "No torrents");
                    } else {
                        //: Torrents list placeholder
                        statusText = qApp->translate("tremotesf", "No torrents matching filters");
                    }
                }
            } else {
                statusText = mRpc->statusString();
                if (mRpc->error() != Rpc::Error::NoError) {
                    errorText = mRpc->errorMessage();
                }
            }
            status->setText(statusText);
            status->setVisible(!statusText.isEmpty());
            error->setText(errorText);
            error->setVisible(!errorText.isEmpty());
        };

        updatePlaceholder();
        QObject::connect(mRpc, &Rpc::statusChanged, this, updatePlaceholder);
        QObject::connect(
            mTorrentsProxyModel,
            &TorrentsModel::rowsInserted,
            this,
            [=, this](const QModelIndex&, int first, int last) {
                if ((last - first) + 1 == mTorrentsProxyModel->rowCount()) {
                    // Model was empty
                    updatePlaceholder();
                }
            }
        );
        QObject::connect(mTorrentsProxyModel, &TorrentsModel::rowsRemoved, this, [=, this] {
            if (mTorrentsProxyModel->rowCount() == 0) {
                // Model is now empty
                updatePlaceholder();
            }
        });
    }

    void MainWindow::setupMenuBar() {
        //: Menu bar item
        mFileMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&File"));
        mFileMenu->addAction(mConnectAction);
        mFileMenu->addAction(mDisconnectAction);
        mFileMenu->addSeparator();
        mFileMenu->addAction(mAddTorrentFileAction);
        mFileMenu->addAction(mAddTorrentLinkAction);
        mFileMenu->addSeparator();

        QAction* quitAction =
            mFileMenu->addAction(QIcon::fromTheme("application-exit"_l1), qApp->translate("tremotesf", "&Quit"));
        if constexpr (isTargetOsWindows) {
            quitAction->setShortcut(QKeySequence("Ctrl+Q"_l1));
        } else {
            quitAction->setShortcuts(QKeySequence::Quit);
        }
        QObject::connect(quitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);

        //: Menu bar item
        QMenu* editMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&Edit"));

        QAction* selectAllAction =
            editMenu->addAction(QIcon::fromTheme("edit-select-all"_l1), qApp->translate("tremotesf", "Select &All"));
        selectAllAction->setShortcut(QKeySequence::SelectAll);
        QObject::connect(selectAllAction, &QAction::triggered, mTorrentsView, &TorrentsView::selectAll);

        QAction* invertSelectionAction = editMenu->addAction(
            QIcon::fromTheme("edit-select-invert"_l1),
            qApp->translate("tremotesf", "&Invert Selection")
        );
        QObject::connect(invertSelectionAction, &QAction::triggered, this, [=, this] {
            mTorrentsView->selectionModel()->select(
                QItemSelection(
                    mTorrentsProxyModel->index(0, 0),
                    mTorrentsProxyModel
                        ->index(mTorrentsProxyModel->rowCount() - 1, mTorrentsProxyModel->columnCount() - 1)
                ),
                QItemSelectionModel::Toggle
            );
        });

        menuBar()->addMenu(mTorrentMenu);

        //: Menu bar item
        QMenu* viewMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&View"));

        mToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Toolbar"));
        mToolBarAction->setCheckable(true);
        QObject::connect(mToolBarAction, &QAction::triggered, mToolBar, &QToolBar::setVisible);

        QAction* sideBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Sidebar"));
        sideBarAction->setCheckable(true);
        sideBarAction->setChecked(Settings::instance()->isSideBarVisible());
        QObject::connect(sideBarAction, &QAction::triggered, this, [=, this](bool checked) {
            mSideBar->setVisible(checked);
            Settings::instance()->setSideBarVisible(checked);
        });

        QAction* statusBarAction = viewMenu->addAction(qApp->translate("tremotesf", "St&atusbar"));
        statusBarAction->setCheckable(true);
        statusBarAction->setChecked(Settings::instance()->isStatusBarVisible());
        QObject::connect(statusBarAction, &QAction::triggered, this, [=, this](bool checked) {
            statusBar()->setVisible(checked);
            Settings::instance()->setStatusBarVisible(checked);
        });

        viewMenu->addSeparator();
        QAction* lockToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Lock Toolbar"));
        lockToolBarAction->setCheckable(true);
        lockToolBarAction->setChecked(!mToolBar->isMovable());
        QObject::connect(lockToolBarAction, &QAction::triggered, mToolBar, [=, this](bool checked) {
            mToolBar->setMovable(!checked);
            Settings::instance()->setToolBarLocked(checked);
        });

        //: Menu bar item
        QMenu* toolsMenu = menuBar()->addMenu(qApp->translate("tremotesf", "T&ools"));

        QAction* settingsAction = toolsMenu->addAction(
            QIcon::fromTheme("configure"_l1, QIcon::fromTheme("preferences-system"_l1)),
            qApp->translate("tremotesf", "&Options")
        );
        settingsAction->setShortcut(QKeySequence::Preferences);
        QObject::connect(settingsAction, &QAction::triggered, this, [=, this] {
            showSingleInstanceDialog<SettingsDialog>(this, [this] { return new SettingsDialog(this); });
        });

        QAction* serversAction = toolsMenu->addAction(
            QIcon::fromTheme("network-server"_l1),
            qApp->translate("tremotesf", "&Connection Settings")
        );
        QObject::connect(serversAction, &QAction::triggered, this, [=, this] {
            showSingleInstanceDialog<ConnectionSettingsDialog>(this, [this] {
                return new ConnectionSettingsDialog(this);
            });
        });

        toolsMenu->addSeparator();

        auto serverSettingsAction = new QAction(
            QIcon::fromTheme("preferences-system-network"_l1, QIcon::fromTheme("preferences-system"_l1)),
            qApp->translate("tremotesf", "&Server Options"),
            this
        );
        QObject::connect(serverSettingsAction, &QAction::triggered, this, [=, this] {
            showSingleInstanceDialog<ServerSettingsDialog>(this, [this] {
                return new ServerSettingsDialog(mRpc, this);
            });
        });
        mConnectionDependentActions.push_back(serverSettingsAction);
        toolsMenu->addAction(serverSettingsAction);

        auto serverStatsAction =
            new QAction(QIcon::fromTheme("view-statistics"_l1), qApp->translate("tremotesf", "Server S&tats"), this);
        QObject::connect(serverStatsAction, &QAction::triggered, this, [=, this] {
            showSingleInstanceDialog<ServerStatsDialog>(this, [this] { return new ServerStatsDialog(mRpc, this); });
        });
        mConnectionDependentActions.push_back(serverStatsAction);
        toolsMenu->addAction(serverStatsAction);

        auto shutdownServerAction =
            new QAction(QIcon::fromTheme("system-shutdown"), qApp->translate("tremotesf", "S&hutdown Server"), this);
        QObject::connect(shutdownServerAction, &QAction::triggered, this, [=, this] {
            auto dialog = new QMessageBox(
                QMessageBox::Warning,
                //: Dialog title
                qApp->translate("tremotesf", "Shutdown Server"),
                qApp->translate("tremotesf", "Are you sure you want to shutdown remote Transmission instance?"),
                QMessageBox::Cancel | QMessageBox::Ok,
                this
            );
            auto okButton = dialog->button(QMessageBox::Ok);
            okButton->setIcon(QIcon::fromTheme("system-shutdown"));
            //: Dialog confirmation button
            okButton->setText(qApp->translate("tremotesf", "Shutdown"));
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setModal(true);
            QObject::connect(dialog, &QDialog::accepted, this, [=, this] { mRpc->shutdownServer(); });
            dialog->show();
        });
        mConnectionDependentActions.push_back(shutdownServerAction);
        toolsMenu->addAction(shutdownServerAction);

        //: Menu bar item
        QMenu* helpMenu = menuBar()->addMenu(qApp->translate("tremotesf", "&Help"));

        QAction* aboutAction = helpMenu->addAction(
            QIcon::fromTheme("help-about"_l1),
            //: Menu item opening "About" dialog
            qApp->translate("tremotesf", "&About")
        );
        QObject::connect(aboutAction, &QAction::triggered, this, [=, this] {
            showSingleInstanceDialog<AboutDialog>(this, [this] { return new AboutDialog(this); });
        });
    }

    void MainWindow::setupToolBar() {
        mToolBar = new QToolBar(this);
        mToolBar->setObjectName("toolBar"_l1);
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

        QObject::connect(mToolBar, &QToolBar::customContextMenuRequested, this, [=, this] {
            QMenu contextMenu;
            QActionGroup group(this);

            //: Toolbar mode
            group.addAction(qApp->translate("tremotesf", "Icon Only"))->setCheckable(true);
            //: Toolbar mode
            group.addAction(qApp->translate("tremotesf", "Text Only"))->setCheckable(true);
            //: Toolbar mode
            group.addAction(qApp->translate("tremotesf", "Text Beside Icon"))->setCheckable(true);
            //: Toolbar mode
            group.addAction(qApp->translate("tremotesf", "Text Under Icon"))->setCheckable(true);
            //: Toolbar mode
            group.addAction(qApp->translate("tremotesf", "Follow System Style"))->setCheckable(true);

            group.actions().at(toolButtonStyle())->setChecked(true);

            contextMenu.addActions(group.actions());

            QAction* action = contextMenu.exec(QCursor::pos());
            if (action) {
                setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(contextMenu.actions().indexOf(action)));
            }
        });
    }

    void MainWindow::setupTrayIcon() {
        auto contextMenu = new QMenu(mFileMenu);
        const QList<QAction*> actions(mFileMenu->actions());
        for (QAction* action : actions) {
            contextMenu->addAction(action);
        }

        mTrayIcon->setContextMenu(contextMenu);
        mTrayIcon->setToolTip(mRpc->statusString());

        QObject::connect(mTrayIcon, &QSystemTrayIcon::activated, this, [=, this](auto reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                if (isHidden() || isMinimized()) {
                    showWindow();
                } else {
                    hideWindow();
                }
            }
        });

        QObject::connect(mRpc, &Rpc::statusChanged, this, [=, this] { mTrayIcon->setToolTip(mRpc->statusString()); });

        QObject::connect(mRpc->serverStats(), &libtremotesf::ServerStats::updated, this, [=, this] {
            mTrayIcon->setToolTip(QString("\u25be %1\n\u25b4 %2")
                                      .arg(
                                          Utils::formatByteSpeed(mRpc->serverStats()->downloadSpeed()),
                                          Utils::formatByteSpeed(mRpc->serverStats()->uploadSpeed())
                                      ));
        });

        if (Settings::instance()->showTrayIcon()) {
            mTrayIcon->show();
        }

        QObject::connect(Settings::instance(), &Settings::showTrayIconChanged, this, [=, this] {
            if (Settings::instance()->showTrayIcon()) {
                mTrayIcon->show();
            } else {
                mTrayIcon->hide();
                showWindow();
            }
        });
    }

    void MainWindow::showWindow([[maybe_unused]] const QByteArray& newStartupNotificationId) {
#ifdef TREMOTESF_UNIX_FREEDESKTOP
        if (!newStartupNotificationId.isEmpty()) {
            KStartupInfo::appStarted(newStartupNotificationId);
        }
#endif
        showAndRaiseWindow(this, false);
        QWidget* lastDialog = nullptr;
        // Hiding/showing widgets while we are iterating over topLevelWidgets() is not safe, so wrap them in QPointers
        // so that we don't operate on deleted QWidgets
        for (const auto& widget : toQPointers(qApp->topLevelWidgets())) {
            if (widget && widget->windowType() == Qt::Dialog && !widget->inherits(kdePlatformFileDialogClassName)) {
                showAndRaiseWindow(widget, false);
                lastDialog = widget;
            }
        }
        QWidget* widgetToActivate = qApp->activeModalWidget();
        if (!widgetToActivate) {
            widgetToActivate = lastDialog;
        }
        if (!widgetToActivate) {
            widgetToActivate = this;
        }
        widgetToActivate->activateWindow();
    }

    void MainWindow::hideWindow() {
        // Hiding/showing widgets while we are iterating over topLevelWidgets() is not safe, so wrap them in QPointers
        // so that we don't operate on deleted QWidgets
        for (const auto& widget : toQPointers(qApp->topLevelWidgets())) {
            if (widget && widget->windowType() == Qt::Dialog && !widget->inherits(kdePlatformFileDialogClassName)) {
                widget->hide();
            }
        }
        hide();
    }

    void MainWindow::runAfterDelay(const std::function<void()>& function) {
        auto timer = new QTimer(this);
        timer->setInterval(100);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, this, function);
        QObject::connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
        timer->start();
    }

    void MainWindow::openTorrentsFiles() {
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
        for (const QModelIndex& index : selectedRows) {
            desktoputils::openFile(
                mRpc->localTorrentFilesPath(mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index))),
                this
            );
        }
    }

    void MainWindow::showTorrentsInFileManager() {
        std::vector<QString> files{};
        const QModelIndexList selectedRows(mTorrentsView->selectionModel()->selectedRows());
        files.reserve(static_cast<size_t>(selectedRows.size()));
        for (const QModelIndex& index : selectedRows) {
            libtremotesf::Torrent* torrent = mTorrentsModel->torrentAtIndex(mTorrentsProxyModel->sourceIndex(index));
            files.push_back(mRpc->localTorrentFilesPath(torrent));
        }
        launchFileManagerAndSelectFiles(files, this);
    }
}

#include "mainwindow.moc"
