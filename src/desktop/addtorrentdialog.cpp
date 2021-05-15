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

#include "addtorrentdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCollator>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <KColumnResizer>
#include <KMessageWidget>

#include "../libtremotesf/serversettings.h"
#include "../localtorrentfilesmodel.h"
#include "../servers.h"
#include "../trpc.h"
#include "../utils.h"
#include "remotedirectoryselectionwidget.h"
#include "torrentfilesview.h"

namespace tremotesf
{
    namespace
    {
        const libtremotesf::Torrent::Priority priorityComboBoxItems[] {
            libtremotesf::Torrent::Priority::HighPriority,
            libtremotesf::Torrent::Priority::NormalPriority,
            libtremotesf::Torrent::Priority::LowPriority
        };
    }

    AddTorrentDialog::AddTorrentDialog(Rpc* rpc,
                                       const QString& url,
                                       Mode mode,
                                       QWidget* parent)
        : QDialog(parent),
          mRpc(rpc),
          mUrl(url),
          mMode(mode),
          mFilesModel(mode == Mode::File ? new LocalTorrentFilesModel(this) : nullptr)
    {
        setupUi();

        if (mFilesModel) {
            mFilesModel->load(url);
        }
    }

    QSize AddTorrentDialog::sizeHint() const
    {
        if (mMode == Mode::File) {
            return minimumSizeHint().expandedTo(QSize(448, 512));
        }
        return minimumSizeHint().expandedTo(QSize(448, 0));
    }

    void AddTorrentDialog::accept()
    {
        if (mMode == Mode::File) {
           mRpc->addTorrentFile(mUrl,
                                mDownloadDirectoryWidget->text(),
                                mFilesModel->unwantedFiles(),
                                mFilesModel->highPriorityFiles(),
                                mFilesModel->lowPriorityFiles(),
                                mFilesModel->renamedFiles(),
                                priorityComboBoxItems[mPriorityComboBox->currentIndex()],
                                mStartTorrentCheckBox->isChecked());
        } else {
            mRpc->addTorrentLink(mTorrentLinkLineEdit->text(),
                                 mDownloadDirectoryWidget->text(),
                                 priorityComboBoxItems[mPriorityComboBox->currentIndex()],
                                 mStartTorrentCheckBox->isChecked());
        }

        Servers::instance()->setCurrentServerAddTorrentDialogDirectories(mDownloadDirectoryWidget->textComboBoxItems());

        QDialog::accept();
    }

    void AddTorrentDialog::setupUi()
    {
        if (mMode == Mode::File) {
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent File"));
        } else {
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
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent file:"), mTorrentLinkLineEdit);
        } else {
            QObject::connect(mTorrentLinkLineEdit, &QLineEdit::textChanged, this, &AddTorrentDialog::canAcceptUpdate);
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent link:"), mTorrentLinkLineEdit);
        }

        mDownloadDirectoryWidget = new RemoteDirectorySelectionWidget(mRpc->serverSettings()->downloadDirectory(),
                                                                      mRpc,
                                                                      true,
                                                                      this);
        firstFormLayout->addRow(qApp->translate("tremotesf", "Download directory:"), mDownloadDirectoryWidget);

        auto freeSpaceLabel = new QLabel(this);
        auto getFreeSpace = [=](const QString& directory) {
            const auto trimmed = directory.trimmed();
            if (trimmed.isEmpty()) {
                freeSpaceLabel->hide();
                freeSpaceLabel->clear();
            } else if (mRpc->serverSettings()->canShowFreeSpaceForPath()) {
                mRpc->getFreeSpaceForPath(trimmed);
            } else if (trimmed == mRpc->serverSettings()->downloadDirectory()) {
                mRpc->getDownloadDirFreeSpace();
            } else {
                freeSpaceLabel->hide();
                freeSpaceLabel->clear();
            }
        };
        QObject::connect(mDownloadDirectoryWidget, &FileSelectionWidget::textChanged, this, [=](const auto& text) {
            getFreeSpace(text);
        });
        if (mRpc->serverSettings()->canShowFreeSpaceForPath()) {
            QObject::connect(mRpc, &Rpc::gotFreeSpaceForPath, this, [=](const QString& path, bool success, long long bytes) {
                if (path == mDownloadDirectoryWidget->text().trimmed()) {
                    if (success) {
                        freeSpaceLabel->setText(qApp->translate("tremotesf", "Free space: %1").arg(Utils::formatByteSize(bytes)));
                    } else {
                        freeSpaceLabel->setText(qApp->translate("tremotesf", "Error getting free space"));
                    }
                    freeSpaceLabel->show();
                }
            });
        } else {
            QObject::connect(mRpc, &Rpc::gotDownloadDirFreeSpace, this, [=](auto bytes) {
                if (mDownloadDirectoryWidget->text().trimmed() == mRpc->serverSettings()->downloadDirectory()) {
                    freeSpaceLabel->setText(qApp->translate("tremotesf", "Free space: %1").arg(Utils::formatByteSize(bytes)));
                    freeSpaceLabel->show();
                }
            });
        }
        getFreeSpace(mDownloadDirectoryWidget->text());
        firstFormLayout->addRow(nullptr, freeSpaceLabel);

        TorrentFilesView* torrentFilesView = nullptr;
        if (mMode == Mode::File) {
            torrentFilesView = new TorrentFilesView(mFilesModel, mRpc);
            layout->addWidget(torrentFilesView, 1);
        }

        mPriorityComboBox = new QComboBox(this);
        mPriorityComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for (libtremotesf::Torrent::Priority priority : priorityComboBoxItems) {
            switch (priority) {
            case libtremotesf::Torrent::Priority::HighPriority:
                //: Priority
                mPriorityComboBox->addItem(qApp->translate("tremotesf", "High"));
                break;
            case libtremotesf::Torrent::Priority::NormalPriority:
                //: Priority
                mPriorityComboBox->addItem(qApp->translate("tremotesf", "Normal"));
                break;
            case libtremotesf::Torrent::Priority::LowPriority:
                //: Priority
                mPriorityComboBox->addItem(qApp->translate("tremotesf", "Low"));
                break;
            }
        }
        mPriorityComboBox->setCurrentIndex(index_of_i(priorityComboBoxItems, libtremotesf::Torrent::Priority::NormalPriority));

        QFormLayout* secondFormLayout = nullptr;
        if (mMode == Mode::File) {
            secondFormLayout = new QFormLayout();
            secondFormLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), mPriorityComboBox);
            layout->addLayout(secondFormLayout);

