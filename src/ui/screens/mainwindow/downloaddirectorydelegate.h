// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_DOWNLOADDIRECTORYDELEGATE_H
#define TREMOTESF_DOWNLOADDIRECTORYDELEGATE_H

#include "ui/widgets/tooltipwhenelideddelegate.h"

namespace tremotesf {

    class DownloadDirectoryDelegate : public TooltipWhenElidedDelegate {
        Q_OBJECT
    public:
        explicit DownloadDirectoryDelegate(QObject* parent = nullptr);

        // QStyledItemDelegate interface
    protected:
        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    };

} // namespace tremotesf

#endif // TREMOTESF_DOWNLOADDIRECTORYDELEGATE_H
