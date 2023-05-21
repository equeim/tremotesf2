// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "addtorrentdialog.h"

#include <array>

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <KColumnResizer>
#include <KMessageWidget>

#include "libtremotesf/fileutils.h"
#include "libtremotesf/log.h"
#include "libtremotesf/stdutils.h"
#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/servers.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/utils.h"
#include "tremotesf/settings.h"
#include "tremotesf/ui/widgets/torrentremotedirectoryselectionwidget.h"
#include "tremotesf/ui/widgets/torrentfilesview.h"

#include "droppedtorrents.h"
#include "localtorrentfilesmodel.h"

namespace tremotesf {
    namespace {
        constexpr std::array priorityComboBoxItems{
            libtremotesf::TorrentData::Priority::High,
            libtremotesf::TorrentData::Priority::Normal,
            libtremotesf::TorrentData::Priority::Low};

        libtremotesf::TorrentData::Priority priorityFromComboBoxIndex(int index) {
            if (index == -1) {
                return libtremotesf::TorrentData::Priority::Normal;
            }
            return priorityComboBoxItems.at(static_cast<size_t>(index));
        }
    }

    AddTorrentDialog::AddTorrentDialog(Rpc* rpc, const QString& url, Mode mode, QWidget* parent)
        : QDialog(parent),
          mRpc(rpc),
          mUrl(url),
          mMode(mode),
          mFilesModel(mode == Mode::File ? new LocalTorrentFilesModel(this) : nullptr) {
        setupUi();

        if (mFilesModel) {
            mFilesModel->load(url);
        }
    }

    QSize AddTorrentDialog::sizeHint() const {
        if (mMode == Mode::File) {
            return minimumSizeHint().expandedTo(QSize(448, 512));
        }
        return minimumSizeHint().expandedTo(QSize(448, 0));
    }

    void AddTorrentDialog::accept() {
        if (mMode == Mode::File) {
            mRpc->addTorrentFile(
                mUrl,
                mDownloadDirectoryWidget->path(),
                mFilesModel->unwantedFiles(),
                mFilesModel->highPriorityFiles(),
                mFilesModel->lowPriorityFiles(),
                mFilesModel->renamedFiles(),
                priorityFromComboBoxIndex(mPriorityComboBox->currentIndex()),
                mStartTorrentCheckBox->isChecked()
            );
        } else {
            mRpc->addTorrentLink(
                mTorrentLinkLineEdit->text(),
                mDownloadDirectoryWidget->path(),
                priorityFromComboBoxIndex(mPriorityComboBox->currentIndex()),
                mStartTorrentCheckBox->isChecked()
            );
        }

        mDownloadDirectoryWidget->saveDirectories();

        const auto settings = Settings::instance();
        if (settings->rememberOpenTorrentDir() && mMode == Mode::File) {
            settings->setLastOpenTorrentDirectory(QFileInfo(mUrl).path());
        }
        if (settings->rememberAddTorrentParameters()) {
            settings->setLastAddTorrentPriority(priorityFromComboBoxIndex(mPriorityComboBox->currentIndex()));
            settings->setLastAddTorrentStartAfterAdding(mStartTorrentCheckBox->isChecked());
            if (mMode == Mode::File) {
                settings->setLastAddTorrentDeleteTorrentFile(mDeleteTorrentFileCheckBox->isChecked());
            }
        }
        if (mMode == Mode::File && mDeleteTorrentFileCheckBox->isChecked()) {
            try {
                deleteFile(mUrl);
            } catch (const QFileError& e) {
                logWarningWithException(e, "Failed to delete torrent file");
            }
        }

        QDialog::accept();
    }

    QString AddTorrentDialog::initialDownloadDirectory() {
        if (Settings::instance()->rememberAddTorrentParameters()) {
            auto lastDir = Servers::instance()->currentServerLastDownloadDirectory(mRpc->serverSettings());
            if (!lastDir.isEmpty()) return lastDir;
        }
        return mRpc->serverSettings()->data().downloadDirectory;
    }

