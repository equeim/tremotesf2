// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTPROPERTIESDIALOG_H
#define TREMOTESF_TORRENTPROPERTIESDIALOG_H

#include <QDialog>

#include "ui/savewindowstatedispatcher.h"

namespace tremotesf {
    class Rpc;
    class Torrent;
    class TorrentPropertiesWidget;

    class TorrentPropertiesDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit TorrentPropertiesDialog(Torrent* torrent, Rpc* rpc, QWidget* parent = nullptr);
        ~TorrentPropertiesDialog() override = default;
        Q_DISABLE_COPY_MOVE(TorrentPropertiesDialog)

    private:
        void saveState();

        TorrentPropertiesWidget* mTorrentPropertiesWidget;

        SaveWindowStateHandler mSaveStateHandler{this, [this] { saveState(); }};
    };
}

#endif // TREMOTESF_TORRENTPROPERTIESDIALOG_H
