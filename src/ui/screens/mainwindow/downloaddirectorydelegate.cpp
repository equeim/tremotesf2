// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "downloaddirectorydelegate.h"
#include "settings.h"

namespace tremotesf {

    DownloadDirectoryDelegate::DownloadDirectoryDelegate(QObject* parent) : TooltipWhenElidedDelegate(parent) {
        const auto settings = Settings::instance();
        mAlwaysShowTooltip = !settings->get_displayFullDownloadDirectoryPath();
        QObject::connect(settings, &Settings::displayFullDownloadDirectoryPathChanged, this, [this, settings] {
            mAlwaysShowTooltip = !settings->get_displayFullDownloadDirectoryPath();
        });
    }

    void DownloadDirectoryDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const {
        TooltipWhenElidedDelegate::initStyleOption(option, index);
        option->textElideMode = Qt::ElideMiddle;
    }

} // namespace tremotesf
