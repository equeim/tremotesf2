// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "rpc/mounteddirectoriesutils.h"

#include <algorithm>
#include <functional>
#include <cmath>
#include <unordered_map>

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
#    include <KWindowSystem>
#    include <kwindowsystem_version.h>
#    include "unixhelpers.h"
#endif

#include "log/log.h"
#include "rpc/serverstats.h"
#include "target_os.h"
#include "rpc/torrent.h"

#include "rpc/servers.h"
#include "filemanagerlauncher.h"
#include "desktoputils.h"
#include "settings.h"
#include "utils.h"

#include "ui/savewindowstatedispatcher.h"
#include "ui/screens/aboutdialog.h"
#include "ui/screens/addtorrent/addtorrentdialog.h"
#include "ui/screens/connectionsettings/servereditdialog.h"
#include "ui/screens/connectionsettings/connectionsettingsdialog.h"
#include "ui/screens/serversettings/serversettingsdialog.h"
#include "ui/screens/serverstatsdialog.h"
#include "ui/screens/settingsdialog.h"
#include "ui/screens/torrentproperties/torrentpropertiesdialog.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"
#include "ui/widgets/torrentfilesview.h"

#include "torrentsmodel.h"
#include "torrentsproxymodel.h"
#include "mainwindowsidebar.h"
#include "mainwindowstatusbar.h"
#include "torrentsview.h"
#include "mainwindowviewmodel.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QRect)

namespace tremotesf {
    namespace {
        class SetLocationDialog final : public QDialog {
            Q_OBJECT

        public:
            explicit SetLocationDialog(const QString& downloadDirectory, Rpc* rpc, QWidget* parent = nullptr)
                : QDialog(parent),
                  mDirectoryWidget(new TorrentDownloadDirectoryDirectorySelectionWidget(this)),
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

                mDirectoryWidget->setup(downloadDirectory, rpc);
                mDirectoryWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                layout->addWidget(mDirectoryWidget);

                mMoveFilesCheckBox->setChecked(true);
                layout->addWidget(mMoveFilesCheckBox);

                auto dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
                QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, [this] {
                    mDirectoryWidget->saveDirectories();
                    accept();
                });
                QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &SetLocationDialog::reject);

                QObject::connect(
                    mDirectoryWidget,
                    &RemoteDirectorySelectionWidget::pathChanged,
                    this,
                    [dialogButtonBox, this] {
                        dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(!mDirectoryWidget->path().isEmpty());
                    }
                );

                layout->addWidget(dialogButtonBox);

                setMinimumSize(minimumSizeHint());

                QObject::connect(rpc, &Rpc::connectedChanged, this, [rpc, this] {
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

        void showAndRaiseWindow(QWidget* window) {
            if (window->isHidden()) {
                window->show();
            }
            if (window->isMinimized()) {
                window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
            }
            window->raise();
        }
    }

    class MainWindow::Impl : public QObject {
        Q_OBJECT
    public:
        explicit Impl(QStringList&& commandLineFiles, QStringList&& commandLineUrls, MainWindow* window)
            : mWindow(window), mViewModel{std::move(commandLineFiles), std::move(commandLineUrls)} {
            mSplitter.setChildrenCollapsible(false);
            if (!Settings::instance()->isSideBarVisible()) {
                mSideBar.hide();
            }
            mSplitter.addWidget(&mSideBar);

            auto mainWidgetContainer = new QWidget(mWindow);
            mSplitter.addWidget(mainWidgetContainer);
            mSplitter.setStretchFactor(1, 1);
            auto mainWidgetLayout = new QVBoxLayout(mainWidgetContainer);
            mainWidgetLayout->setContentsMargins(
                0,
                mainWidgetLayout->contentsMargins().top(),
                0,
                mainWidgetLayout->contentsMargins().bottom()
            );

            auto messageWidget = new KMessageWidget(mWindow);
            messageWidget->setWordWrap(true);
            messageWidget->hide();
            mainWidgetLayout->addWidget(messageWidget);

            auto torrentsViewContainer = new QWidget(mWindow);
            mainWidgetLayout->addWidget(torrentsViewContainer);
            auto torrentsViewLayout = new QStackedLayout(torrentsViewContainer);
            torrentsViewLayout->setStackingMode(QStackedLayout::StackAll);

            torrentsViewLayout->addWidget(&mTorrentsView);
            QObject::connect(&mTorrentsView, &TorrentsView::customContextMenuRequested, this, [this](auto point) {
                if (mTorrentsView.indexAt(point).isValid()) {
                    mTorrentMenu->popup(QCursor::pos());
                }
            });
            QObject::connect(
                &mTorrentsView,
                &TorrentsView::activated,
                this,
                &MainWindow::Impl::showTorrentsPropertiesDialogs
            );

            setupTorrentsPlaceholder(torrentsViewLayout);

            mSplitter.restoreState(Settings::instance()->splitterState());

            mWindow->setCentralWidget(&mSplitter);

            mWindow->setStatusBar(new MainWindowStatusBar(mViewModel.rpc()));
            if (!Settings::instance()->isStatusBarVisible()) {
                mWindow->statusBar()->hide();
            }

            setupActions();

            updateRpcActions();
            QObject::connect(mViewModel.rpc(), &Rpc::connectionStateChanged, this, &MainWindow::Impl::updateRpcActions);
            QObject::connect(
                Servers::instance(),
                &Servers::hasServersChanged,
                this,
                &MainWindow::Impl::updateRpcActions
            );

            updateTorrentActions();
            QObject::connect(mViewModel.rpc(), &Rpc::torrentsUpdated, this, &MainWindow::Impl::updateTorrentActions);
            QObject::connect(
                mTorrentsView.selectionModel(),
                &QItemSelectionModel::selectionChanged,
                this,
                &MainWindow::Impl::updateTorrentActions
            );

            setupMenuBar();
            setupToolBar();

            setupTrayIcon();
            mViewModel.setupNotificationsController(&mTrayIcon);

            mWindow->restoreState(Settings::instance()->mainWindowState());
            mToolBarAction->setChecked(!mToolBar.isHidden());
            mWindow->restoreGeometry(Settings::instance()->mainWindowGeometry());

            QObject::connect(
                &mViewModel,
                &MainWindowViewModel::showWindow,
                this,
                &MainWindow::Impl::showWindowsAndActivateMainOrDialog
            );
            showAddTorrentDialogsFromIpc(messageWidget);
            showAddTorrentErrors();

            auto pasteShortcut = new QShortcut(QKeySequence::Paste, mWindow);
            QObject::connect(
                pasteShortcut,
                &QShortcut::activated,
                &mViewModel,
                &MainWindowViewModel::pasteShortcutActivated
            );

            if (mViewModel.performStartupAction() == MainWindowViewModel::StartupActionResult::ShowAddServerDialog) {
                runAfterDelay([this] {
                    auto dialog = new ServerEditDialog(nullptr, -1, mWindow);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->open();
                });
            }

            QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
                mViewModel.rpc()->disconnect();
                mTrayIcon.hide();
            });

            SaveWindowStateDispatcher::registerHandler(mWindow, [this] { saveState(); });
        }

