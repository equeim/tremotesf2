// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RECOLORINGSVGICONENGINEPLUGIN_H
#define TREMOTESF_RECOLORINGSVGICONENGINEPLUGIN_H

#include <QProxyStyle>

namespace tremotesf {
    class RecoloringSvgIconStyle : public QProxyStyle {
        Q_OBJECT
    public:
        explicit RecoloringSvgIconStyle(QObject* parent);
        void drawControl(
            ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget
        ) const override;
    };
}

#endif //TREMOTESF_RECOLORINGSVGICONENGINEPLUGIN_H
