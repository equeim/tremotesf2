// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTDIALOG_H
#define TREMOTESF_ADDTORRENTDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;

namespace tremotesf {
    class LocalTorrentFilesModel;
    class TorrentDownloadDirectoryDirectorySelectionWidget;
    class TorrentFilesView;
    class Rpc;

    class AddTorrentDialog final : public QDialog {
        Q_OBJECT

    public:
        enum class Mode { File, Url };

        explicit AddTorrentDialog(Rpc* rpc, const QString& url, Mode mode, QWidget* parent = nullptr);

        QSize sizeHint() const override;
        void accept() override;

    private:
        QString initialDownloadDirectory();
        void setupUi();
        void canAcceptUpdate();

        Rpc* mRpc;
        QString mUrl;
        Mode mMode;

        LocalTorrentFilesModel* mFilesModel{};

        QLineEdit* mTorrentLinkLineEdit{};
        TorrentDownloadDirectoryDirectorySelectionWidget* mDownloadDirectoryWidget{};
        TorrentFilesView* mTorrentFilesView{};
        QComboBox* mPriorityComboBox{};
        QCheckBox* mStartTorrentCheckBox{};
        QCheckBox* mDeleteTorrentFileCheckBox{};

        QDialogButtonBox* mDialogButtonBox{};
    };
}

#endif // TREMOTESF_ADDTORRENTDIALOG_H