        Q_DISABLE_COPY_MOVE(Impl)

        void onCloseEvent() {
            if (!(mTrayIcon.isVisible() && QSystemTrayIcon::isSystemTrayAvailable())) {
                qApp->quit();
            }
        }

        void onDragEnterEvent(QDragEnterEvent* event) { MainWindowViewModel::processDragEnterEvent(event); }

        void onDropEvent(QDropEvent* event) { mViewModel.processDropEvent(event); }

        void saveState() {
            logDebug("Saving MainWindow state, window geometry is {}", mWindow->geometry());
            Settings::instance()->setMainWindowGeometry(mWindow->saveGeometry());
            Settings::instance()->setMainWindowState(mWindow->saveState());
            Settings::instance()->setSplitterState(mSplitter.saveState());
            mTorrentsView.saveState();
        }

        void activateMainWindow() { activateWindow(mWindow, {}, {}); }

    private:
        MainWindow* mWindow;
        MainWindowViewModel mViewModel;

        QSplitter mSplitter{};

        TorrentsModel mTorrentsModel{mViewModel.rpc()};
        TorrentsProxyModel mTorrentsProxyModel{&mTorrentsModel};
        TorrentsView mTorrentsView{&mTorrentsProxyModel};

        MainWindowSideBar mSideBar{mViewModel.rpc(), &mTorrentsProxyModel};
        std::unordered_map<int, TorrentPropertiesDialog*> mTorrentsDialogs{};

        //: Button / menu item to connect to server
        QAction mConnectAction{qApp->translate("tremotesf", "&Connect")};
        //: Button / menu item to disconnect from server
        QAction mDisconnectAction{qApp->translate("tremotesf", "&Disconnect")};
        QAction mAddTorrentFileAction{
            QIcon::fromTheme("list-add"_l1),
            //: Menu item
            qApp->translate("tremotesf", "&Add Torrent File...")};
        QAction mAddTorrentLinkAction{
            QIcon::fromTheme("insert-link"_l1),
            //: Menu item
            qApp->translate("tremotesf", "Add Torrent &Link..."),
        };

        QMenu* mTorrentMenu{};
        QAction* mStartTorrentAction{};
        QAction* mStartTorrentNowAction{};
        QAction* mPauseTorrentAction{};
        QAction* mRemoveTorrentAction{};

        QAction* mRenameTorrentAction{};

        QAction* mOpenTorrentFilesAction{};
        QAction* mShowInFileManagerAction{};

        QAction* mHighPriorityAction{};
        QAction* mNormalPriorityAction{};
        QAction* mLowPriorityAction{};

        std::vector<QAction*> mConnectionDependentActions;

        QMenu* mFileMenu{};

        QToolBar mToolBar{};
        QAction* mToolBarAction{};

        QSystemTrayIcon mTrayIcon{QIcon::fromTheme("tremotesf-tray-icon"_l1, mWindow->windowIcon())};

