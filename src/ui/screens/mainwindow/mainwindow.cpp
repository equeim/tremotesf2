// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <ranges>
#include <unordered_map>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
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
#include <QPointer>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QToolBar>

#ifdef TREMOTESF_UNIX_FREEDESKTOP
#    include <KStartupInfo>
#    include <KWindowSystem>
#    include "unixhelpers.h"
#endif

#include "log/log.h"
#include "rpc/serverstats.h"
#include "rpc/mounteddirectoriesutils.h"
#include "rpc/torrent.h"
#include "rpc/servers.h"

#include "ui/savewindowstatedispatcher.h"
#include "ui/stylehelpers.h"
#include "ui/screens/aboutdialog.h"
#include "ui/screens/addtorrent/addtorrentdialog.h"
#include "ui/screens/addtorrent/addtorrenthelpers.h"
#include "ui/screens/connectionsettings/servereditdialog.h"
#include "ui/screens/connectionsettings/connectionsettingsdialog.h"
#include "ui/screens/serversettings/serversettingsdialog.h"
#include "ui/screens/serverstatsdialog.h"
#include "ui/screens/settingsdialog.h"
#include "ui/screens/torrentproperties/torrentpropertiesdialog.h"
#include "ui/screens/torrentproperties/torrentpropertieswidget.h"
#include "ui/widgets/listplaceholder.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"
#include "ui/widgets/torrentfilesview.h"

#include "desktoputils.h"
#include "editlabelsdialog.h"
#include "filemanagerlauncher.h"
#include "formatutils.h"
#include "macoshelpers.h"
#include "mainwindowsidebar.h"
#include "mainwindowstatusbar.h"
#include "mainwindowviewmodel.h"
#include "settings.h"
#include "target_os.h"
#include "torrentsmodel.h"
#include "torrentsproxymodel.h"
#include "torrentsview.h"

using namespace Qt::StringLiterals;

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

                QObject::connect(rpc, &Rpc::connectedChanged, this, [rpc, this] {
                    if (!rpc->isConnected()) {
                        reject();
                    }
                });

                resize(sizeHint().expandedTo(QSize(320, 0)));
            }

            [[nodiscard]] QString downloadDirectory() const { return mDirectoryWidget->path(); }

            [[nodiscard]] bool moveFiles() const { return mMoveFilesCheckBox->isChecked(); }

        private:
            TorrentDownloadDirectoryDirectorySelectionWidget* const mDirectoryWidget;
            QCheckBox* const mMoveFilesCheckBox;
        };

#ifndef Q_OS_MACOS
        bool isAllowedToHide(const QWidget* window) {
            static constexpr std::array classNames{
                // Managed by QFileDialog
                "KDEPlatformFileDialog"_L1,
                "KDirSelectDialog"_L1,
                // Managed by QSystemTrayIcon
                "QSystemTrayIconSys"_L1
            };
            auto* const metaObject = window->metaObject();
            return metaObject && !std::ranges::contains(classNames, QLatin1String(metaObject->className()));
        }

        [[nodiscard]] std::vector<QPointer<QWidget>> toQPointers(const QWidgetList& widgets) {
            return {widgets.begin(), widgets.end()};
        }
#endif

        void unminimizeAndRaiseWindow(QWidget* window) {
            if (window->isMinimized()) {
                info().log("Unminimizing window {}", *window);
                window->setWindowState(window->windowState().setFlag(Qt::WindowMinimized, false));
            }
            info().log("Raising window {}", *window);
            window->raise();
        }

#ifdef TREMOTESF_UNIX_FREEDESKTOP
        void activeWindowOnX11(QWidget* window, const std::optional<QByteArray>& startupNotificationId) {
            if (startupNotificationId.has_value()) {
                info().log("Removing startup notification with id {}", *startupNotificationId);
                KStartupInfo::setNewStartupId(window->windowHandle(), *startupNotificationId);
                KStartupInfo::appStarted(*startupNotificationId);
            }
            window->activateWindow();
        }

        void activeWindowOnWayland(
            [[maybe_unused]] QWidget* window, [[maybe_unused]] const std::optional<QByteArray>& xdgActivationToken
        ) {
            if (xdgActivationToken.has_value()) {
                info().log("Activating window with token {}", *xdgActivationToken);
                // Qt gets new token from XDG_ACTIVATION_TOKEN environment variable
                // It we be read and unset in QWidget::activateWindow() call below
                qputenv(xdgActivationTokenEnvVariable, *xdgActivationToken);
            }
            window->activateWindow();
        }
