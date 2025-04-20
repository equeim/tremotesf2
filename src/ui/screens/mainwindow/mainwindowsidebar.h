// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWSIDEBAR_H
#define TREMOTESF_MAINWINDOWSIDEBAR_H

#include <QScrollArea>

namespace tremotesf {
    class Rpc;
    class TorrentsProxyModel;

    class MainWindowSideBar final : public QScrollArea {
        Q_OBJECT

    public:
        explicit MainWindowSideBar(Rpc* rpc, TorrentsProxyModel* proxyModel, QWidget* parent = nullptr);
    };
}

#endif // TREMOTESF_MAINWINDOWSIDEBAR_H
