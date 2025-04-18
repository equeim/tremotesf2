// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QStyleFactory>
#include <QStyleOptionProgressBar>

#include "progressbardelegate.h"

#include "ui/stylehelpers.h"
#include "target_os.h"

namespace tremotesf {
    namespace {
        [[maybe_unused]] QStyle* fusionStyle() {
            static QStyle* const style = [] {
                const auto s = QStyleFactory::create("fusion");
                if (!s) {
                    throw std::runtime_error("Failed to create Fusion style");
                }
                s->setParent(qApp);
                return s;
            }();
            return style;
        }
    }

    void
    ProgressBarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        auto* style = opt.widget ? opt.widget->style() : QApplication::style();
        if constexpr (targetOs == TargetOs::UnixMacOS) {
            if (determineStyle(style) == KnownStyle::macOS) {
                style = fusionStyle();
            }
        }

        // Draw background
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

        const int horizontalMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin);
        const int verticalMargin = style->pixelMetric(QStyle::PM_FocusFrameVMargin);

        QStyleOptionProgressBar progressBar{};
        progressBar.rect =
            opt.rect.marginsRemoved(QMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin));
        progressBar.minimum = 0;
        progressBar.maximum = 100;
        const auto progress = index.data(mProgressRole).toDouble();
        progressBar.progress = static_cast<int>(progress * 100);
        if (progressBar.progress < 0) {
            progressBar.progress = 0;
        } else if (progressBar.progress > 100) {
            progressBar.progress = 100;
        }
        progressBar.state = opt.state | QStyle::State_Horizontal;
        progressBar.text = opt.text;
        progressBar.textVisible = true;
        progressBar.palette = opt.palette;
        // Sometimes this is out of sync
        if (opt.widget && opt.widget->isActiveWindow()) {
            progressBar.palette.setCurrentColorGroup(QPalette::Active);
        }
        style->drawControl(QStyle::CE_ProgressBar, &progressBar, painter, opt.widget);
    }
}
