// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTDIALOG_H
#define TREMOTESF_ADDTORRENTDIALOG_H

#include <QDialog>
#include "ui/savewindowstatedispatcher.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
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
        void saveState();

        Rpc* mRpc;
        QString mUrl;
        Mode mMode;

        LocalTorrentFilesModel* mFilesModel{};

        QLineEdit* mTorrentLinkLineEdit{};
        TorrentDownloadDirectoryDirectorySelectionWidget* mDownloadDirectoryWidget{};
        TorrentFilesView* mTorrentFilesView{};
        QComboBox* mPriorityComboBox{};
        QCheckBox* mStartTorrentCheckBox{};
        QGroupBox* mDeleteTorrentFileGroupBox{};
        QCheckBox* mMoveTorrentFileToTrashCheckBox{};

        QDialogButtonBox* mDialogButtonBox{};

        SaveWindowStateHandler mSaveStateHandler{this, [this] { saveState(); }};
    };
}

#endif // TREMOTESF_ADDTORRENTDIALOG_H
