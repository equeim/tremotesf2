// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "addtorrentdialog.h"

#include <array>
#include <stdexcept>

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <KMessageWidget>

#include "addtorrenthelpers.h"
#include "settings.h"
#include "stdutils.h"
#include "formatutils.h"
#include "log/log.h"
#include "rpc/torrent.h"
#include "rpc/rpc.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"
#include "ui/widgets/torrentfilesview.h"

#include "droppedtorrents.h"
#include "localtorrentfilesmodel.h"

namespace tremotesf {
    namespace {
        constexpr std::array priorityComboBoxItems{
            TorrentData::Priority::High, TorrentData::Priority::Normal, TorrentData::Priority::Low
        };

        TorrentData::Priority priorityFromComboBoxIndex(int index) {
            if (index == -1) {
                return TorrentData::Priority::Normal;
            }
            return priorityComboBoxItems.at(static_cast<size_t>(index));
        }

        int rowForWidget(QFormLayout* layout, QWidget* widget) {
            int row{};
            QFormLayout::ItemRole role{};
            layout->getWidgetPosition(widget, &row, &role);
            if (row == -1) {
                throw std::logic_error(fmt::format("Did not find widget {} in layout {}", *widget, *layout));
            }
            return row;
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

        QSize size(448, 0);
        if (mMode == Mode::File) {
            size.setHeight(512);
        }
        resize(sizeHint().expandedTo(size));
    }

    void AddTorrentDialog::accept() {
        if (mMode == Mode::File) {
            mRpc->addTorrentFile(
                mUrl,
                mAddTorrentParametersWidgets.downloadDirectoryWidget->path(),
                mFilesModel->unwantedFiles(),
                mFilesModel->highPriorityFiles(),
                mFilesModel->lowPriorityFiles(),
                mFilesModel->renamedFiles(),
                priorityFromComboBoxIndex(mAddTorrentParametersWidgets.priorityComboBox->currentIndex()),
                mAddTorrentParametersWidgets.startTorrentCheckBox->isChecked()
            );
        } else {
            mRpc->addTorrentLink(
                mTorrentLinkLineEdit->text(),
                mAddTorrentParametersWidgets.downloadDirectoryWidget->path(),
                priorityFromComboBoxIndex(mAddTorrentParametersWidgets.priorityComboBox->currentIndex()),
                mAddTorrentParametersWidgets.startTorrentCheckBox->isChecked()
            );
        }

        const auto settings = Settings::instance();
        if (settings->rememberOpenTorrentDir() && mMode == Mode::File) {
            settings->setLastOpenTorrentDirectory(QFileInfo(mUrl).path());
        }
        if (settings->rememberAddTorrentParameters()) {
            mAddTorrentParametersWidgets.saveToSettings();
        }
        if (mMode == Mode::File && mAddTorrentParametersWidgets.deleteTorrentFileGroupBox->isChecked()) {
            deleteTorrentFile(mUrl, mAddTorrentParametersWidgets.moveTorrentFileToTrashCheckBox->isChecked());
        }

        QDialog::accept();
    }