#endif

        void activateWindowCompat(
            QWidget* window, [[maybe_unused]] const std::optional<QByteArray>& windowActivationToken = {}
        ) {
            info().log("Activating window {}", *window);
#ifdef TREMOTESF_UNIX_FREEDESKTOP
            switch (KWindowSystem::platform()) {
            case KWindowSystem::Platform::X11:
                debug().log("Windowing system is X11");
                activeWindowOnX11(window, windowActivationToken);
                break;
            case KWindowSystem::Platform::Wayland:
                debug().log("Windowing system is Wayland");
                activeWindowOnWayland(window, windowActivationToken);
                break;
            default:
                warning().log("Unknown windowing system");
                window->activateWindow();
                break;
            }
#else
            window->activateWindow();
#endif
        }
    }

    class MainWindow::Impl : public QObject {
        Q_OBJECT
    public:
        explicit Impl(QStringList&& commandLineFiles, QStringList&& commandLineUrls, MainWindow* window)
            : mWindow(window), mViewModel{std::move(commandLineFiles), std::move(commandLineUrls)} {
            mHorizontalSplitter.setChildrenCollapsible(false);
            if (!Settings::instance()->get_sideBarVisible()) {
                mSideBar.hide();
            }
            mHorizontalSplitter.addWidget(&mSideBar);

            mHorizontalSplitter.addWidget(&mVerticalSplitter);
            mHorizontalSplitter.setStretchFactor(1, 1);

            mVerticalSplitter.setChildrenCollapsible(false);
            mVerticalSplitter.setOrientation(Qt::Vertical);

            mVerticalSplitter.addWidget(&mTorrentsView);
            mVerticalSplitter.setStretchFactor(0, 1);
            QObject::connect(&mTorrentsView, &TorrentsView::customContextMenuRequested, this, [this](QPoint pos) {
                if (mTorrentsView.indexAt(pos).isValid()) {
                    mTorrentMenu->popup(mTorrentsView.viewport()->mapToGlobal(pos));
                }
            });
            QObject::connect(
                &mTorrentsView,
                &TorrentsView::activated,
                this,
                &MainWindow::Impl::performTorrentDoubleClickAction
            );

            QObject::connect(mViewModel.rpc(), &Rpc::connectedChanged, this, [this] {
                if (mViewModel.rpc()->isConnected() && mTorrentsProxyModel.rowCount() > 0) {
                    mTorrentsView.setCurrentIndex(mTorrentsProxyModel.index(0, 0));
                }
            });
            setupTorrentsPlaceholder();

            setupTorrentPropertiesWidget();

            mHorizontalSplitter.restoreState(Settings::instance()->get_horizontalSplitterState());
            mVerticalSplitter.restoreState(Settings::instance()->get_verticalSplitterState());

            mWindow->setCentralWidget(&mHorizontalSplitter);

            auto* const statusBar = new MainWindowStatusBar(mViewModel.rpc());
            mWindow->setStatusBar(statusBar);
            if (!Settings::instance()->get_statusBarVisible()) {
                statusBar->hide();
            }
            QObject::connect(statusBar, &MainWindowStatusBar::showConnectionSettingsDialog, this, [this] {
                showSingleInstanceDialog<ConnectionSettingsDialog>([this] {
                    return new ConnectionSettingsDialog(mWindow);
                });
            });

            setupActions();

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

            updateRpcActions();
            QObject::connect(mViewModel.rpc(), &Rpc::connectionStateChanged, this, &MainWindow::Impl::updateRpcActions);
            QObject::connect(
                Servers::instance(),
                &Servers::hasServersChanged,
                this,
                &MainWindow::Impl::updateRpcActions
            );

            mWindow->restoreState(Settings::instance()->get_mainWindowState());
            mToolBarAction->setChecked(!mToolBar.isHidden());

            QObject::connect(
                &mViewModel,
                &MainWindowViewModel::showWindow,
                this,
                &MainWindow::Impl::showWindowsOrActivateMainWindow
            );
            QObject::connect(
                &mViewModel,
                &MainWindowViewModel::showAddTorrentDialogs,
                this,
                &MainWindow::Impl::showAddTorrentDialogs
            );
            QObject::connect(
                &mViewModel,
                &MainWindowViewModel::askForMergingTrackers,
                this,
                &MainWindow::Impl::askForMergingTrackers
            );
            QObject::connect(
                &mViewModel,
                &MainWindowViewModel::showDelayedTorrentAddDialog,
                this,
                &MainWindow::Impl::showDelayedTorrentAddDialog
            );
            showAddTorrentErrors();

            auto pasteShortcut = new QShortcut(QKeySequence::Paste, mWindow);
            QObject::connect(
                pasteShortcut,
                &QShortcut::activated,
                &mViewModel,
                &MainWindowViewModel::pasteShortcutActivated
            );

            if (mViewModel.performStartupAction() == MainWindowViewModel::StartupActionResult::ShowAddServerDialog) {
                QMetaObject::invokeMethod(
                    this,
                    [this] {
                        auto* const dialog = new ServerEditDialog(nullptr, -1, mWindow);
                        dialog->setAttribute(Qt::WA_DeleteOnClose);
                        dialog->show();
                    },
                    Qt::QueuedConnection
                );
            }

            QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
                mViewModel.rpc()->disconnect();
                mTrayIcon.hide();
            });

            if constexpr (targetOs == TargetOs::UnixMacOS) {
                QObject::connect(
                    qApp,
                    &QGuiApplication::applicationStateChanged,
                    this,
                    &MainWindow::Impl::updateShowHideAction
                );
            }

            info().log("Restoring main window geometry");
            if (!mWindow->restoreGeometry(Settings::instance()->get_mainWindowGeometry())) {
                info().log("Did not restore geometry");
                mWindow->resize(mWindow->sizeHint().expandedTo(QSize(896, 640)));
            } else {
                info().log("Restored geometry {}", mWindow->geometry());
            }
        }

        Q_DISABLE_COPY_MOVE(Impl)

        void updateShowHideAction() {
            QObject::disconnect(&mShowHideAppAction, &QAction::triggered, nullptr, nullptr);
            if (isMainWindowHiddenOrMinimized()) {
                mShowHideAppAction.setText(qApp->translate("tremotesf", "&Show Tremotesf"));
                QObject::connect(&mShowHideAppAction, &QAction::triggered, this, [this] {
                    showWindowsOrActivateMainWindow();
                });
            } else {
                mShowHideAppAction.setText(qApp->translate("tremotesf", "&Hide Tremotesf"));
                QObject::connect(&mShowHideAppAction, &QAction::triggered, this, [this] { hideWindows(); });
            }
        }

        /**
         * @return true if event should be ignored, false otherwise
         */
        bool onCloseEvent() {
            if (mAppQuitEventFilter.isQuittingApplication) {
                debug().log("Received close event on main window while quitting app, just close window");
                return false;
            }
            // Do stuff at the next event loop iteration since we are in the middle of event handling
            if (mTrayIcon.isVisible() && QSystemTrayIcon::isSystemTrayAvailable()) {
                info().log("Closed main window but tray icon is active, hide windows without quitting app");
                QMetaObject::invokeMethod(this, &MainWindow::Impl::hideWindows, Qt::QueuedConnection);
                return true;
            }
            info().log("Closed main window when tray icon is not active, quitting app");
            QMetaObject::invokeMethod(qApp, &QCoreApplication::quit, Qt::QueuedConnection);
            return false;
        }

        void onDragEnterEvent(QDragEnterEvent* event) { MainWindowViewModel::processDragEnterEvent(event); }

        void onDropEvent(QDropEvent* event) { mViewModel.processDropEvent(event); }

        void saveState() {
            debug().log("Saving MainWindow state, window geometry is {}", mWindow->geometry());
            Settings::instance()->set_mainWindowGeometry(mWindow->saveGeometry());
            Settings::instance()->set_mainWindowState(mWindow->saveState());
            Settings::instance()->set_horizontalSplitterState(mHorizontalSplitter.saveState());
            Settings::instance()->set_verticalSplitterState(mVerticalSplitter.saveState());
            mTorrentsView.saveState();
            if (mTorrentPropertiesWidget) {
                mTorrentPropertiesWidget->saveState();
            }
        }

    private:
        MainWindow* mWindow;
        MainWindowViewModel mViewModel;

        QSplitter mHorizontalSplitter{};
        QSplitter mVerticalSplitter{};

        TorrentsModel mTorrentsModel{mViewModel.rpc()};
        TorrentsProxyModel mTorrentsProxyModel{&mTorrentsModel};
        TorrentsView mTorrentsView{&mTorrentsProxyModel};
        TorrentPropertiesWidget* mTorrentPropertiesWidget{};

        MainWindowSideBar mSideBar{mViewModel.rpc(), &mTorrentsProxyModel};
        std::unordered_map<QString, TorrentPropertiesDialog*> mTorrentPropertiesDialogs{};

        QAction mShowHideAppAction{};
        //: Button / menu item to connect to server
        QAction mConnectAction{qApp->translate("tremotesf", "&Connect")};
        //: Button / menu item to disconnect from server
        QAction mDisconnectAction{qApp->translate("tremotesf", "&Disconnect")};
        QAction mAddTorrentFileAction{
            QIcon::fromTheme("list-add"_L1),
            //: Menu item
            qApp->translate("tremotesf", "&Add Torrent File...")
        };
        QAction mAddTorrentLinkAction{
            QIcon::fromTheme("insert-link"_L1),
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
        QAction* mOpenTorrentDownloadDirectoryAction{};

        QAction* mHighPriorityAction{};
        QAction* mNormalPriorityAction{};
        QAction* mLowPriorityAction{};

        std::vector<QAction*> mConnectionDependentActions;

        QMenu* mFileMenu{};

        QToolBar mToolBar{};
        QAction* mToolBarAction{};

        QSystemTrayIcon mTrayIcon{QIcon::fromTheme("tremotesf-tray-icon"_L1, mWindow->windowIcon())};

#ifndef Q_OS_MACOS
        std::vector<QPointer<QWidget>> mOtherWindowsHiddenByUs;
#endif

        SaveWindowStateHandler mSaveStateHandler{mWindow, [this] { saveState(); }};
        ApplicationQuitEventFilter mAppQuitEventFilter{};

        void setupActions() {
            updateShowHideAction();

            QObject::connect(&mConnectAction, &QAction::triggered, mViewModel.rpc(), &Rpc::connect);
            QObject::connect(&mDisconnectAction, &QAction::triggered, mViewModel.rpc(), &Rpc::disconnect);

            const auto connectIcon = QIcon::fromTheme("network-connect"_L1);
            const auto disconnectIcon = QIcon::fromTheme("network-disconnect"_L1);
            if (connectIcon.name() != disconnectIcon.name()) {
                mConnectAction.setIcon(connectIcon);
                mDisconnectAction.setIcon(disconnectIcon);
            }

            mAddTorrentFileAction.setShortcuts(QKeySequence::Open);
            QObject::connect(&mAddTorrentFileAction, &QAction::triggered, this, &MainWindow::Impl::openTorrentFiles);
            mConnectionDependentActions.push_back(&mAddTorrentFileAction);

            QObject::connect(&mAddTorrentLinkAction, &QAction::triggered, this, [this] {
                if (Settings::instance()->get_showMainWindowWhenAddingTorrent() && isMainWindowHiddenOrMinimized()) {
                    showWindowsOrActivateMainWindow();
                }
                mViewModel.triggeredAddTorrentLinkAction();
            });
            mConnectionDependentActions.push_back(&mAddTorrentLinkAction);

            //
            // Torrent menu
            //
            //: Menu bar item
            mTorrentMenu = new QMenu(qApp->translate("tremotesf", "&Torrent"), mWindow);

            QAction* torrentPropertiesAction = mTorrentMenu->addAction(
                QIcon::fromTheme("document-properties"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Properties")
            );
            QObject::connect(
                torrentPropertiesAction,
                &QAction::triggered,
                this,
                &MainWindow::Impl::showTorrentsPropertiesDialogs
            );
            torrentPropertiesAction->setVisible(!Settings::instance()->get_showTorrentPropertiesInMainWindow());
            QObject::connect(
                Settings::instance(),
                &Settings::showTorrentPropertiesInMainWindowChanged,
                this,
                [torrentPropertiesAction] {
                    torrentPropertiesAction->setVisible(!Settings::instance()->get_showTorrentPropertiesInMainWindow());
                }
            );

            mTorrentMenu->addSeparator();

            mStartTorrentAction = mTorrentMenu->addAction(
                QIcon::fromTheme("media-playback-start"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Start")
            );
            QObject::connect(mStartTorrentAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->startTorrents(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            mStartTorrentNowAction = mTorrentMenu->addAction(
                QIcon::fromTheme("media-playback-start"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Start &Now")
            );
            QObject::connect(mStartTorrentNowAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->startTorrentsNow(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            mPauseTorrentAction = mTorrentMenu->addAction(
                QIcon::fromTheme("media-playback-pause"_L1),
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
                QIcon::fromTheme("edit-copy"_L1),
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
                QIcon::fromTheme("edit-delete"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Delete")
            );
            mRemoveTorrentAction->setShortcut(QKeySequence::Delete);
            QObject::connect(mRemoveTorrentAction, &QAction::triggered, this, [this] {
                removeSelectedTorrents(false);
            });

            const auto removeTorrentWithFilesShortcut =
                new QShortcut(QKeyCombination(Qt::ShiftModifier, Qt::Key_Delete), mWindow);
            QObject::connect(removeTorrentWithFilesShortcut, &QShortcut::activated, this, [this] {
                removeSelectedTorrents(true);
            });

            QAction* setLocationAction = mTorrentMenu->addAction(
                QIcon::fromTheme("mark-location"_L1),
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
                QIcon::fromTheme("edit-rename"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Rename")
            );
            mRenameTorrentAction->setShortcut(Qt::Key_F2);
            QObject::connect(mRenameTorrentAction, &QAction::triggered, this, [this] {
                const auto indexes = mTorrentsView.selectionModel()->selectedRows();
                if (indexes.size() == 1) {
                    const auto torrent =
                        mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.mapToSource(indexes.first()));
                    const auto id = torrent->data().id;
                    const auto name = torrent->data().name;
                    TorrentFilesView::showFileRenameDialog(name, mWindow, [id, name, this](const auto& newName) {
                        mViewModel.rpc()->renameTorrentFile(id, name, newName);
                    });
                }
            });

            QAction* editLabelsAction = mTorrentMenu->addAction(
                QIcon::fromTheme("tag"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Edi&t Labels")
            );
            QObject::connect(editLabelsAction, &QAction::triggered, this, [this] {
                if (mTorrentsView.selectionModel()->hasSelection()) {
                    const auto selectedTorrents =
                        mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                        | std::views::transform([this](const QModelIndex& index) {
                              return mTorrentsModel.torrentAtIndex(index);
                          })
                        | std::ranges::to<std::vector>();
                    auto dialog = new EditLabelsDialog(selectedTorrents, mViewModel.rpc(), mWindow);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->show();
                }
            });

            mTorrentMenu->addSeparator();

            mOpenTorrentFilesAction = mTorrentMenu->addAction(
                QIcon::fromTheme("document-open"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Open")
            );
            QObject::connect(mOpenTorrentFilesAction, &QAction::triggered, this, &MainWindow::Impl::openTorrentsFiles);

            mOpenTorrentDownloadDirectoryAction = mTorrentMenu->addAction(
                QIcon::fromTheme("go-jump"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Op&en Download Directory")
            );
            QObject::connect(
                mOpenTorrentDownloadDirectoryAction,
                &QAction::triggered,
                this,
                &MainWindow::Impl::showTorrentsInFileManager
            );

            mTorrentMenu->addSeparator();

            QAction* checkTorrentAction = mTorrentMenu->addAction(
                QIcon::fromTheme("document-preview"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "&Check Local Data")
            );
            QObject::connect(checkTorrentAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->checkTorrents(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* reannounceAction = mTorrentMenu->addAction(
                QIcon::fromTheme("view-refresh"_L1),
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
                QIcon::fromTheme("go-top"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Move To &Top")
            );
            QObject::connect(moveTorrentToTopAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsToTop(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* moveTorrentUpAction = queueMenu->addAction(
                QIcon::fromTheme("go-up"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Move &Up")
            );
            QObject::connect(moveTorrentUpAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsUp(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* moveTorrentDownAction = queueMenu->addAction(
                QIcon::fromTheme("go-down"_L1),
                //: Torrent's context menu item
                qApp->translate("tremotesf", "Move &Down")
            );
            QObject::connect(moveTorrentDownAction, &QAction::triggered, this, [this] {
                mViewModel.rpc()->moveTorrentsDown(mTorrentsModel.idsFromIndexes(
                    mTorrentsProxyModel.sourceIndexes(mTorrentsView.selectionModel()->selectedRows())
                ));
            });

            QAction* moveTorrentToBottomAction = queueMenu->addAction(
                QIcon::fromTheme("go-bottom"_L1),
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

        QAction* createQuitAction() {
            const auto action = new QAction(
                QIcon::fromTheme("application-exit"_L1),
                //: Menu item
                qApp->translate("tremotesf", "&Quit"),
                this
            );
            if constexpr (targetOs == TargetOs::Windows) {
                action->setShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_Q));
            } else {
                action->setShortcuts(QKeySequence::Quit);
            }
            QObject::connect(action, &QAction::triggered, this, &QCoreApplication::quit);
            return action;
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

        void openTorrentFiles() {
            auto* const settings = Settings::instance();
            if (settings->get_showMainWindowWhenAddingTorrent() && isMainWindowHiddenOrMinimized()) {
                showWindowsOrActivateMainWindow();
            }
            auto directory =
                settings->get_rememberOpenTorrentDir() ? settings->get_lastOpenTorrentDirectory() : QString{};
            if (directory.isEmpty()) {
                directory = QDir::homePath();
            }
            auto* const fileDialog = new QFileDialog(
                settings->get_showMainWindowWhenAddingTorrent() ? mWindow : nullptr,
                //: File chooser dialog title
                qApp->translate("tremotesf", "Select Files"),
                directory,
                //: Torrent file type. Parentheses and text within them must remain unchanged
                qApp->translate("tremotesf", "Torrent Files (*.torrent)")
            );
            fileDialog->setAttribute(Qt::WA_DeleteOnClose);
            fileDialog->setFileMode(QFileDialog::ExistingFiles);

            QObject::connect(fileDialog, &QFileDialog::accepted, this, [fileDialog, this] {
                mViewModel.acceptedFileDialog(fileDialog->selectedFiles());
            });

            if constexpr (targetOs == TargetOs::Windows) {
                fileDialog->open();
            } else {
                fileDialog->show();
            }
        }

        void setupTorrentPropertiesWidget() {
            const auto setup = [this] {
                if (Settings::instance()->get_showTorrentPropertiesInMainWindow()) {
                    useTorrentPropertiesWidget();
                } else {
                    useTorrentPropertiesDialogs();
                }
            };
            setup();
            QObject::connect(Settings::instance(), &Settings::showTorrentPropertiesInMainWindowChanged, this, setup);
        }

        void useTorrentPropertiesDialogs() {
            if (!mTorrentPropertiesWidget) {
                return;
            }

            mTorrentPropertiesWidget->deleteLater();
            mTorrentPropertiesWidget = nullptr;
        }

        void useTorrentPropertiesWidget() {
            if (mTorrentPropertiesWidget) {
                return;
            }

            if (!mTorrentPropertiesDialogs.empty()) {
                // Don't iterate over mTorrentPropertiesDialogs directly since call to reject() will modify it (through QDialog::finished slot)
                const auto dialogs = mTorrentPropertiesDialogs;
                mTorrentPropertiesDialogs.clear();
                for (const auto& [hashString, dialog] : dialogs) {
                    dialog->reject();
                }
            }

            mTorrentPropertiesWidget = new TorrentPropertiesWidget(mViewModel.rpc(), true, mWindow);
            mVerticalSplitter.addWidget(mTorrentPropertiesWidget);

            const auto updateCurrentTorrent = [this] {
                const auto currentIndex = mTorrentsView.selectionModel()->currentIndex();
                if (currentIndex.isValid()) {
                    auto source = mTorrentsProxyModel.mapToSource(currentIndex);
                    mTorrentPropertiesWidget->setTorrent(mTorrentsModel.torrentAtIndex(source));
                } else {
                    mTorrentPropertiesWidget->setTorrent(nullptr);
                }
            };
            updateCurrentTorrent();
            QObject::connect(
                mTorrentsView.selectionModel(),
                &QItemSelectionModel::currentChanged,
                mTorrentPropertiesWidget,
                updateCurrentTorrent
            );
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
                    mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.mapToSource(selectedRows.first()));
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

            bool localOrMounted = true;
            if (!mViewModel.rpc()->isLocal()) {
                for (const QModelIndex& index : selectedRows) {
                    Torrent* torrent = mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.mapToSource(index));
                    if (!isServerLocalOrTorrentIsMounted(mViewModel.rpc(), torrent)) {
                        localOrMounted = false;
                        break;
                    }
                }
            }
            mOpenTorrentFilesAction->setEnabled(localOrMounted);
            mOpenTorrentDownloadDirectoryAction->setEnabled(localOrMounted);
        }

        void performTorrentDoubleClickAction() {
            switch (Settings::instance()->get_torrentDoubleClickAction()) {
            case Settings::TorrentDoubleClickAction::OpenPropertiesDialog:
                if (Settings::instance()->get_showTorrentPropertiesInMainWindow()) {
                    warning().log(
                        "torrentDoubleClickAction is OpenPropertiesDialog, but "
                        "showTorrentPropertiesInMainWindow is true"
                    );
                } else {
                    showTorrentsPropertiesDialogs();
                }
                break;
            case Settings::TorrentDoubleClickAction::OpenTorrentFile:
                if (mOpenTorrentFilesAction->isEnabled()) {
                    openTorrentsFiles();
                }
                break;
            case Settings::TorrentDoubleClickAction::OpenDownloadDirectory:
                showTorrentsInFileManager();
                break;
            default:
                break;
            }
        }

        void showTorrentsPropertiesDialogs() {
            const QModelIndexList selectedRows(mTorrentsView.selectionModel()->selectedRows());
            for (const auto& index : selectedRows) {
                auto* const torrent = mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.mapToSource(index));
                const auto hashString = torrent->data().hashString;
                const auto existingDialog = mTorrentPropertiesDialogs.find(hashString);
                if (existingDialog != mTorrentPropertiesDialogs.end()) {
                    unminimizeAndRaiseWindow(existingDialog->second);
                    activateWindowCompat(existingDialog->second);
                } else {
                    auto dialog = new TorrentPropertiesDialog(torrent, mViewModel.rpc(), mWindow);
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    mTorrentPropertiesDialogs.emplace(hashString, dialog);
                    QObject::connect(dialog, &TorrentPropertiesDialog::finished, this, [hashString, this] {
                        mTorrentPropertiesDialogs.erase(hashString);
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
                okButton->setIcon(QIcon::fromTheme("edit-delete"_L1));
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

        void setupTorrentsPlaceholder() {
            auto layout = new QVBoxLayout(mTorrentsView.viewport());

            layout->addStretch();

            auto status = createListPlaceholderLabel();
            layout->addWidget(status);
            layout->setAlignment(status, Qt::AlignCenter);
            {
                auto font = status->font();
                constexpr int minFontSize = 12;
                font.setPointSize(std::max(minFontSize, static_cast<int>(std::round(font.pointSize() * 1.3))));
                status->setFont(font);
            }

            auto error = createListPlaceholderLabel();
            layout->addWidget(error);
            layout->setAlignment(error, Qt::AlignCenter);
            {
                auto font = error->font();
                constexpr int minFontSize = 10;
                font.setPointSize(std::max(minFontSize, font.pointSize()));
                error->setFont(font);
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
                unminimizeAndRaiseWindow(existingDialog);
                activateWindowCompat(existingDialog);
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

            if constexpr (targetOs == TargetOs::UnixMacOS) {
                auto closeWindowAction = mFileMenu->addAction(qApp->translate("tremotesf", "&Close Window"));
                closeWindowAction->setShortcuts(QKeySequence::Close);
                QObject::connect(closeWindowAction, &QAction::triggered, this, [] {
                    auto window = QApplication::activeWindow();
                    if (window) {
                        window->close();
                    }
                });
            }
            const auto quitAction = createQuitAction();
            quitAction->setMenuRole(QAction::QuitRole);
            mFileMenu->addAction(quitAction);

            //: Menu bar item
            QMenu* editMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "&Edit"));

            QAction* selectAllAction = editMenu->addAction(
                QIcon::fromTheme("edit-select-all"_L1),
                qApp->translate("tremotesf", "Select &All")
            );
            selectAllAction->setShortcut(QKeySequence::SelectAll);
            QObject::connect(selectAllAction, &QAction::triggered, &mTorrentsView, &TorrentsView::selectAll);

            QAction* invertSelectionAction = editMenu->addAction(
                QIcon::fromTheme("edit-select-invert"_L1),
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
            sideBarAction->setChecked(Settings::instance()->get_sideBarVisible());
            QObject::connect(sideBarAction, &QAction::triggered, this, [this](bool checked) {
                mSideBar.setVisible(checked);
                Settings::instance()->set_sideBarVisible(checked);
            });

            QAction* statusBarAction = viewMenu->addAction(qApp->translate("tremotesf", "St&atusbar"));
            statusBarAction->setCheckable(true);
            statusBarAction->setChecked(Settings::instance()->get_statusBarVisible());
            QObject::connect(statusBarAction, &QAction::triggered, this, [this](bool checked) {
                mWindow->statusBar()->setVisible(checked);
                Settings::instance()->set_statusBarVisible(checked);
            });

            QAction* torrentPropertiesWidgetAction =
                viewMenu->addAction(qApp->translate("tremotesf", "Torrent properties &panel"));
            torrentPropertiesWidgetAction->setCheckable(true);
            torrentPropertiesWidgetAction->setChecked(Settings::instance()->get_showTorrentPropertiesInMainWindow());
            QObject::connect(torrentPropertiesWidgetAction, &QAction::triggered, this, [](bool checked) {
                Settings::instance()->set_showTorrentPropertiesInMainWindow(checked);
                Settings::TorrentDoubleClickAction action;
                if (checked) {
                    action = Settings::TorrentDoubleClickAction::OpenTorrentFile;
                } else {
                    action = Settings::TorrentDoubleClickAction::OpenPropertiesDialog;
                }
                Settings::instance()->set_torrentDoubleClickAction(action);
            });
            QObject::connect(Settings::instance(), &Settings::showTorrentPropertiesInMainWindowChanged, this, [=] {
                torrentPropertiesWidgetAction->setChecked(
                    Settings::instance()->get_showTorrentPropertiesInMainWindow()
                );
            });

            viewMenu->addSeparator();
            QAction* lockToolBarAction = viewMenu->addAction(qApp->translate("tremotesf", "&Lock Toolbar"));
            lockToolBarAction->setCheckable(true);
            lockToolBarAction->setChecked(Settings::instance()->get_toolBarLocked());
            QObject::connect(lockToolBarAction, &QAction::triggered, &mToolBar, [this](bool checked) {
                mToolBar.setMovable(!checked);
                Settings::instance()->set_toolBarLocked(checked);
            });

            //: Menu bar item
            QMenu* toolsMenu = mWindow->menuBar()->addMenu(qApp->translate("tremotesf", "T&ools"));

            QAction* settingsAction = toolsMenu->addAction(
                QIcon::fromTheme("configure"_L1, QIcon::fromTheme("preferences-system"_L1)),
                qApp->translate("tremotesf", "&Options")
            );
            settingsAction->setShortcut(QKeySequence::Preferences);
            settingsAction->setMenuRole(QAction::PreferencesRole);
            QObject::connect(settingsAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<SettingsDialog>([this] {
                    return new SettingsDialog(mViewModel.rpc(), mWindow);
                });
            });

            QAction* serversAction = toolsMenu->addAction(
                QIcon::fromTheme("network-server"_L1),
                qApp->translate("tremotesf", "&Connection Settings")
            );
            serversAction->setMenuRole(QAction::NoRole);
            QObject::connect(serversAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<ConnectionSettingsDialog>([this] {
                    return new ConnectionSettingsDialog(mWindow);
                });
            });

            toolsMenu->addSeparator();

            auto serverSettingsAction = new QAction(
                QIcon::fromTheme("preferences-system-network"_L1, QIcon::fromTheme("preferences-system"_L1)),
                qApp->translate("tremotesf", "&Server Options"),
                this
            );
            serverSettingsAction->setMenuRole(QAction::NoRole);
            QObject::connect(serverSettingsAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<ServerSettingsDialog>([this] {
                    return new ServerSettingsDialog(mViewModel.rpc(), mWindow);
                });
            });
            mConnectionDependentActions.push_back(serverSettingsAction);
            toolsMenu->addAction(serverSettingsAction);

            auto serverStatsAction = new QAction(
                QIcon::fromTheme("view-statistics"_L1),
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
                QIcon::fromTheme("help-about"_L1),
                //: Menu item opening "About" dialog
                qApp->translate("tremotesf", "&About")
            );
            aboutAction->setMenuRole(QAction::AboutRole);
            QObject::connect(aboutAction, &QAction::triggered, this, [this] {
                showSingleInstanceDialog<AboutDialog>([this] { return new AboutDialog(mWindow); });
            });
        }

        void setupToolBar() {
            mToolBar.setObjectName("toolBar"_L1);
            mToolBar.setContextMenuPolicy(Qt::CustomContextMenu);
            mToolBar.setMovable(!Settings::instance()->get_toolBarLocked());
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

            QObject::connect(&mToolBar, &QToolBar::customContextMenuRequested, this, [this](QPoint pos) {
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

                QAction* action = contextMenu.exec(mToolBar.mapToGlobal(pos));
                if (action) {
                    const auto style = static_cast<Qt::ToolButtonStyle>(contextMenu.actions().indexOf(action));
                    mWindow->setToolButtonStyle(style);
                    Settings::instance()->set_toolButtonStyle(style);
                }
            });
        }

        void setupTrayIcon() {
            auto contextMenu = new QMenu(mWindow);

            contextMenu->addAction(&mShowHideAppAction);
            contextMenu->addSeparator();
            if constexpr (targetOs != TargetOs::UnixMacOS) {
                QObject::connect(&mTrayIcon, &QSystemTrayIcon::activated, this, [this](auto reason) {
                    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                        if (isMainWindowHiddenOrMinimized()) {
                            showWindowsOrActivateMainWindow();
                        } else {
                            hideWindows();
                        }
                    }
                });
            }

            contextMenu->addAction(&mConnectAction);
            contextMenu->addAction(&mDisconnectAction);
            contextMenu->addSeparator();
            contextMenu->addAction(&mAddTorrentFileAction);
            contextMenu->addAction(&mAddTorrentLinkAction);
            contextMenu->addSeparator();
            contextMenu->addAction(createQuitAction());

            mTrayIcon.setContextMenu(contextMenu);
            mTrayIcon.setToolTip(mViewModel.rpc()->status().toString());

            QObject::connect(mViewModel.rpc(), &Rpc::statusChanged, this, [this] {
                mTrayIcon.setToolTip(mViewModel.rpc()->status().toString());
            });

            QObject::connect(mViewModel.rpc()->serverStats(), &ServerStats::updated, this, [this] {
                mTrayIcon.setToolTip(
                    QString("\u25be %1\n\u25b4 %2")
                        .arg(
                            formatutils::formatByteSpeed(mViewModel.rpc()->serverStats()->downloadSpeed()),
                            formatutils::formatByteSpeed(mViewModel.rpc()->serverStats()->uploadSpeed())
                        )
                );
            });

            if (Settings::instance()->get_showTrayIcon()) {
                mTrayIcon.show();
            }

            QObject::connect(Settings::instance(), &Settings::showTrayIconChanged, this, [this] {
                if (Settings::instance()->get_showTrayIcon()) {
                    mTrayIcon.show();
                } else {
                    mTrayIcon.hide();
                    showWindowsOrActivateMainWindow();
                }
            });
        }

        bool isMainWindowHiddenOrMinimized() const {
            if constexpr (targetOs == TargetOs::UnixMacOS) {
                if (isNSAppHidden()) return true;
            }
            return mWindow->isHidden() || mWindow->isMinimized();
        }

        void showWindowsOrActivateMainWindow([[maybe_unused]] std::optional<QByteArray> windowActivationToken = {}) {
            info().log("Showing windows");
#ifdef Q_OS_MACOS
            if (isNSAppHidden()) {
                info().log("NSApp is hidden, unhiding it");
                unhideNSApp();
            } else {
                info().log("NSApp is not hidden, activating main window");
                unminimizeAndRaiseWindow(mWindow);
                activateWindowCompat(mWindow);
            }
#else
            if (!mWindow->isHidden()) {
                info().log("Main window is not hidden, activating it");
                unminimizeAndRaiseWindow(mWindow);
                activateWindowCompat(mWindow, windowActivationToken);
                return;
            }

#    if defined(TREMOTESF_UNIX_FREEDESKTOP)
            // With Wayland we need to set XDG_ACTIVATION_TOKEN environment variable before show()
            // so that Qt handles activation automatically
            if (windowActivationToken.has_value() && KWindowSystem::isPlatformWayland()) {
                info().log("Showing window with token {}", *windowActivationToken);
                // Qt gets new token from XDG_ACTIVATION_TOKEN environment variable
                // It we be read and unset in QWidget::show() call below
                qputenv(xdgActivationTokenEnvVariable, *windowActivationToken);
                windowActivationToken.reset();
            }
#    endif
            info().log("Showing window {}", *mWindow);
            mWindow->show();
            unminimizeAndRaiseWindow(mWindow);
            if (windowActivationToken.has_value()) {
                activateWindowCompat(mWindow, windowActivationToken);
            }
            for (const auto& window : mOtherWindowsHiddenByUs) {
                if (window) {
                    info().log("Showing window {}", *window);
                    window->show();
                    unminimizeAndRaiseWindow(window);
                }
            }
            mOtherWindowsHiddenByUs.clear();
#endif
        }

        void hideWindows() {
            info().log("Hiding windows");
#ifdef Q_OS_MACOS
            if (isNSAppHidden()) {
                info().log("NSApp is already hidden, do nothing");
                return;
            }
            // Hiding application doesn't work in fullscreen mode
            if (mWindow->isFullScreen()) {
                info().log("Exiting fullscreen");
                mWindow->setWindowState(mWindow->windowState().setFlag(Qt::WindowFullScreen, false));
            }
            info().log("Hiding NSApp");
            hideNSApp();
#else
            if (mWindow->isHidden()) {
                info().log("Main window is already hidden, do nothing");
                return;
            }
            info().log("Hiding {}", *mWindow);
            mWindow->hide();
            mOtherWindowsHiddenByUs.clear();
            for (auto&& widget : toQPointers(qApp->topLevelWidgets())) {
                if (widget != mWindow && widget->isWindow() && !widget->isHidden() && isAllowedToHide(widget)) {
                    info().log("Hiding {}", *widget);
                    widget->hide();
                    mOtherWindowsHiddenByUs.push_back(std::move(widget));
                }
            }
#endif
        }

        void openTorrentsFiles() {
            const QModelIndexList selectedRows(mTorrentsView.selectionModel()->selectedRows());
            for (const QModelIndex& index : selectedRows) {
                desktoputils::openFile(
                    localTorrentRootFilePath(
                        mViewModel.rpc(),
                        mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.mapToSource(index))
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
                Torrent* torrent = mTorrentsModel.torrentAtIndex(mTorrentsProxyModel.mapToSource(index));
                files.push_back(localTorrentRootFilePath(mViewModel.rpc(), torrent));
            }
            launchFileManagerAndSelectFiles(files, mWindow);
        }

        void showAddTorrentDialogs(
            const QStringList& files, const QStringList& urls, std::optional<QByteArray> windowActivationToken
        ) {
            if (!files.isEmpty()) {
                showAddTorrentFileDialogs(files, std::move(windowActivationToken));
                // NOLINTNEXTLINE(bugprone-use-after-move)
                windowActivationToken.reset();
            }
            if (!urls.isEmpty()) {
                showAddTorrentLinksDialog(urls, std::move(windowActivationToken));
            }
        }

        void showAddTorrentFileDialogs(const QStringList& files, std::optional<QByteArray> windowActivationToken) {
            const bool setParent = Settings::instance()->get_showMainWindowWhenAddingTorrent();
            for (const QString& filePath : files) {
                auto* const dialog = showAddTorrentFileDialog(filePath, setParent);
                if (windowActivationToken.has_value()) {
                    activateWindowCompat(dialog, windowActivationToken);
                    // Can use token only once
                    windowActivationToken.reset();
                }
            }
        }

        QDialog* showAddTorrentFileDialog(const QString& filePath, bool setParent) {
            auto* const dialog = new AddTorrentDialog(
                mViewModel.rpc(),
                AddTorrentDialog::FileParams{filePath},
                setParent ? mWindow : nullptr
            );
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            return dialog;
        }

        void showAddTorrentLinksDialog(const QStringList& urls, std::optional<QByteArray> windowActivationToken) {
            auto* const dialog = new AddTorrentDialog(
                mViewModel.rpc(),
                AddTorrentDialog::UrlParams{urls},
                Settings::instance()->get_showMainWindowWhenAddingTorrent() ? mWindow : nullptr
            );
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
            if (windowActivationToken.has_value()) {
                activateWindowCompat(dialog, windowActivationToken);
            }
        }

        void askForMergingTrackers(
            std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>> existingTorrents,
            std::optional<QByteArray> windowActivationToken
        ) {
            const bool setParent = Settings::instance()->get_showMainWindowWhenAddingTorrent();
            for (auto& [torrent, trackers] : existingTorrents) {
                auto* const dialog =
                    tremotesf::askForMergingTrackers(torrent, std::move(trackers), setParent ? mWindow : nullptr);
                if (windowActivationToken.has_value()) {
                    activateWindowCompat(dialog, windowActivationToken);
                    // Can use token only once
                    windowActivationToken.reset();
                }
            }
        }

        void showAddTorrentErrors() {
            const auto showError = [this](const QString& title, const QString& text) {
                QWidget* parent{};
                if (Settings::instance()->get_showMainWindowWhenAddingTorrent()) {
                    parent = mWindow;
                    showWindowsOrActivateMainWindow();
                }
                auto* const dialog = new QMessageBox(QMessageBox::Warning, title, text, QMessageBox::Close, parent);
                dialog->setAttribute(Qt::WA_DeleteOnClose, true);
                dialog->setModal(false);
                dialog->show();
                activateWindowCompat(dialog);
            };

            QObject::connect(mViewModel.rpc(), &Rpc::torrentAddDuplicate, this, [=] {
                showError(
                    qApp->translate("tremotesf", "Error adding torrent"),
                    qApp->translate("tremotesf", "This torrent is already added")
                );
            });

            QObject::connect(mViewModel.rpc(), &Rpc::torrentAddError, this, [=](const QString& filePathOrUrl) {
                showError(
                    qApp->translate("tremotesf", "Error adding torrent"),
                    qApp->translate("tremotesf", "Error adding torrent «%1»").arg(filePathOrUrl)
                );
            });
        }

        void showDelayedTorrentAddDialog(
            const QStringList& torrents, const std::optional<QByteArray>& windowActivationToken
        ) {
            debug().log("MainWindow: showing delayed torrent add dialog");
            const auto dialog = new QMessageBox(
                QMessageBox::Information,
                qApp->translate("tremotesf", "Disconnected"),
                //: Message shown when user attempts to add torrent while disconnect from server.
                qApp->translate("tremotesf", "Torrents will be added after connection to server"),
                QMessageBox::Close,
                Settings::instance()->get_showMainWindowWhenAddingTorrent() ? mWindow : nullptr
            );
            dialog->setAttribute(Qt::WA_DeleteOnClose, true);
            dialog->setModal(false);
            QString detailedText{};
            for (const auto& torrent : torrents) {
                detailedText += "\u2022 ";
                detailedText += torrent;
                detailedText += '\n';
            }
            dialog->setDetailedText(detailedText);
            dialog->show();
            if (windowActivationToken.has_value()) {
                activateWindowCompat(dialog, windowActivationToken);
            }
            QObject::connect(mViewModel.rpc(), &Rpc::connectedChanged, dialog, [=, this] {
                if (mViewModel.rpc()->isConnected()) dialog->close();
            });
        }
    };

    MainWindow::MainWindow(QStringList&& commandLineFiles, QStringList&& commandLineUrls, QWidget* parent)
        : QMainWindow(parent), mImpl(new Impl(std::move(commandLineFiles), std::move(commandLineUrls), this)) {
        setWindowTitle(TREMOTESF_APP_NAME ""_L1);
        setMinimumSize(minimumSizeHint().expandedTo(QSize(384, 256)));
        setContextMenuPolicy(Qt::NoContextMenu);
        setToolButtonStyle(Settings::instance()->get_toolButtonStyle());
        setAcceptDrops(true);
        if constexpr (targetOs == TargetOs::UnixMacOS) {
            if (determineStyle() == KnownStyle::macOS) {
                setUnifiedTitleAndToolBarOnMac(true);
            }
        }
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::initialShow(bool minimized) {
        if (!(minimized && Settings::instance()->get_showTrayIcon() && QSystemTrayIcon::isSystemTrayAvailable())) {
            show();
        }
    }

    bool MainWindow::event(QEvent* event) {
        if (event->type() == QEvent::WindowStateChange) {
            // This may be called in Impl constructor from restoreGeometry(), when mImpl is still uninitialized, so access it inside the lambda
            QMetaObject::invokeMethod(this, [this] { mImpl->updateShowHideAction(); }, Qt::QueuedConnection);
        }
        return QMainWindow::event(event);
    }

    void MainWindow::showEvent(QShowEvent* event) {
        QMetaObject::invokeMethod(this, [this] { mImpl->updateShowHideAction(); }, Qt::QueuedConnection);
        QMainWindow::showEvent(event);
    }

    void MainWindow::hideEvent(QHideEvent* event) {
        QMetaObject::invokeMethod(this, [this] { mImpl->updateShowHideAction(); }, Qt::QueuedConnection);
        QMainWindow::hideEvent(event);
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        if (mImpl->onCloseEvent()) {
            event->ignore();
        } else {
            QMainWindow::closeEvent(event);
        }
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) { mImpl->onDragEnterEvent(event); }

    void MainWindow::dropEvent(QDropEvent* event) { mImpl->onDropEvent(event); }
}

#include "mainwindow.moc"
