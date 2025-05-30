// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERSTATSDIALOG_H
#define TREMOTESF_SERVERSTATSDIALOG_H

#include <QDialog>

#include "coroutines/scope.h"

class QLabel;

namespace tremotesf {
    class Rpc;

    class ServerStatsDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit ServerStatsDialog(Rpc* rpc, QWidget* parent = nullptr);

    private:
        Coroutine<> getDownloadDirFreeSpace(Rpc* rpc, QLabel* freeSpaceField);

        CoroutineScope mFreeSpaceCoroutineScope{};
    };
}

#endif // TREMOTESF_SERVERSTATSDIALOG_H