    void AddTorrentDialog::setupUi() {
        if (mMode == Mode::File) {
            //: Dialog title
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent File"));
        } else {
            //: Dialog title
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent Link"));
        }

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto messageWidget = new KMessageWidget(this);
        messageWidget->setCloseButtonVisible(false);
        messageWidget->hide();
        layout->addWidget(messageWidget);

        auto firstFormWidget = new QWidget(this);
        firstFormWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        auto firstFormLayout = new QFormLayout(firstFormWidget);
        firstFormLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(firstFormWidget);

        mTorrentLinkLineEdit = new QLineEdit(mUrl, this);
        if (mMode == Mode::File) {
            mTorrentLinkLineEdit->setReadOnly(true);
            //: Input field's label
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent file:"), mTorrentLinkLineEdit);
        } else {
            //: Input field's label
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent link:"), mTorrentLinkLineEdit);
            if (mUrl.isEmpty() && Settings::instance()->fillTorrentLinkFromClipboard()) {
                const auto dropped = DroppedTorrents(QGuiApplication::clipboard()->mimeData());
                if (!dropped.urls.isEmpty()) {
                    logInfo("AddTorrentDialog: filling torrent link from clipboard = {}", dropped.urls.first());
                    mTorrentLinkLineEdit->setText(dropped.urls.first());
                }
            }
            if (!mTorrentLinkLineEdit->text().isEmpty()) {
                mTorrentLinkLineEdit->setCursorPosition(0);
            }
            QObject::connect(mTorrentLinkLineEdit, &QLineEdit::textChanged, this, &AddTorrentDialog::canAcceptUpdate);
        }

        mDownloadDirectoryWidget = new TorrentDownloadDirectoryDirectorySelectionWidget(this);
        mDownloadDirectoryWidget->setup(initialDownloadDirectory(), mRpc);
        //: Input field's label
        firstFormLayout->addRow(qApp->translate("tremotesf", "Download directory:"), mDownloadDirectoryWidget);

        auto freeSpaceLabel = new QLabel(this);
        auto getFreeSpace = [=, this](const QString& directory) {
            if (!directory.isEmpty()) {
                if (mRpc->serverSettings()->data().canShowFreeSpaceForPath()) {
                    mRpc->getFreeSpaceForPath(directory);
                    return;
                }
                if (directory == mRpc->serverSettings()->data().downloadDirectory) {
                    mRpc->getDownloadDirFreeSpace();
                    return;
                }
            }
            freeSpaceLabel->hide();
            freeSpaceLabel->clear();
        };
        QObject::connect(mDownloadDirectoryWidget, &RemoteDirectorySelectionWidget::pathChanged, this, [=, this] {
            getFreeSpace(mDownloadDirectoryWidget->path());
        });
        if (mRpc->serverSettings()->data().canShowFreeSpaceForPath()) {
            QObject::connect(
                mRpc,
                &Rpc::gotFreeSpaceForPath,
                this,
                [=, this](const QString& path, bool success, long long bytes) {
                    if (path == mDownloadDirectoryWidget->path()) {
                        if (success) {
                            freeSpaceLabel->setText(
                                //: %1 is a amount of free space in a directory, e.g. 1 GiB
                                qApp->translate("tremotesf", "Free space: %1").arg(Utils::formatByteSize(bytes))
                            );
                        } else {
                            freeSpaceLabel->setText(qApp->translate("tremotesf", "Error getting free space"));
                        }
                        freeSpaceLabel->show();
                    }
                }
            );
        } else {
            QObject::connect(mRpc, &Rpc::gotDownloadDirFreeSpace, this, [=, this](auto bytes) {
                if (mDownloadDirectoryWidget->path() == mRpc->serverSettings()->data().downloadDirectory) {
                    freeSpaceLabel->setText(
                        qApp->translate("tremotesf", "Free space: %1").arg(Utils::formatByteSize(bytes))
                    );
                    freeSpaceLabel->show();
                }
            });
        }
        getFreeSpace(mDownloadDirectoryWidget->path());
        firstFormLayout->addRow(nullptr, freeSpaceLabel);

        TorrentFilesView* torrentFilesView = nullptr;
        if (mMode == Mode::File) {
            torrentFilesView = new TorrentFilesView(mFilesModel, mRpc);
            layout->addWidget(torrentFilesView, 1);
        }

        mPriorityComboBox = new QComboBox(this);
        mPriorityComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (const auto priority : priorityComboBoxItems) {
            switch (priority) {
            case libtremotesf::TorrentData::Priority::High:
                //: Torrent's loading priority
                mPriorityComboBox->addItem(qApp->translate("tremotesf", "High"));
                break;
            case libtremotesf::TorrentData::Priority::Normal:
                //: Torrent's loading priority
                mPriorityComboBox->addItem(qApp->translate("tremotesf", "Normal"));
                break;
            case libtremotesf::TorrentData::Priority::Low:
                //: Torrent's loading priority
                mPriorityComboBox->addItem(qApp->translate("tremotesf", "Low"));
                break;
            }
        }
        const auto initialPriority = [&] {
            if (const auto settings = Settings::instance(); settings->rememberAddTorrentParameters()) {
                return settings->lastAddTorrentPriority();
            }
            return libtremotesf::TorrentData::Priority::Normal;
        }();
        mPriorityComboBox->setCurrentIndex(indexOfCasted<int>(priorityComboBoxItems, initialPriority).value());

        QFormLayout* secondFormLayout = nullptr;
        if (mMode == Mode::File) {
            secondFormLayout = new QFormLayout();
            //: Combo box label
            secondFormLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), mPriorityComboBox);
            layout->addLayout(secondFormLayout);

