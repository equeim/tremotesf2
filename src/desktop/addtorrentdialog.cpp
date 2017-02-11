/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <KColumnResizer>

#include "../localtorrentfilesmodel.h"
#include "../rpc.h"
#include "../serversettings.h"
#include "../torrentfileparser.h"
#include "fileselectionwidget.h"
#include "torrentfilesview.h"

namespace tremotesf
{
    AddTorrentDialog::AddTorrentDialog(Rpc* rpc,
                                       const QString& filePath,
                                       TorrentFileParser* parser,
                                       LocalTorrentFilesModel* filesModel,
                                       QWidget* parent)
        : QDialog(parent),
          mRpc(rpc),
          mLocalFile(true),
          mFilePath(filePath),
          mParser(parser),
          mFilesModel(filesModel)
    {
        mParser->setParent(this);
        mFilesModel->setParent(this);
        setupUi();
    }

    AddTorrentDialog::AddTorrentDialog(Rpc* rpc, const QString& url, QWidget* parent)
        : QDialog(parent),
          mRpc(rpc),
          mLocalFile(false),
          mUrl(url)
    {
        setupUi();
    }

    QSize AddTorrentDialog::sizeHint() const
    {
        if (mLocalFile) {
            return layout()->totalMinimumSize().expandedTo(QSize(448, 512));
        }
        return layout()->totalMinimumSize().expandedTo(QSize(448, 0));
    }

    void AddTorrentDialog::accept()
    {
        if (mLocalFile) {
            mRpc->addTorrentFile(mParser->fileData(),
                                 mDownloadDirectoryWidget->lineEdit()->text(),
                                 mFilesModel->wantedFiles(),
                                 mFilesModel->unwantedFiles(),
                                 mFilesModel->highPriorityFiles(),
                                 mFilesModel->normalPriorityFiles(),
                                 mFilesModel->lowPriorityFiles(),
                                 1 - mPriorityComboBox->currentIndex(),
                                 mStartTorrentCheckBox->isChecked());
        } else {
            mRpc->addTorrentLink(mTorrentLinkLineEdit->text(),
                                 mDownloadDirectoryWidget->lineEdit()->text(),
                                 1 - mPriorityComboBox->currentIndex(),
                                 mStartTorrentCheckBox->isChecked());
        }
        QDialog::accept();
    }

    void AddTorrentDialog::setupUi()
    {
        if (mLocalFile) {
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent File"));
        } else {
            setWindowTitle(qApp->translate("tremotesf", "Add Torrent Link"));
        }

        auto layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        auto firstFormWidget = new QWidget(this);
        firstFormWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        auto firstFormLayout = new QFormLayout(firstFormWidget);
        firstFormLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(firstFormWidget);

        if (mLocalFile) {
            auto torrentFileLineEdit = new QLineEdit(mFilePath, this);
            torrentFileLineEdit->setReadOnly(true);
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent file:"), torrentFileLineEdit);
        } else {
            mTorrentLinkLineEdit = new QLineEdit(mUrl, this);
            QObject::connect(mTorrentLinkLineEdit, &QLineEdit::textChanged, this, &AddTorrentDialog::canAcceptUpdate);
            firstFormLayout->addRow(qApp->translate("tremotesf", "Torrent link:"), mTorrentLinkLineEdit);
        }

        mDownloadDirectoryWidget = new FileSelectionWidget(true,
                                                           QString(),
                                                           this);
        mDownloadDirectoryWidget->lineEdit()->setText(mRpc->serverSettings()->downloadDirectory());
        mDownloadDirectoryWidget->selectionButton()->setEnabled(mRpc->isLocal());
        QObject::connect(mDownloadDirectoryWidget->lineEdit(), &QLineEdit::textChanged, this, &AddTorrentDialog::canAcceptUpdate);
        firstFormLayout->addRow(qApp->translate("tremotesf", "Download directory:"), mDownloadDirectoryWidget);

        if (mLocalFile) {
            layout->addWidget(new TorrentFilesView(mFilesModel), 1);
        }

        mPriorityComboBox = new QComboBox(this);
        mPriorityComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mPriorityComboBox->addItems({qApp->translate("tremotesf", "High"),
                                     qApp->translate("tremotesf", "Normal"),
                                     qApp->translate("tremotesf", "Low")});
        mPriorityComboBox->setCurrentIndex(1);

        if (mLocalFile) {
            auto secondFormLayout = new QFormLayout();
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

        /*mTrashTorrentFileCheckBox = new QCheckBox(qApp->translate("tremotesf", "Trash .torrent file"), this);
        mTrashTorrentFileCheckBox->setChecked(mRpc->serverSettings()->trashTorrentFiles());
        layout->addWidget(mTrashTorrentFileCheckBox);*/

        mDialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::accepted, this, &AddTorrentDialog::accept);
        QObject::connect(mDialogButtonBox, &QDialogButtonBox::rejected, this, &AddTorrentDialog::reject);
        layout->addWidget(mDialogButtonBox);

        canAcceptUpdate();
    }

    void AddTorrentDialog::canAcceptUpdate()
    {
        bool can = true;
        if (!mLocalFile && mTorrentLinkLineEdit->text().isEmpty()) {
            can = false;
        }
        if (mDownloadDirectoryWidget->lineEdit()->text().isEmpty()) {
            can = false;
        }
        mDialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(can);
    }
}