        void setupActions() {
            QObject::connect(&mConnectAction, &QAction::triggered, mViewModel.rpc(), &Rpc::connect);
            QObject::connect(&mDisconnectAction, &QAction::triggered, mViewModel.rpc(), &Rpc::disconnect);

            const auto connectIcon = QIcon::fromTheme("network-connect"_l1);
            const auto disconnectIcon = QIcon::fromTheme("network-disconnect"_l1);
            if (connectIcon.name() != disconnectIcon.name()) {
                mConnectAction.setIcon(connectIcon);
                mDisconnectAction.setIcon(disconnectIcon);
            }

            mAddTorrentFileAction.setShortcuts(QKeySequence::Open);
            QObject::connect(&mAddTorrentFileAction, &QAction::triggered, this, &MainWindow::Impl::addTorrentsFiles);
            mConnectionDependentActions.push_back(&mAddTorrentFileAction);

            QObject::connect(&mAddTorrentLinkAction, &QAction::triggered, this, [this] {
                mWindow->setWindowState(mWindow->windowState() & ~Qt::WindowMinimized);
                auto showDialog = [this] {
                    auto dialog =
                        new AddTorrentDialog(mViewModel.rpc(), QString(), AddTorrentDialog::Mode::Url, mWindow);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->show();
                };
                if (mWindow->isHidden()) {
                    mWindow->show();
                    runAfterDelay(showDialog);
                } else {
                    showDialog();
                }
            });
            mConnectionDependentActions.push_back(&mAddTorrentLinkAction);

            //
            // Torrent menu
            //
            //: Menu bar item
            mTorrentMenu = new QMenu(qApp->translate("tremotesf", "&Torrent"), mWindow);

            QAction* torrentPropertiesAction = mTorrentMenu->addAction(
                QIcon::fromTheme("document-properties"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Properties")
            );
            QObject::connect(
                torrentPropertiesAction,
                &QAction::triggered,
                this,
                &MainWindow::Impl::showTorrentsPropertiesDialogs
            );

            mTorrentMenu->addSeparator();

            mStartTorrentAction = mTorrentMenu->addAction(
                QIcon::fromTheme("media-playback-start"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Start")
            );
            QObject::connect(mStartTorrentAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->startTorrents(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            mStartTorrentNowAction = mTorrentMenu->addAction(
                QIcon::fromTheme("media-playback-start"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Start &Now")
            );
            QObject::connect(mStartTorrentNowAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->startTorrentsNow(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            mPauseTorrentAction = mTorrentMenu->addAction(
                QIcon::fromTheme("media-playback-pause"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "P&ause")
            );
            QObject::connect(mPauseTorrentAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->pauseTorrents(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            mTorrentMenu->addSeparator();

            QAction* copyMagnetLinkAction = mTorrentMenu->addAction(
                QIcon::fromTheme("edit-copy"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Copy &Magnet Link")
            );
            QObject::connect(copyMagnetLinkAction, &QAction::triggered, this, [this] {
                const auto indexes(mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows()));
                QStringList links;
                links.reserve(indexes.size());
                for (const auto& index : indexes) {
                    links.push_back(mTorrentsModel.torrentAtIndex(index)->data().magnetLink);
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
            QObject::connect(mRemoveTorrentAction, &QAction::triggered, this, [this] {
                removeSelectedTorrents(false);
            });

            const auto removeTorrentWithFilesShortcut = new QShortcut(
#if QT_VERSION_MAJOR >= 6
                QKeyCombination(Qt::ShiftModifier, Qt::Key_Delete),
#else
                QKeySequence(static_cast<int>(Qt::ShiftModifier) | static_cast<int>(Qt::Key_Delete)),
#endif
                mWindow
            );
            QObject::connect(removeTorrentWithFilesShortcut, &QShortcut::activated, this, [this] {
                removeSelectedTorrents(true);
            });

            QAction* setLocationAction = mTorrentMenu->addAction(
                QIcon::fromTheme("mark-location"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Set &Location")
            );
            QObject::connect(setLocationAction, &QAction::triggered, this, [this] {
                if (mTorrentsView.selectionModel()->hasSelection()) {
                    QModelIndexList indexes(
                        mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                    );
                    auto dialog = new SetLocationDialog(
                        mTorrentsModel.torrentAtIndex(indexes.first())->data().downloadDirectory,
                        mViewModel.rpc(),
                        mWindow
                    );
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    QObject::connect(dialog, &SetLocationDialog::accepted, this, [indexes, dialog, this] {
                        mViewModel.rpc()->setTorrentsLocation(
                            mTorrentsModel.idsFromIndexes(indexes),
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
            QObject::connect(mRenameTorrentAction, &QAction::triggered, this, [this] {
                const auto indexes = mTorrentsView.selectionModel()->selectedRows();
                if (indexes.size() == 1) {
                    const auto torrent =
                        mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.sourceIndex(indexes.first()));
                    const auto id = torrent->data().id;
                    const auto name = torrent->data().name;
                    TorrentFilesView::showFileRenameDialog(name, mWindow, [id, name, this](const auto& newName) {
                        mViewModel.rpc()->renameTorrentFile(id, name, newName);
                    });
                }
            });

            mTorrentMenu->addSeparator();

            mOpenTorrentFilesAction = mTorrentMenu->addAction(
                QIcon::fromTheme("document-open"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Open")
            );
            QObject::connect(mOpenTorrentFilesAction, &QAction::triggered, this, &MainWindow::Impl::openTorrentsFiles);

            mShowInFileManagerAction = mTorrentMenu->addAction(
                QIcon::fromTheme("go-jump"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Show In &File Manager")
            );
            QObject::connect(
                mShowInFileManagerAction,
                &QAction::triggered,
                this,
                &MainWindow::Impl::showTorrentsInFileManager
            );

            mTorrentMenu->addSeparator();

            QAction* checkTorrentAction = mTorrentMenu->addAction(
                QIcon::fromTheme("document-preview"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Check Local Data")
            );
            QObject::connect(checkTorrentAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->checkTorrents(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* reannounceAction = mTorrentMenu->addAction(
                QIcon::fromTheme("view-refresh"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Reanno&unce")
            );
            QObject::connect(reannounceAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->reannounceTorrents(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
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
            QObject::connect(moveTorrentToTopAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsToTop(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* moveTorrentUpAction = queueMenu->addAction(
                QIcon::fromTheme("go-up"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Move &Up")
            );
            QObject::connect(moveTorrentUpAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsUp(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* moveTorrentDownAction = queueMenu->addAction(
                QIcon::fromTheme("go-down"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Move &Down")
            );
            QObject::connect(moveTorrentDownAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsDown(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* moveTorrentToBottomAction = queueMenu->addAction(
                QIcon::fromTheme("go-bottom"_l1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Move To &Bottom")
            );
            QObject::connect(moveTorrentToBottomAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsToBottom(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            const auto priorityMenu = mTorrentMenu->addMenu(
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Priority")
            );
            const auto priorityGroup = new QActionGroup(priorityMenu);
            priorityGroup->setExclusive(true);
            const auto setTorrentsPriority = [this](TorrentData::Priority priority) {
                for (const auto& index :
                     mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())) {
                    mTorrentsModel.torrentAtIndex(index)->setBandwidthPriority(priority);
                }
            };
            //: Torrent's loading priority
            mHighPriorityAction = priorityMenu->addAction(qApp->translate("tremotesf", "High"));
            QObject::connect(
                mHighPriorityAction,
                &QAction::triggered,
                this,
                std::bind_front(setTorrentsPriority, TorrentData::Priority::High)
            );
            mHighPriorityAction->setCheckable(true);
            priorityGroup->addAction(mHighPriorityAction);

            //: Torrent's loading priority
            mNormalPriorityAction = priorityMenu->addAction(qApp->translate("tremotesf", "Normal"));
            QObject::connect(
                mNormalPriorityAction,
                &QAction::triggered,
                this,
                std::bind_front(setTorrentsPriority, TorrentData::Priority::Normal)
            );
            mNormalPriorityAction->setCheckable(true);
            priorityGroup->addAction(mNormalPriorityAction);

            //: Torrent's loading priority
            mLowPriorityAction = priorityMenu->addAction(qApp->translate("tremotesf", "Low"));
            QObject::connect(
                mLowPriorityAction,
                &QAction::triggered,
                this,
                std::bind_front(setTorrentsPriority, TorrentData::Priority::Low)
            );
            mLowPriorityAction->setCheckable(true);
            priorityGroup->addAction(mLowPriorityAction);
        }

        void updateRpcActions() {
            if (Servers::instance()->hasServers()) {
                const bool disconnected = mViewModel.rpc()->connectionState() == Rpc::ConnectionState::Disconnected;
                mConnectAction.setEnabled(disconnected);
                mDisconnectAction.setEnabled(!disconnected);
            } else {
                mConnectAction.setEnabled(false);
                mDisconnectAction.setEnabled(false);
            }

            const bool connected = mViewModel.rpc()->isConnected();
            for (auto action : mConnectionDependentActions) {
                action->setEnabled(connected);
            }
        }

        void addTorrentsFiles() {
            mWindow->setWindowState(mWindow->windowState() & ~Qt::WindowMinimized);

            auto showDialog = [this] {
                auto settings = Settings::instance();
                auto directory = settings->rememberOpenTorrentDir() ? settings->lastOpenTorrentDirectory() : QString{};
                if (directory.isEmpty()) {
                    directory = QDir::homePath();
                }
                auto fileDialog = new QFileDialog(
                    mWindow,
                    //: File chooser dialog title
                    qApp->translate("tremotesf", "Select Files"),
                    directory,
                    //: Torrent file type. Parentheses and text within them must remain unchanged
                    qApp->translate("tremotesf", "Torrent Files (*.torrent)")
                );
                fileDialog->setAttribute(Qt::WA_DeleteOnClose);
                fileDialog->setFileMode(QFileDialog::ExistingFiles);

                QObject::connect(fileDialog, &QFileDialog::accepted, this, [fileDialog, this] {
                    showAddTorrentFileDialogs(fileDialog->selectedFiles());
                });

                if constexpr (isTargetOsWindows) {
                    fileDialog->open();
                } else {
                    fileDialog->show();
                }
            };

            if (mWindow->isHidden()) {
                mWindow->show();
                runAfterDelay(showDialog);
            } else {
                showDialog();
            }
        }

        void showAddTorrentFileDialogs(const QStringList& files) {
            for (const QString& filePath : files) {
                auto dialog = new AddTorrentDialog(mViewModel.rpc(), filePath, AddTorrentDialog::Mode::File, mWindow);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
                activateWindow(dialog);
            }
        }

        void showAddTorrentLinkDialogs(const QStringList& urls) {
            for (const QString& url : urls) {
                auto dialog = new AddTorrentDialog(mViewModel.rpc(), url, AddTorrentDialog::Mode::Url, mWindow);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
                activateWindow(dialog);
            }
        }

        void updateTorrentActions() {
            const auto actions = mTorrentMenu->actions();

            if (!mTorrentsView.selectionModel()->hasSelection()) {
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

            const QModelIndexList selectedRows = mTorrentsView.selectionModel()->selectedRows();
            if (selectedRows.size() == 1) {
                const auto torrent =
                    mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.sourceIndex(selectedRows.first()));
                if (torrent->data().status == TorrentData::Status::Paused) {
                    mPauseTorrentAction->setEnabled(false);
                } else {
                    mStartTorrentAction->setEnabled(false);
                    mStartTorrentNowAction->setEnabled(false);
                }
                switch (torrent->data().bandwidthPriority) {
                case TorrentData::Priority::High:
                    mHighPriorityAction->setChecked(true);
                    break;
                case TorrentData::Priority::Normal:
                    mNormalPriorityAction->setChecked(true);
                    break;
                case TorrentData::Priority::Low:
                    mLowPriorityAction->setChecked(true);
                    break;
                }
            } else {
                mRenameTorrentAction->setEnabled(false);
            }

            if (mViewModel.rpc()->isLocal() || Servers::instance()->currentServerHasMountedDirectories()) {
                bool disableOpen = false;
                bool disableBoth = false;
                for (const QModelIndex& index : selectedRows) {
                    Torrent* torrent = mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.sourceIndex(index));
                    if (isTorrentLocalMounted(mViewModel.rpc(), torrent) &&
                        QFile::exists(localTorrentDownloadDirectoryPath(mViewModel.rpc(), torrent))) {
                        if (!disableOpen && !QFile::exists(localTorrentFilesPath(mViewModel.rpc(), torrent))) {
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

        void showTorrentsPropertiesDialogs() {
            const QModelIndexList selectedRows(mTorrentsView.selectionModel()->selectedRows());

            for (QModelIndexList::size_type i = 0, max = selectedRows.size(); i < max; i++) {
                Torrent* torrent = mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.sourceIndex(selectedRows.at(i)));
                const int id = torrent->data().id;
                if (mTorrentsDialogs.find(id) != mTorrentsDialogs.end()) {
                    if (i == (max - 1)) {
                        TorrentPropertiesDialog* dialog = mTorrentsDialogs[id];
                        showAndRaiseWindow(dialog);
                        activateWindow(dialog);
                    }
                } else {
                    auto dialog = new TorrentPropertiesDialog(torrent, mViewModel.rpc(), mWindow);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    mTorrentsDialogs.insert({id, dialog});
                    QObject::connect(dialog, &TorrentPropertiesDialog::finished, this, [id, this] {
                        mTorrentsDialogs.erase(mTorrentsDialogs.find(id));
                    });

                    dialog->show();
                }
            }
        }

        void removeSelectedTorrents(bool deleteFiles) {
            const auto ids = mTorrentsModel.idsFromIndexes(
                mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
            );
            if (ids.empty()) {
                return;
            }

            QMessageBox dialog(mWindow);
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
                dialog.setText(qApp->translate(
                    "tremotesf",
                    "Are you sure you want to delete %Ln selected torrents?",
                    nullptr,
                    count
                ));
            }

            if (dialog.exec() == QMessageBox::Ok) {
                mViewModel.rpc()->removeTorrents(ids, deleteFilesCheckBox.checkState() == Qt::Checked);
            }
        }

        void setupTorrentsPlaceholder(QStackedLayout* parentLayout) {
            auto container = new QWidget(mWindow);
            parentLayout->addWidget(container);
            parentLayout->setCurrentWidget(container);
            container->setAttribute(Qt::WA_TransparentForMouseEvents);

            auto layout = new QVBoxLayout(container);
            layout->setAlignment(Qt::AlignHCenter);

            const auto setupPlaceholderLabel = [](QLabel* label) {
                label->setAlignment(Qt::AlignHCenter);
                label->setForegroundRole(QPalette::PlaceholderText);
                label->setTextInteractionFlags(Qt::NoTextInteraction);
#if QT_VERSION_MAJOR < 6
                const auto setPalette = [label] {
                    auto palette = label->palette();
                    auto brush = QGuiApplication::palette().placeholderText();
                    brush.setStyle(Qt::SolidPattern);
                    palette.setBrush(QPalette::PlaceholderText, brush);
                    label->setPalette(palette);
                };
                setPalette();
                QObject::connect(qApp, &QGuiApplication::paletteChanged, label, setPalette);
#endif
            };

            layout->addStretch();

            auto status = new QLabel(mWindow);
            layout->addWidget(status);
            setupPlaceholderLabel(status);
            {
                auto font = status->font();
                constexpr int minFontSize = 12;
                font.setPointSize(std::max(minFontSize, static_cast<int>(std::round(font.pointSize() * 1.3))));
                status->setFont(font);
            }

            auto error = new QLabel(mWindow);
            layout->addWidget(error);
            setupPlaceholderLabel(error);
            {
                auto font = status->font();
                constexpr int minFontSize = 10;
                font.setPointSize(std::max(minFontSize, font.pointSize()));
                status->setFont(font);
            }

            layout->addStretch();

            const auto updatePlaceholder = [status, error, this] {
                QString statusText{};
                QString errorText{};
                if (mViewModel.rpc()->isConnected()) {
                    if (mTorrentsProxyModel.rowCount() == 0) {
                        if (mTorrentsModel.rowCount() == 0) {
                            //: Torrents list placeholder
                            statusText = qApp->translate("tremotesf", "No torrents");
                        } else {
                            //: Torrents list placeholder
                            statusText = qApp->translate("tremotesf", "No torrents matching filters");
                        }
                    }
                } else if (Servers::instance()->hasServers()) {
                    statusText = mViewModel.rpc()->status().toString();
                    if (mViewModel.rpc()->error() != Rpc::Error::NoError) {
                        errorText = mViewModel.rpc()->errorMessage();
                    }
                } else {
                    statusText = qApp->translate("tremotesf", "No servers");
                }
                status->setText(statusText);
                status->setVisible(!statusText.isEmpty());
                error->setText(errorText);
                error->setVisible(!errorText.isEmpty());
            };

            updatePlaceholder();
            QObject::connect(mViewModel.rpc(), &Rpc::statusChanged, this, updatePlaceholder);
            QObject::connect(
                &mTorrentsProxyModel,
                &TorrentsModel::rowsInserted,
                this,
                [updatePlaceholder, this](const QModelIndex&, int first, int last) {
                    if ((last - first) + 1 == mTorrentsProxyModel.rowCount()) {
                        // Model was empty
                        updatePlaceholder();
                    }
                }
            );
            QObject::connect(&mTorrentsProxyModel, &TorrentsModel::rowsRemoved, this, [updatePlaceholder, this] {
                if (mTorrentsProxyModel.rowCount() == 0) {
                    // Model is now empty
                    updatePlaceholder();
                }
            });
        }

        template<std::derived_from<QDialog> Dialog, typename CreateDialogFunction>
            requires std::is_invocable_r_v<Dialog*, CreateDialogFunction>
        void showSingleInstanceDialog(CreateDialogFunction createDialog) {
            auto existingDialog = mWindow->findChild<Dialog*>({}, Qt::FindDirectChildrenOnly);
            if (existingDialog) {
                showAndRaiseWindow(existingDialog);
                activateWindow(existingDialog);
            } else {
                auto dialog = createDialog();
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            }
        }

        void setupMenuBar() {
            //: Menu bar item
            mFileMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "&File"));
            mFileMenu->addAction(&mConnectAction);
            mFileMenu->addAction(&mDisconnectAction);
            mFileMenu->addSeparator();
            mFileMenu->addAction(&mAddTorrentFileAction);
            mFileMenu->addAction(&mAddTorrentLinkAction);
            mFileMenu->addSeparator();

            QAction* quitAction =
                mFileMenu->addAction(QIcon::fromTheme("application-exit"_l1), qApp->translate("tremotesf", "&Quit"));
            if constexpr (isTargetOsWindows) {
#if QT_VERSION_MAJOR >= 6
                quitAction->setShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_Q));
#else
                quitAction->setShortcut(
                    QKeySequence(static_cast<int>(Qt::ControlModifier) | static_cast<int>(Qt::Key_Q))
                );
#endif
            } else {
                quitAction->setShortcuts(QKeySequence::Quit);
            }
            QObject::connect(quitAction, &QAction::triggered, this, &QCoreApplication::quit);

            //: Menu bar item
            QMenu* editMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "&Edit"));

            QAction* selectAllAction = editMenu->addAction(
                QIcon::fromTheme("edit-select-all"_l1),
                qApp->translate("tremotesf", "Select &All")
            );
            selectAllAction->setShortcut(QKeySequence::SelectAll);
            QObject::connect(selectAllAction, &QAction::triggered, &mTorrentsView, &TorrentsView::selectAll);

            QAction* invertSelectionAction = editMenu->addAction(
                QIcon::fromTheme("edit-select-invert"_l1),
                qApp->translate("tremotesf", "&Invert Selection")
            );
            QObject::connect(invertSelectionAction, &QAction::triggered, this, [this] {
                mTorrentsView.selectionModel()->select(
                    QItemSelection(
                        mTorrentsProxyModel.index(0, 0),
                        mTorrentsProxyModel
                            .index(mTorrentsProxyModel.rowCount() - 1, mTorrentsProxyModel.columnCount() - 1)
                    ),
                    QItemSelectionModel::Toggle
                );
            });

            mWindow->menuBar()->addMenu(mTorrentMenu);

            //: Menu bar item
            QMenu* viewMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "&View"));

            mToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Toolbar"));
            mToolBarAction->setCheckable(true);
            QObject::connect(mToolBarAction, &QAction::triggered, &mToolBar, &QToolBar::setVisible);

            QAction* sideBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Sidebar"));
            sideBarAction->setCheckable(true);
            sideBarAction->setChecked(Settings::instance()->isSideBarVisible());
            QObject::connect(sideBarAction, &QAction::triggered, this, [this](bool checked) {
                mSideBar.setVisible(checked);
                Settings::instance()->setSideBarVisible(checked);
            });

            QAction* statusBarAction = viewMenu->addAction(qApp->translate("tremotesf", "St&atusbar"));
            statusBarAction->setCheckable(true);
            statusBarAction->setChecked(Settings::instance()->isStatusBarVisible());
            QObject::connect(statusBarAction, &QAction::triggered, this, [this](bool checked) {
                mWindow->statusBar()->setVisible(checked);
                Settings::instance()->setStatusBarVisible(checked);
            });

            viewMenu->addSeparator();
            QAction* lockToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Lock Toolbar"));
            lockToolBarAction->setCheckable(true);
            lockToolBarAction->setChecked(!mToolBar.isMovable());
            QObject::connect(lockToolBarAction, &QAction::triggered, &mToolBar, [this](bool checked) {
                mToolBar.setMovable(!checked);
                Settings::instance()->setToolBarLocked(checked);
            });

            //: Menu bar item
            QMenu* toolsMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "T&ools"));

            QAction* settingsAction = toolsMenu->addAction(
                QIcon::fromTheme("configure"_l1, QIcon::fromTheme("preferences-system"_l1)),
                qApp->translate("tremotesf", "&Options")
            );
            settingsAction->setShortcut(QKeySequence::Preferences);
            QObject::connect(settingsAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<SettingsDialog>([this] { return new SettingsDialog(mWindow); });
            });

            QAction* serversAction = toolsMenu->addAction(
                QIcon::fromTheme("network-server"_l1),
                qApp->translate("tremotesf", "&Connection Settings")
            );
            QObject::connect(serversAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<ConnectionSettingsDialog>([this] {
                    return new ConnectionSettingsDialog(mWindow);
                });
            });

            toolsMenu->addSeparator();

            auto serverSettingsAction = new QAction(
                QIcon::fromTheme("preferences-system-network"_l1, QIcon::fromTheme("preferences-system"_l1)),
                qApp->translate("tremotesf", "&Server Options"),
                this
            );
            QObject::connect(serverSettingsAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<ServerSettingsDialog>([this] {
                    return new ServerSettingsDialog(mViewModel.rpc(), mWindow);
                });
            });
            mConnectionDependentActions.push_back(serverSettingsAction);
            toolsMenu->addAction(serverSettingsAction);

            auto serverStatsAction = new QAction(
                QIcon::fromTheme("view-statistics"_l1),
                qApp->translate("tremotesf", "Server S&tats"),
                this
            );
            QObject::connect(serverStatsAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<ServerStatsDialog>([this] {
                    return new ServerStatsDialog(mViewModel.rpc(), mWindow);
                });
            });
            mConnectionDependentActions.push_back(serverStatsAction);
            toolsMenu->addAction(serverStatsAction);

            auto shutdownServerAction = new QAction(
                QIcon::fromTheme("system-shutdown"),
                qApp->translate("tremotesf", "S&hutdown Server"),
                this
            );
            QObject::connect(shutdownServerAction, &QAction::triggered, this, [this] {
                auto dialog = new QMessageBox(
                    QMessageBox::Warning,
                    //: Dialog title
                    qApp->translate("tremotesf", "Shutdown Server"),
                    qApp->translate("tremotesf", "Are you sure you want to shutdown remote Transmission instance?"),
                    QMessageBox::Cancel | QMessageBox::Ok,
                    mWindow
                );
                auto okButton = dialog->button(QMessageBox::Ok);
                okButton->setIcon(QIcon::fromTheme("system-shutdown"));
                //: Dialog confirmation button
                okButton->setText(qApp->translate("tremotesf", "Shutdown"));
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->setModal(true);
                QObject::connect(dialog, &QDialog::accepted, this, [this] { mViewModel.rpc()->shutdownServer(); });
                dialog->show();
            });
            mConnectionDependentActions.push_back(shutdownServerAction);
            toolsMenu->addAction(shutdownServerAction);

            //: Menu bar item
            QMenu* helpMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "&Help"));

            QAction* aboutAction = helpMenu->addAction(
                QIcon::fromTheme("help-about"_l1),
                //: Menu item opening "About" dialog
                qApp->translate("tremotesf", "&About")
            );
            QObject::connect(aboutAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<AboutDialog>([this] { return new AboutDialog(mWindow); });
            });
        }

        void setupToolBar() {
            mToolBar.setObjectName("toolBar"_l1);
            mToolBar.setContextMenuPolicy(Qt::CustomContextMenu);
            mToolBar.setMovable(!Settings::instance()->isToolBarLocked());
            mWindow->addToolBar(Qt::TopToolBarArea, &mToolBar);

            mToolBar.addAction(&mConnectAction);
            mToolBar.addAction(&mDisconnectAction);
            mToolBar.addSeparator();
            mToolBar.addAction(&mAddTorrentFileAction);
            mToolBar.addAction(&mAddTorrentLinkAction);
            mToolBar.addSeparator();
            mToolBar.addAction(mStartTorrentAction);
            mToolBar.addAction(mPauseTorrentAction);
            mToolBar.addAction(mRemoveTorrentAction);

            QObject::connect(&mToolBar, &QToolBar::customContextMenuRequested, this, [this] {
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

                group.actions().at(mWindow->toolButtonStyle())->setChecked(true);

                contextMenu.addActions(group.actions());

                QAction* action = contextMenu.exec(QCursor::pos());
                if (action) {
                    const auto style = static_cast<Qt::ToolButtonStyle>(contextMenu.actions().indexOf(action));
                    mWindow->setToolButtonStyle(style);
                    Settings::instance()->setToolButtonStyle(style);
                }
            });
        }

        void setupTrayIcon() {
            auto contextMenu = new QMenu(mWindow);
            contextMenu->addActions(mFileMenu->actions());

            mTrayIcon.setContextMenu(contextMenu);
            mTrayIcon.setToolTip(mViewModel.rpc()->status().toString());

            QObject::connect(&mTrayIcon, &QSystemTrayIcon::activated, this, [this](auto reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                    if (mWindow->isHidden() || mWindow->isMinimized()) {
                        showWindowsAndActivateMainOrDialog();
                    } else {
                        hideWindows();
                    }
                }
            });

            QObject::connect(mViewModel.rpc(), &Rpc::statusChanged, this, [this] {
                mTrayIcon.setToolTip(mViewModel.rpc()->status().toString());
            });

            QObject::connect(mViewModel.rpc()->serverStats(), &ServerStats::updated, this, [this] {
                mTrayIcon.setToolTip(QString("\u25be %1\n\u25b4 %2")
                                         .arg(
                                             Utils::formatByteSpeed(mViewModel.rpc()->serverStats()->downloadSpeed()),
                                             Utils::formatByteSpeed(mViewModel.rpc()->serverStats()->uploadSpeed())
                                         ));
            });

            if (Settings::instance()->showTrayIcon()) {
                mTrayIcon.show();
            }

            QObject::connect(Settings::instance(), &Settings::showTrayIconChanged, this, [this] {
                if (Settings::instance()->showTrayIcon()) {
                    mTrayIcon.show();
                } else {
                    mTrayIcon.hide();
                    showWindowsAndActivateMainOrDialog();
                }
            });
        }

        void showWindowsAndActivateMainOrDialog(
            [[maybe_unused]] const QByteArray& newStartupNotificationId = {},
            [[maybe_unused]] const QByteArray& newXdgActivationToken = {}
        ) {
            showAndRaiseWindow(mWindow);
            QWidget* lastDialog = nullptr;
            // Hiding/showing widgets while we are iterating over topLevelWidgets() is not safe, so wrap them in QPointers
            // so that we don't operate on deleted QWidgets
            for (const auto& widget : toQPointers(qApp->topLevelWidgets())) {
                if (widget && widget->windowType() == Qt::Dialog && !widget->inherits(kdePlatformFileDialogClassName)) {
                    showAndRaiseWindow(widget);
                    lastDialog = widget;
                }
            }
            QWidget* widgetToActivate = qApp->activeModalWidget();
            if (!widgetToActivate) {
                widgetToActivate = lastDialog;
            }
            if (!widgetToActivate) {
                widgetToActivate = mWindow;
            }
            activateWindow(widgetToActivate, newStartupNotificationId, newXdgActivationToken);
        }

        void activateWindow(
            QWidget* widgetToActivate,
            [[maybe_unused]] const QByteArray& newStartupNotificationId = {},
            [[maybe_unused]] const QByteArray& newXdgActivationToken = {}
        ) {
            logInfo("Activating window {}", *widgetToActivate);
#ifdef TREMOTESF_UNIX_FREEDESKTOP
            switch (KWindowSystem::platform()) {
            case KWindowSystem::Platform::X11:
                logInfo("Windowing system is X11");
                activeWindowOnX11(widgetToActivate, newStartupNotificationId);
                break;
            case KWindowSystem::Platform::Wayland:
                logInfo("Windowing system is Wayland");
                activeWindowOnWayland(widgetToActivate, newXdgActivationToken);
                break;
            default:
                logWarning("Unknown windowing system");
                widgetToActivate->activateWindow();
                break;
            }
#else
            widgetToActivate->activateWindow();
#endif
        }

#ifdef TREMOTESF_UNIX_FREEDESKTOP
        void activeWindowOnX11(QWidget* widgetToActivate, const QByteArray& newStartupNotificationId) {
            if (!newStartupNotificationId.isEmpty()) {
                logInfo("Removing startup notification with id '{}'", newStartupNotificationId);
                KStartupInfo::setNewStartupId(widgetToActivate->windowHandle(), newStartupNotificationId);
                KStartupInfo::appStarted(newStartupNotificationId);
            }
            widgetToActivate->activateWindow();
        }

        void activeWindowOnWayland(QWidget* widgetToActivate, const QByteArray& newXdgActivationToken) {
            // Qt supports xdg-activation since 6.3
#    if KWINDOWSYSTEM_VERSION >= QT_VERSION_CHECK(5, 89, 0)
#        if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
            if (!newXdgActivationToken.isEmpty()) {
                logInfo("Updating xdg-activation token with new value '{}'", newXdgActivationToken);
                // Qt gets new token from XDG_ACTIVATION_TOKEN environment variable
                // It we be read and unset in QWidget::activateWindow() call below
                qputenv(xdgActivationTokenEnvVariable, newXdgActivationToken);
            }
            widgetToActivate->activateWindow();
#        else
            if (!newXdgActivationToken.isEmpty()) {
                logInfo("Updating xdg-activation token with new value '{}'", newXdgActivationToken);
                KWindowSystem::setCurrentXdgActivationToken(newXdgActivationToken);
            }
            if (const auto handle = widgetToActivate->windowHandle(); handle) {
                KWindowSystem::activateWindow(handle);
            } else {
                logWarning("This window's QWidget::windowHandle() is null");
            }
#        endif
#    else
            logWarning("Window activation on Wayland is not supported because KWindowSystem version is too low");
#    endif
        }
#endif

        void hideWindows() {
            // Hiding/showing widgets while we are iterating over topLevelWidgets() is not safe, so wrap them in QPointers
            // so that we don't operate on deleted QWidgets
            for (const auto& widget : toQPointers(qApp->topLevelWidgets())) {
                if (widget && widget->windowType() == Qt::Dialog && !widget->inherits(kdePlatformFileDialogClassName)) {
                    widget->hide();
                }
            }
            mWindow->hide();
        }

        void runAfterDelay(const std::function<void()>& function) {
            auto timer = new QTimer(this);
            timer->setInterval(100);
            timer->setSingleShot(true);
            QObject::connect(timer, &QTimer::timeout, this, function);
            QObject::connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
            timer->start();
        }

        void openTorrentsFiles() {
            const QModelIndexList selectedRows(mTorrentsView.selectionModel()->selectedRows());
            for (const QModelIndex& index : selectedRows) {
                desktoputils::openFile(
                    localTorrentFilesPath(
                        mViewModel.rpc(),
                        mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.sourceIndex(index))
                    ),
                    mWindow
                );
            }
        }

        void showTorrentsInFileManager() {
            std::vector<QString> files{};
            const QModelIndexList selectedRows(mTorrentsView.selectionModel()->selectedRows());
            files.reserve(static_cast<size_t>(selectedRows.size()));
            for (const QModelIndex& index : selectedRows) {
                Torrent* torrent = mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.sourceIndex(index));
                files.push_back(localTorrentFilesPath(mViewModel.rpc(), torrent));
            }
            launchFileManagerAndSelectFiles(files, mWindow);
        }

        void showAddTorrentDialogsFromIpc(KMessageWidget* messageWidget) {
            QObject::connect(
                &mViewModel,
                &MainWindowViewModel::showAddTorrentDialogs,
                this,
                [messageWidget, this](const auto& files, const auto& urls) {
                    if (messageWidget->isVisible()) {
                        messageWidget->animatedHide();
                    }
                    const bool wasHidden = mWindow->isHidden();
                    showWindowsAndActivateMainOrDialog();
                    if (wasHidden) {
                        runAfterDelay([files, urls, this] {
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
                &mViewModel,
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
                        text += qApp->translate("tremotesf", "And %n more", nullptr, static_cast<int>(remaining));
                    }
                    messageWidget->setText(text);
                    messageWidget->animatedShow();
                }
            );
        }

        void showAddTorrentErrors() {
            QObject::connect(mViewModel.rpc(), &Rpc::torrentAddDuplicate, this, [this] {
                QMessageBox::warning(
                    mWindow,
                    qApp->translate("tremotesf", "Error adding torrent"),
                    qApp->translate("tremotesf", "This torrent is already added"),
                    QMessageBox::Close
                );
            });

            QObject::connect(mViewModel.rpc(), &Rpc::torrentAddError, this, [this] {
                QMessageBox::warning(
                    mWindow,
                    qApp->translate("tremotesf", "Error adding torrent"),
                    qApp->translate("tremotesf", "Error adding torrent"),
                    QMessageBox::Close
                );
            });
        }
    };

    MainWindow::MainWindow(QStringList&& commandLineFiles, QStringList&& commandLineUrls, QWidget* parent)
        : QMainWindow(parent), mImpl(new Impl(std::move(commandLineFiles), std::move(commandLineUrls), this)) {
        setWindowTitle(TREMOTESF_APP_NAME ""_l1);
        setMinimumSize(minimumSizeHint().expandedTo(QSize(384, 256)));
        setContextMenuPolicy(Qt::NoContextMenu);
        setToolButtonStyle(Settings::instance()->toolButtonStyle());
        setAcceptDrops(true);
    }

    MainWindow::~MainWindow() = default;

    QSize MainWindow::sizeHint() const { return minimumSizeHint().expandedTo(QSize(896, 640)); }

    void MainWindow::showMinimized(bool minimized) {
        if (!(minimized && Settings::instance()->showTrayIcon() && QSystemTrayIcon::isSystemTrayAvailable())) {
            show();
            mImpl->activateMainWindow();
        }
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        QMainWindow::closeEvent(event);
        mImpl->onCloseEvent();
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) { mImpl->onDragEnterEvent(event); }

    void MainWindow::dropEvent(QDropEvent* event) { mImpl->onDropEvent(event); }
}

#include "mainwindow.moc"