            auto resizer = new KColumnResizer(this);
            resizer->addWidgetsFromLayout(firstFormLayout);
            resizer->addWidgetsFromLayout(secondFormLayout);
        } else {
            //: Combo box label
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), mPriorityComboBox);
        }

        mStartTorrentCheckBox = new QCheckBox(qApp->translate("tremotesf", "Start downloading after adding"), this);
        if (const auto settings = Settings::instance(); settings->rememberAddTorrentParameters()) {
            mStartTorrentCheckBox->setChecked(settings->lastAddTorrentStartAfterAdding());
        } else {
            mStartTorrentCheckBox->setChecked(mRpc->serverSettings()->data().startAddedTorrents);
        }
        layout->addWidget(mStartTorrentCheckBox);

        if (mMode == Mode::File) {
            mDeleteTorrentFileCheckBox = new QCheckBox(qApp->translate("tremotesf", "Delete .torrent file"), this);
            if (const auto settings = Settings::instance(); settings->rememberAddTorrentParameters()) {
                mDeleteTorrentFileCheckBox->setChecked(settings->lastAddTorrentDeleteTorrentFile());
            }
            layout->addWidget(mDeleteTorrentFileCheckBox);
        }

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &AddTorrentDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &AddTorrentDialog::reject);
        layout->addWidget(mDialogButtonBox);

        setMinimumSize(minimumSizeHint());

        const auto updateUi = [=, this] {
            const bool enabled = mRpc->isConnected() && (mFilesModel ? mFilesModel->isLoaded() : true);
            if (enabled) {
                mDownloadDirectoryWidget->updatePath(initialDownloadDirectory());
            }

            for (int i = 0, max = firstFormLayout->count(); i < max; ++i) {
                firstFormLayout->itemAt(i)->widget()->setEnabled(enabled);
            }

            if (torrentFilesView) {
                torrentFilesView->setEnabled(enabled);
            }

            if (secondFormLayout) {
                for (int i = 0, max = secondFormLayout->count(); i < max; ++i) {
                    secondFormLayout->itemAt(i)->widget()->setEnabled(enabled);
                }
            }

            mStartTorrentCheckBox->setEnabled(enabled);

            if (enabled) {
                if (messageWidget->isShowAnimationRunning()) {
                    messageWidget->hide();
                } else {
                    messageWidget->animatedHide();
                }
                messageWidget->animatedHide();
            } else if (mFilesModel && !mFilesModel->isLoaded()) {
                messageWidget->setMessageType(KMessageWidget::Information);
                //: Placeholder shown when torrent file is being read/parsed
                messageWidget->setText(qApp->translate("tremotesf", "Loading"));
                messageWidget->animatedShow();
            } else {
                messageWidget->setMessageType(KMessageWidget::Warning);
                //: Server connection status
                messageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
                messageWidget->animatedShow();
            }

            canAcceptUpdate();
        };

        QObject::connect(mRpc, &Rpc::connectedChanged, this, updateUi);
        QObject::connect(
            mDownloadDirectoryWidget,
            &RemoteDirectorySelectionWidget::pathChanged,
            this,
            &AddTorrentDialog::canAcceptUpdate
        );

        if (mFilesModel) {
            QObject::connect(mFilesModel, &LocalTorrentFilesModel::loadedChanged, this, [=, this] {
                if (mFilesModel->isSuccessfull()) {
                    updateUi();
                } else {
                    auto messageBox = new QMessageBox(
                        QMessageBox::Critical,
                        //: Dialog title
                        qApp->translate("tremotesf", "Error"),
                        mFilesModel->errorString(),
                        QMessageBox::Close,
                        parentWidget()
                    );
                    messageBox->setAttribute(Qt::WA_DeleteOnClose);
                    messageBox->show();
                    close();
                }
            });
        }

        // If call updateUi() right now and need to show messageWidget, it will be
        // shown without animation, because we are not visible yet
        // Call it at the next available event loop iteration
        QTimer::singleShot(0, this, updateUi);
    }

    void AddTorrentDialog::canAcceptUpdate() {
        bool can = mRpc->isConnected() && (mFilesModel ? mFilesModel->isLoaded() : true);
        if (mMode == Mode::Url && mTorrentLinkLineEdit->text().isEmpty()) {
            can = false;
        }
        if (mDownloadDirectoryWidget->path().isEmpty()) {
            can = false;
        }
        mDialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(can);
    }
}
