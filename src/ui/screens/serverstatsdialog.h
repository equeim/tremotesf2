// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERSTATSDIALOG_H
#define TREMOTESF_SERVERSTATSDIALOG_H

#include <QDialog>

namespace tremotesf {
    class Rpc;

    class ServerStatsDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit ServerStatsDialog(Rpc* rpc, QWidget* parent = nullptr);
        QSize sizeHint() const override;
    };
}

#endif // TREMOTESF_SERVERSTATSDIALOG_H
