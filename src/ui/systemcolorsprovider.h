// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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
            QColor accentColorDark2{};

            [[nodiscard]] bool isValid() const {
                return accentColor.isValid()
                       && accentColorLight1.isValid()
                       && accentColorDark1.isValid()
                       && accentColorDark2.isValid();
            }
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

template<>
struct fmt::formatter<tremotesf::SystemColorsProvider::AccentColors> : tremotesf::SimpleFormatter {
    format_context::iterator
    format(const tremotesf::SystemColorsProvider::AccentColors& colors, format_context& ctx) const {
        return fmt::format_to(
            ctx.out(),
            "AccentColors(accentColor={}, accentColorLight1={}, accentColorDark1={}, accentColorDark2={})",
            colors.accentColor.name(),
            colors.accentColorLight1.name(),
            colors.accentColorDark1.name(),
            colors.accentColorDark2.name()
        );
    }
};

#endif // SYSTEMCOLORSPROVIDER_H
