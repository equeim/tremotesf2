// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSTEMCOLORSPROVIDER_H
#define SYSTEMCOLORSPROVIDER_H

#include <QColor>
#include <QObject>

#include "log/formatters.h"

namespace tremotesf {

    class SystemColorsProvider : public QObject {
        Q_OBJECT

    public:
        static SystemColorsProvider* createInstance(QObject* parent = nullptr);

        virtual bool isDarkThemeEnabled() const { return false; };

        struct AccentColors {
            QColor accentColor{};
            QColor accentColorLight1{};
            QColor accentColorDark1{};

            [[nodiscard]] bool isValid() const { return accentColor.isValid(); }
            [[nodiscard]] bool operator==(const AccentColors&) const = default;
        };

        virtual AccentColors accentColors() const { return {}; };

    protected:
        explicit SystemColorsProvider(QObject* parent = nullptr) : QObject{parent} {}

    signals:
        void darkThemeEnabledChanged();
        void accentColorsChanged();
    };

}

SPECIALIZE_FORMATTER_FOR_QDEBUG(QColor)

template<>
struct fmt::formatter<tremotesf::SystemColorsProvider::AccentColors> : tremotesf::SimpleFormatter {
    format_context::iterator format(const tremotesf::SystemColorsProvider::AccentColors& colors, format_context& ctx)
        FORMAT_CONST {
        return fmt::format_to(
            ctx.out(),
            "AccentColors(accentColor={}, accentColorLight1={})",
            colors.accentColor,
            colors.accentColorLight1
        );
    }
};

#endif // SYSTEMCOLORSPROVIDER_H
