// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTSVIEW_H
#define TREMOTESF_TORRENTSVIEW_H

#include <QTreeView>

namespace tremotesf {
    class TorrentsProxyModel;

    class TorrentsView final : public QTreeView {
        Q_OBJECT
    public:
        TorrentsView(TorrentsProxyModel* model, QWidget* parent = nullptr);
        ~TorrentsView() override = default;
        Q_DISABLE_COPY_MOVE(TorrentsView)

        void saveState();
    };
}

#endif // TREMOTESF_TORRENTSVIEW_H