            auto resizer = new KColumnResizer(this);
            resizer->addWidgetsFromLayout(firstFormLayout);
            resizer->addWidgetsFromLayout(secondFormLayout);
        } else {
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent priority:"), mPriorityComboBox);
        }

        mStartTorrentCheckBox = new QCheckBox(qApp->translate("tremotesf", "Start downloading after adding"), this);
        mStartTorrentCheckBox->setChecked(mRpc->serverSettings()->startAddedTorrents());
        layout->addWidget(mStartTorrentCheckBox);

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &AddTorrentDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &AddTorrentDialog::reject);
        layout->addWidget(mDialogButtonBox);

        setMinimumSize(minimumSizeHint());

        const auto updateUi = [=] {
            const bool enabled = mRpc->isConnected() && (mFilesModel ? mFilesModel->isLoaded() : true);
            if (enabled) {
                mDownloadDirectoryWidget->updateComboBox(mRpc->serverSettings()->downloadDirectory());
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
                messageWidget->setText(qApp->translate("tremotesf", "Loading"));
                messageWidget->animatedShow();
            } else {
                messageWidget->setMessageType(KMessageWidget::Warning);
                messageWidget->setText(qApp->translate("tremotesf", "Disconnected"));
                messageWidget->animatedShow();
            }

            canAcceptUpdate();
        };

        QObject::connect(mRpc, &Rpc::connectedChanged, this, updateUi);
        QObject::connect(mDownloadDirectoryWidget, &FileSelectionWidget::textChanged, this, &AddTorrentDialog::canAcceptUpdate);

        if (mFilesModel) {
            QObject::connect(mFilesModel, &LocalTorrentFilesModel::loadedChanged, this, [=] {
                if (mFilesModel->isSuccessfull()) {
                    updateUi();
                } else {
                    auto messageBox = new QMessageBox(QMessageBox::Critical,
                                                      qApp->translate("tremotesf", "Error"),
                                                      mFilesModel->errorString(),
                                                      QMessageBox::Close,
                                                      parentWidget());
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

    void AddTorrentDialog::canAcceptUpdate()
    {
        bool can = mRpc->isConnected() && (mFilesModel ? mFilesModel->isLoaded() : true);
        if (mMode == Mode::Url && mTorrentLinkLineEdit->text().isEmpty()) {
            can = false;
        }
        if (mDownloadDirectoryWidget->text().isEmpty()) {
            can = false;
        }
        mDialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(can);
    }
}
