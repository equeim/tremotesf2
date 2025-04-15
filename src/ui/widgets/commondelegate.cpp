// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commondelegate.h"

#include <stdexcept>

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionProgressBar>
#include <QToolTip>

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

        bool isTextElided(const QString& text, const QStyleOptionViewItem& option) {
            const QFontMetrics metrics(option.font);
            const int textWidth = metrics.horizontalAdvance(text);
            const auto style = option.widget ? option.widget->style() : qApp->style();
            QRect textRect(style->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget));
            const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;
            textRect.adjust(textMargin, 0, -textMargin, 0);

            return textWidth > textRect.width();
        }
    }

    void CommonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        auto* style = opt.widget ? opt.widget->style() : QApplication::style();
        if constexpr (targetOs == TargetOs::UnixMacOS) {
            if (determineStyle(style) == KnownStyle::macOS) {
                style = fusionStyle();
            }
        }

        if (!(mParams.progressBarColumn.has_value() && mParams.progressRole.has_value() &&
              index.column() == mParams.progressBarColumn)) {
            if (mParams.textElideModeRole.has_value()) {
                opt.textElideMode = index.data(*mParams.textElideModeRole).value<Qt::TextElideMode>();
            }
            // Not progress bar
            style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
            return;
        }

        // Progress bar

        // Draw background
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

        const int horizontalMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin);
        const int verticalMargin = style->pixelMetric(QStyle::PM_FocusFrameVMargin);

        QStyleOptionProgressBar progressBar{};
        progressBar.rect =
            opt.rect.marginsRemoved(QMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin));
        progressBar.minimum = 0;
        progressBar.maximum = 100;
        const auto progress = index.data(*mParams.progressRole).toDouble();
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

    bool CommonDelegate::helpEvent(
        QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index
    ) {
        if (event->type() != QEvent::ToolTip) {
            return QStyledItemDelegate::helpEvent(event, view, option, index);
        }

        if (!index.isValid()) {
            event->ignore();
            return false;
        }

        const auto tooltip = displayText(index.data(Qt::ToolTipRole), QLocale{});
        if (tooltip.isEmpty()) {
            event->ignore();
            return false;
        }

        if (QToolTip::isVisible() && QToolTip::text() == tooltip) {
            event->accept();
            return true;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        const bool alwaysShowTooltip =
            mParams.alwaysShowTooltipRole.has_value() ? index.data(*mParams.alwaysShowTooltipRole).toBool() : false;

        if (!alwaysShowTooltip) {
            // Get real item rect
            const QRect intersected(opt.rect.intersected(view->viewport()->rect()));
            opt.rect.setLeft(intersected.left());
            opt.rect.setRight(intersected.right());

            // Show tooltip only if display text is elided
            if (!isTextElided(displayText(index.data(Qt::DisplayRole), opt.locale), opt)) {
                event->ignore();
                return false;
            }
        }

        QToolTip::showText(event->globalPos(), tooltip, view->viewport(), opt.rect);
        event->accept();
        return true;
    }
}
