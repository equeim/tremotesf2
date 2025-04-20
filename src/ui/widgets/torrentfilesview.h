// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESVIEW_H
#define TREMOTESF_TORRENTFILESVIEW_H

#include <functional>

#include "basetreeview.h"

namespace tremotesf {
    class BaseTorrentFilesModel;
    class LocalTorrentFilesModel;
    class Rpc;
    class TorrentFilesModel;
    class TorrentFilesProxyModel;

    class TorrentFilesView final : public BaseTreeView {
        Q_OBJECT

    public:
        explicit TorrentFilesView(LocalTorrentFilesModel* model, Rpc* rpc, QWidget* parent = nullptr);
        explicit TorrentFilesView(TorrentFilesModel* model, Rpc* rpc, QWidget* parent = nullptr);
        Q_DISABLE_COPY_MOVE(TorrentFilesView)

        static void showFileRenameDialog(
            const QString& fileName, QWidget* parent, const std::function<void(const QString&)>& onAccepted
        );

        void saveState();

    private:
        void init();
        void onModelReset();
        void showContextMenu(QPoint pos);

        bool mLocalFile;
        BaseTorrentFilesModel* mModel;
        TorrentFilesProxyModel* mProxyModel;
        Rpc* mRpc;
    };
}

#endif // TREMOTESF_TORRENTFILESVIEW_H
