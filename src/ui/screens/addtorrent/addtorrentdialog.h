// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTDIALOG_H
#define TREMOTESF_ADDTORRENTDIALOG_H

#include <QDialog>

#include "coroutines/scope.h"
#include "ui/savewindowstatedispatcher.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QFormLayout;
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

        void accept() override;

        struct AddTorrentParametersWidgets {
            TorrentDownloadDirectoryDirectorySelectionWidget* downloadDirectoryWidget;
            QComboBox* priorityComboBox;
            QCheckBox* startTorrentCheckBox;
            QGroupBox* deleteTorrentFileGroupBox;
            QCheckBox* moveTorrentFileToTrashCheckBox;

            void reset(Rpc* rpc) const;
            void saveToSettings() const;
        };

        static AddTorrentParametersWidgets createAddTorrentParametersWidgets(Mode mode, QFormLayout* layout, Rpc* rpc);

    private:
        void setupUi();
        void canAcceptUpdate();
        void saveState();
        void onDownloadDirectoryPathChanged(QString path);
        Coroutine<> getFreeSpaceForPath(QString path);

        Rpc* mRpc;
        QString mUrl;
        Mode mMode;

        LocalTorrentFilesModel* mFilesModel{};

        QLineEdit* mTorrentLinkLineEdit{};
        QLabel* mFreeSpaceLabel{};
        TorrentFilesView* mTorrentFilesView{};
        AddTorrentParametersWidgets mAddTorrentParametersWidgets{};

        QDialogButtonBox* mDialogButtonBox{};

        CoroutineScope mFreeSpaceCoroutineScope{};

        SaveWindowStateHandler mSaveStateHandler{this, [this] { saveState(); }};
    };
}

#endif // TREMOTESF_ADDTORRENTDIALOG_H
