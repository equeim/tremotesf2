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

#ifndef TREMOTESF_ADDTORRENTDIALOG_H
#define TREMOTESF_ADDTORRENTDIALOG_H

#include <QDialog>
#include <QVariantMap>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;

namespace tremotesf
{
    class FileSelectionWidget;
    class LocalTorrentFilesModel;
    class Rpc;

    class AddTorrentDialog : public QDialog
    {
    public:
        explicit AddTorrentDialog(Rpc* rpc, const QString& filePath, const QVariantMap& parseResult, QWidget* parent = nullptr);
        explicit AddTorrentDialog(Rpc* rpc, const QString& url, QWidget* parent = nullptr);

        QSize sizeHint() const override;
        void accept() override;
    private:
        void setupUi();
        void canAcceptUpdate();

        Rpc* mRpc;

        bool mLocalFile;

        QString mFilePath;
        LocalTorrentFilesModel* mFilesModel;

        QString mUrl;

        FileSelectionWidget* mTorrentFileWidget;
        QLineEdit* mTorrentLinkLineEdit;

        FileSelectionWidget* mDownloadDirectoryWidget;
        QComboBox* mPriorityComboBox;
        QCheckBox* mStartTorrentCheckBox;
        //QCheckBox* mTrashTorrentFileCheckBox;

        QDialogButtonBox* mDialogButtonBox;
    };
}

#endif // TREMOTESF_ADDTORRENTDIALOG_H
