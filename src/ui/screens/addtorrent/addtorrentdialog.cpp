// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "addtorrentdialog.h"

#include <array>
#include <stdexcept>

#include <QCheckBox>
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
#include "formatutils.h"
#include "magnetlinkparser.h"
#include "settings.h"
#include "stdutils.h"
#include "torrentfileparser.h"
#include "coroutines/threadpool.h"
#include "log/log.h"
#include "rpc/torrent.h"
#include "rpc/rpc.h"
#include "ui/widgets/torrentremotedirectoryselectionwidget.h"
#include "ui/widgets/torrentfilesview.h"

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

        QSize size(448, 0);
        if (mMode == Mode::File) {
            size.setHeight(512);
        }
        resize(sizeHint().expandedTo(size));

        if (mode == Mode::File) {
            mParseTorrentFileCoroutineScope.launch(parseTorrentFile());
        }
    }

    void AddTorrentDialog::accept() {
        if (mMode == Mode::Url) {
            parseMagnetLink();
        }
        if (!checkIfTorrentExists()) {
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
        }
        const auto settings = Settings::instance();
        if (settings->rememberOpenTorrentDir() && mMode == Mode::File) {
            settings->setLastOpenTorrentDirectory(QFileInfo(mUrl).path());
        }
        if (settings->rememberAddTorrentParameters()) {
            mAddTorrentParametersWidgets.saveToSettings();
        }
        deleteTorrentFileIfEnabled();
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

        mMessageWidget = new KMessageWidget(this);
        mMessageWidget->setCloseButtonVisible(false);
        mMessageWidget->hide();
        layout->addRow(mMessageWidget);

        mTorrentLinkLineEdit = new QLineEdit(mUrl, this);
        if (mMode == Mode::File) {
            mTorrentLinkLineEdit->setReadOnly(true);
            //: Input field's label
            layout->addRow(qApp->translate("tremotesf", "Torrent file:"), mTorrentLinkLineEdit);
        } else {
            //: Input field's label
            layout->addRow(qApp->translate("tremotesf", "Torrent link:"), mTorrentLinkLineEdit);
            if (!mTorrentLinkLineEdit->text().isEmpty()) {
                mTorrentLinkLineEdit->setCursorPosition(0);
            }
            QObject::connect(mTorrentLinkLineEdit, &QLineEdit::textChanged, this, &AddTorrentDialog::canAcceptUpdate);
        }

        mAddTorrentParametersWidgets = createAddTorrentParametersWidgets(mMode, layout, mRpc);

        mFreeSpaceLabel = new QLabel(this);
        QObject::connect(
            mAddTorrentParametersWidgets.downloadDirectoryWidget,
            &RemoteDirectorySelectionWidget::pathChanged,
            this,
            [=, this] { onDownloadDirectoryPathChanged(mAddTorrentParametersWidgets.downloadDirectoryWidget->path()); }
        );
        onDownloadDirectoryPathChanged(mAddTorrentParametersWidgets.downloadDirectoryWidget->path());
        layout->insertRow(
            rowForWidget(layout, mAddTorrentParametersWidgets.downloadDirectoryWidget) + 1,
            nullptr,
            mFreeSpaceLabel
        );

        if (mMode == Mode::File) {
            mTorrentFilesView = new TorrentFilesView(mFilesModel, mRpc);
            layout->insertRow(rowForWidget(layout, mFreeSpaceLabel) + 1, mTorrentFilesView);
        } else {
            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &AddTorrentDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &AddTorrentDialog::reject);
        layout->addRow(mDialogButtonBox);

        QObject::connect(mRpc, &Rpc::connectedChanged, this, &AddTorrentDialog::updateUi);
        QObject::connect(
            mAddTorrentParametersWidgets.downloadDirectoryWidget,
            &RemoteDirectorySelectionWidget::pathChanged,
            this,
            &AddTorrentDialog::canAcceptUpdate
        );

        // If call updateUi() right now and need to show messageWidget, it will be
        // shown without animation, because we are not visible yet
        // Call it at the next available event loop iteration
        QTimer::singleShot(0, this, &AddTorrentDialog::updateUi);
    }

    void AddTorrentDialog::updateUi() {
        const bool enabled = mRpc->isConnected() && (mFilesModel ? mFilesModel->isLoaded() : true);
        if (enabled) {
            // Update parameters which initial values depend on server state
            const auto parameters = getAddTorrentParameters(mRpc);
            mAddTorrentParametersWidgets.downloadDirectoryWidget->updatePath(parameters.downloadDirectory);
            mAddTorrentParametersWidgets.startTorrentCheckBox->setChecked(parameters.startAfterAdding);
        }

        for (int i = 1, max = layout()->count(); i < max; ++i) {
            auto* const widget = layout()->itemAt(i)->widget();
            if (widget) {
                widget->setEnabled(enabled);
            }
        }

        mAddTorrentParametersWidgets.startTorrentCheckBox->setEnabled(enabled);

        if (enabled) {
            if (mMessageWidget->isShowAnimationRunning()) {
                mMessageWidget->hide();
            } else {
                mMessageWidget->animatedHide();
            }
            mMessageWidget->animatedHide();
        } else if (mFilesModel && !mFilesModel->isLoaded()) {
            mMessageWidget->setMessageType(KMessageWidget::Information);
            //: Placeholder shown when torrent file is being read/parsed
            mMessageWidget->setText(qApp->translate("tremotesf", "Loading"));
            mMessageWidget->animatedShow();
        } else {
            mMessageWidget->setMessageType(KMessageWidget::Warning);
            //: Server connection status
            mMessageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
            mMessageWidget->animatedShow();
        }

        canAcceptUpdate();
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

    void AddTorrentDialog::onDownloadDirectoryPathChanged(QString path) {
        mFreeSpaceLabel->hide();
        mFreeSpaceLabel->clear();
        mFreeSpaceCoroutineScope.cancelAll();
        if (!path.isEmpty()) {
            mFreeSpaceCoroutineScope.launch(getFreeSpaceForPath(std::move(path)));
        }
    }

    Coroutine<> AddTorrentDialog::getFreeSpaceForPath(QString path) {
        const auto freeSpace = co_await mRpc->getFreeSpaceForPath(std::move(path));
        if (freeSpace) {
            mFreeSpaceLabel->setText(
                //: %1 is a amount of free space in a directory, e.g. 1 GiB
                qApp->translate("tremotesf", "Free space: %1").arg(formatutils::formatByteSize(*freeSpace))
            );
        } else {
            mFreeSpaceLabel->setText(qApp->translate("tremotesf", "Error getting free space"));
        }
        mFreeSpaceLabel->show();
    }

    Coroutine<> AddTorrentDialog::parseTorrentFile() {
        try {
            info().log("Parsing torrent file {}", mUrl);
            auto torrentFile =
                co_await runOnThreadPool([filePath = mUrl]() { return tremotesf::parseTorrentFile(filePath); });
            info().log("Parsed, result = {}", torrentFile);
            mTorrentInfoHashAndTrackers = std::pair{std::move(torrentFile.infoHashV1), std::move(torrentFile.trackers)};
            if (checkIfTorrentExists()) {
                deleteTorrentFileIfEnabled();
                close();
                co_return;
            }
            co_await mFilesModel->load(std::move(torrentFile));
            updateUi();
        } catch (const bencode::Error& e) {
            warning().logWithException(e, "Failed to parse torrent file {}", mUrl);
            showTorrentParsingError(bencodeErrorString(e.type()));
            close();
        }
    }

    void AddTorrentDialog::showTorrentParsingError(const QString& errorString) {
        auto* const messageBox = new QMessageBox(
            QMessageBox::Critical,
            //: Dialog title
            qApp->translate("tremotesf", "Error"),
            errorString,
            QMessageBox::Close,
            parentWidget()
        );
        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->show();
    }

    void AddTorrentDialog::parseMagnetLink() {
        try {
            info().log("Parsing {} as a magnet link", mTorrentLinkLineEdit->text());
            auto magnetLink = tremotesf::parseMagnetLink(QUrl(mTorrentLinkLineEdit->text()));
            info().log("Parsed, result = {}", magnetLink);
            mTorrentInfoHashAndTrackers = std::pair{std::move(magnetLink.infoHashV1), std::move(magnetLink.trackers)};
        } catch (const std::runtime_error& e) {
            warning().logWithException(e, "Failed to parse {} as a magnet link", mTorrentLinkLineEdit->text());
        }
    }

    bool AddTorrentDialog::checkIfTorrentExists() {
        if (!mTorrentInfoHashAndTrackers.has_value()) return false;
        auto& [infoHashV1, trackers] = *mTorrentInfoHashAndTrackers;
        auto* const torrent = mRpc->torrentByHash(infoHashV1);
        if (torrent) {
            askForMergingTrackers(torrent, std::move(trackers), parentWidget());
            return true;
        }
        return false;
    }

    void AddTorrentDialog::deleteTorrentFileIfEnabled() {
        if (mMode == Mode::File && mAddTorrentParametersWidgets.deleteTorrentFileGroupBox->isChecked()) {
            deleteTorrentFile(mUrl, mAddTorrentParametersWidgets.moveTorrentFileToTrashCheckBox->isChecked());
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
