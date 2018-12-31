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

#ifndef TREMOTESF_ADDTORRENTDIALOG_H
#define TREMOTESF_ADDTORRENTDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;

namespace tremotesf
{
    class LocalTorrentFilesModel;
    class RemoteDirectorySelectionWidget;
    class Rpc;
    class TorrentFileParser;

    class AddTorrentDialog : public QDialog
    {
    public:
        explicit AddTorrentDialog(Rpc* rpc,
                                  const QString& filePath,
                                  TorrentFileParser* parser,
                                  LocalTorrentFilesModel* filesModel,
                                  QWidget* parent = nullptr);
        explicit AddTorrentDialog(Rpc* rpc, const QString& url, QWidget* parent = nullptr);

        QSize sizeHint() const override;
        void accept() override;

    private:
        void setupUi();
        void canAcceptUpdate();

        Rpc* mRpc;
        QString mUrl;
        bool mLocalFile;

        TorrentFileParser* mParser = nullptr;
        LocalTorrentFilesModel* mFilesModel = nullptr;

        QLineEdit* mTorrentLinkLineEdit = nullptr;
        RemoteDirectorySelectionWidget* mDownloadDirectoryWidget = nullptr;
        QComboBox* mPriorityComboBox = nullptr;
        QCheckBox* mStartTorrentCheckBox = nullptr;

        QDialogButtonBox* mDialogButtonBox = nullptr;
    };
}

#endif // TREMOTESF_ADDTORRENTDIALOG_H
