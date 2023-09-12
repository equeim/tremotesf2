// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTSVIEW_H
#define TREMOTESF_TORRENTSVIEW_H

#include "ui/widgets/basetreeview.h"

namespace tremotesf {
    class TorrentsProxyModel;

    class TorrentsView final : public BaseTreeView {
        Q_OBJECT
    public:
        TorrentsView(TorrentsProxyModel* model, QWidget* parent = nullptr);
        ~TorrentsView() override;
        Q_DISABLE_COPY_MOVE(TorrentsView)
    };
}

#endif // TREMOTESF_TORRENTSVIEW_H