    AddTorrentDialog::AddTorrentParametersWidgets
    AddTorrentDialog::createAddTorrentParametersWidgets(Mode mode, QFormLayout* layout, Rpc* rpc) {
        const auto parameters = getAddTorrentParameters(rpc);

        auto* const downloadDirectoryWidget = new TorrentDownloadDirectoryDirectorySelectionWidget{};
        downloadDirectoryWidget->setup(parameters.downloadDirectory, rpc);
        //: Input field's label
        layout->addRow(qApp->translate("tremotesf", "Download directory:"), downloadDirectoryWidget);

        auto* const priorityComboBox = new QComboBox{};
        priorityComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (const auto priority : priorityComboBoxItems) {
            switch (priority) {
            case TorrentData::Priority::High:
                //: Torrent's loading priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "High"));
                break;
            case TorrentData::Priority::Normal:
                //: Torrent's loading priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "Normal"));
                break;
            case TorrentData::Priority::Low:
                //: Torrent's loading priority
                priorityComboBox->addItem(qApp->translate("tremotesf", "Low"));
                break;
            }
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        priorityComboBox->setCurrentIndex(indexOfCasted<int>(priorityComboBoxItems, parameters.priority).value());
        //: Combo box label
        layout->addRow(qApp->translate("tremotesf", "Torrent priority:"), priorityComboBox);

        auto* const startTorrentCheckBox =
            new QCheckBox(qApp->translate("tremotesf", "Start downloading after adding"));
        startTorrentCheckBox->setChecked(parameters.startAfterAdding);
        layout->addRow(startTorrentCheckBox);

        QGroupBox* deleteTorrentFileGroupBox{};
        QCheckBox* moveTorrentFileToTrashCheckBox{};
        if (mode == Mode::File) {
            deleteTorrentFileGroupBox = new QGroupBox(qApp->translate("tremotesf", "Delete .torrent file"));
            layout->addWidget(deleteTorrentFileGroupBox);
            deleteTorrentFileGroupBox->setCheckable(true);
            const auto groupBoxLayout = new QVBoxLayout(deleteTorrentFileGroupBox);
            moveTorrentFileToTrashCheckBox = new QCheckBox(qApp->translate("tremotesf", "Move .torrent file to trash"));
            groupBoxLayout->addWidget(moveTorrentFileToTrashCheckBox);

            deleteTorrentFileGroupBox->setChecked(parameters.deleteTorrentFile);
            moveTorrentFileToTrashCheckBox->setChecked(parameters.moveTorrentFileToTrash);
            layout->addRow(deleteTorrentFileGroupBox);
        }

        return {
            .downloadDirectoryWidget = downloadDirectoryWidget,
            .priorityComboBox = priorityComboBox,
            .startTorrentCheckBox = startTorrentCheckBox,
            .deleteTorrentFileGroupBox = deleteTorrentFileGroupBox,
            .moveTorrentFileToTrashCheckBox = moveTorrentFileToTrashCheckBox
        };
    }

    void AddTorrentDialog::setupUi() {
        if (mMode == Mode::File) {
            //: Dialog title
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent File"));
        } else {
            //: Dialog title
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent Link"));
        }

        auto layout = new QFormLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto messageWidget = new KMessageWidget(this);
        messageWidget->setCloseButtonVisible(false);
        messageWidget->hide();
        layout->addRow(messageWidget);

        mTorrentLinkLineEdit = new QLineEdit(mUrl, this);
        if (mMode == Mode::File) {
            mTorrentLinkLineEdit->setReadOnly(true);
            //: Input field's label
            layout->addRow(qApp->translate("tremotesf", "Torrent file:"), mTorrentLinkLineEdit);
        } else {
            //: Input field's label
            layout->addRow(qApp->translate("tremotesf", "Torrent link:"), mTorrentLinkLineEdit);
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

        mAddTorrentParametersWidgets = createAddTorrentParametersWidgets(mMode, layout, mRpc);

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
        QObject::connect(
            mAddTorrentParametersWidgets.downloadDirectoryWidget,
            &RemoteDirectorySelectionWidget::pathChanged,
            this,
            [=, this] { getFreeSpace(mAddTorrentParametersWidgets.downloadDirectoryWidget->path()); }
        );
        if (mRpc->serverSettings()->data().canShowFreeSpaceForPath()) {
            QObject::connect(
                mRpc,
                &Rpc::gotFreeSpaceForPath,
                this,
                [=, this](const QString& path, bool success, long long bytes) {
                    if (path == mAddTorrentParametersWidgets.downloadDirectoryWidget->path()) {
                        if (success) {
                            freeSpaceLabel->setText(
                                //: %1 is a amount of free space in a directory, e.g. 1 GiB
                                qApp->translate("tremotesf", "Free space: %1").arg(formatutils::formatByteSize(bytes))
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
                if (mAddTorrentParametersWidgets.downloadDirectoryWidget->path() ==
                    mRpc->serverSettings()->data().downloadDirectory) {
                    freeSpaceLabel->setText(
                        qApp->translate("tremotesf", "Free space: %1").arg(formatutils::formatByteSize(bytes))
                    );
                    freeSpaceLabel->show();
                }
            });
        }
        getFreeSpace(mAddTorrentParametersWidgets.downloadDirectoryWidget->path());
        layout->insertRow(
            rowForWidget(layout, mAddTorrentParametersWidgets.downloadDirectoryWidget) + 1,
            nullptr,
            freeSpaceLabel
        );

        if (mMode == Mode::File) {
            mTorrentFilesView = new TorrentFilesView(mFilesModel, mRpc);
            layout->insertRow(rowForWidget(layout, freeSpaceLabel) + 1, mTorrentFilesView);
        } else {
            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &AddTorrentDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &AddTorrentDialog::reject);
        layout->addRow(mDialogButtonBox);

        const auto updateUi = [=, this] {
            const bool enabled = mRpc->isConnected() && (mFilesModel ? mFilesModel->isLoaded() : true);
            if (enabled) {
                // Update parameters which initial values depend on server state
                const auto parameters = getAddTorrentParameters(mRpc);
                mAddTorrentParametersWidgets.downloadDirectoryWidget->updatePath(parameters.downloadDirectory);
                mAddTorrentParametersWidgets.startTorrentCheckBox->setChecked(parameters.startAfterAdding);
            }

            for (int i = 1, max = layout->count(); i < max; ++i) {
                auto* const widget = layout->itemAt(i)->widget();
                if (widget) {
                    widget->setEnabled(enabled);
                }
            }

            mAddTorrentParametersWidgets.startTorrentCheckBox->setEnabled(enabled);

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
            mAddTorrentParametersWidgets.downloadDirectoryWidget,
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
        if (mAddTorrentParametersWidgets.downloadDirectoryWidget->path().isEmpty()) {
            can = false;
        }
        mDialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(can);
    }

    void AddTorrentDialog::saveState() {
        if (mTorrentFilesView) {
            mTorrentFilesView->saveState();
        }
    }

    void AddTorrentDialog::AddTorrentParametersWidgets::reset(Rpc* rpc) const {
        const auto initialParameters = getInitialAddTorrentParameters(rpc);
        downloadDirectoryWidget->updatePath(initialParameters.downloadDirectory);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        priorityComboBox->setCurrentIndex(indexOfCasted<int>(priorityComboBoxItems, initialParameters.priority).value()
        );
        startTorrentCheckBox->setChecked(initialParameters.startAfterAdding);
        if (deleteTorrentFileGroupBox) {
            deleteTorrentFileGroupBox->setChecked(initialParameters.deleteTorrentFile);
            moveTorrentFileToTrashCheckBox->setChecked(initialParameters.moveTorrentFileToTrash);
        }
    }

    void AddTorrentDialog::AddTorrentParametersWidgets::saveToSettings() const {
        downloadDirectoryWidget->saveDirectories();
        auto* const settings = Settings::instance();
        settings->setLastAddTorrentPriority(priorityFromComboBoxIndex(priorityComboBox->currentIndex()));
        settings->setLastAddTorrentStartAfterAdding(startTorrentCheckBox->isChecked());
        if (deleteTorrentFileGroupBox) {
            settings->setLastAddTorrentDeleteTorrentFile(deleteTorrentFileGroupBox->isChecked());
            settings->setLastAddTorrentMoveTorrentFileToTrash(moveTorrentFileToTrashCheckBox->isChecked());
        }
    }

}
