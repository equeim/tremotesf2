// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_EDITLABELSDIALOG_H
#define TREMOTESF_EDITLABELSDIALOG_H

#include <vector>

#include <QDialog>

namespace tremotesf {
    class Rpc;
    class Torrent;

    class EditLabelsDialog : public QDialog {
        Q_OBJECT
    public:
        explicit EditLabelsDialog(const std::vector<Torrent*> selectedTorrents, Rpc* rpc, QWidget* parent);
    };
}

#endif // TREMOTESF_EDITLABELSDIALOG_H
