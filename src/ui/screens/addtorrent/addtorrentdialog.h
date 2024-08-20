// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTDIALOG_H
#define TREMOTESF_ADDTORRENTDIALOG_H

#include <optional>
#include <set>
#include <utility>
#include <variant>
#include <vector>

#include <QDialog>
#include <QStringList>

#include "coroutines/scope.h"
#include "ui/savewindowstatedispatcher.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

class KMessageWidget;

namespace tremotesf {
    class LocalTorrentFilesModel;
    class TorrentDownloadDirectoryDirectorySelectionWidget;
    class TorrentFilesView;
    class Rpc;

    class AddTorrentDialog final : public QDialog {
        Q_OBJECT

    public:
        struct FileParams {
            QString filePath;
        };

        struct UrlParams {
            QStringList urls;
        };

        explicit AddTorrentDialog(Rpc* rpc, std::variant<FileParams, UrlParams> params, QWidget* parent = nullptr);

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

        static AddTorrentParametersWidgets
        createAddTorrentParametersWidgets(bool forTorrentFile, QFormLayout* layout, Rpc* rpc);

    private:
        bool isAddingFile() const;

        void setupUi();
        void updateUi();
        void canAcceptUpdate();
        void saveState();

        void onDownloadDirectoryPathChanged(QString path);
        Coroutine<> getFreeSpaceForPath(QString path);

        Coroutine<> parseTorrentFile();
        void showTorrentParsingError(const QString& errorString);

        void parseMagnetLinksAndCheckIfTorrentsExist(QStringList& urls);
        bool checkIfTorrentFileExists();

        void deleteTorrentFileIfEnabled();

        Rpc* mRpc;
        std::variant<FileParams, UrlParams> mParams;

        LocalTorrentFilesModel* mFilesModel{};
        CoroutineScope mParseTorrentFileCoroutineScope{};
        std::optional<std::pair<QString, std::vector<std::set<QString>>>> mTorrentFileInfoHashAndTrackers{};

        KMessageWidget* mMessageWidget{};
        QLineEdit* mTorrentFilePathTextField{};
        QPlainTextEdit* mTorrentLinkTextField{};
        QLabel* mFreeSpaceLabel{};
        TorrentFilesView* mTorrentFilesView{};
        AddTorrentParametersWidgets mAddTorrentParametersWidgets{};

        QDialogButtonBox* mDialogButtonBox{};

        CoroutineScope mFreeSpaceCoroutineScope{};

        SaveWindowStateHandler mSaveStateHandler{this, [this] { saveState(); }};
    };
}

#endif // TREMOTESF_ADDTORRENTDIALOG_H
