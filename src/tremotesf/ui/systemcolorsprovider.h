#ifndef SYSTEMCOLORSPROVIDER_H
#define SYSTEMCOLORSPROVIDER_H

#include <QColor>
#include <QObject>

#include "libtremotesf/formatters.h"

namespace tremotesf {

class SystemColorsProvider : public QObject
{
    Q_OBJECT
public:
    static SystemColorsProvider* createInstance(QObject* parent = nullptr);

    virtual bool isDarkThemeEnabled() const { return false; };

    struct AccentColors {
        QColor accentColor{};
        QColor accentColorLight1{};
        QColor accentColorDark1{};

        bool isValid() const { return accentColor.isValid(); }

        bool operator==(const AccentColors& other) const {
            return accentColor == other.accentColor &&
                accentColorLight1 == other.accentColorLight1 &&
                accentColorDark1 == other.accentColorDark1;
        }

        bool operator!=(const AccentColors& other) const {
            return !(*this == other);
        }
    };

    virtual AccentColors accentColors() const { return {}; };

    static bool isDarkThemeFollowSystemSupported();
    static bool isAccentColorsSupported();

protected:
    explicit SystemColorsProvider(QObject* parent = nullptr) : QObject{parent} {}

signals:
    void darkThemeEnabledChanged();
    void accentColorsChanged();
};

}

SPECIALIZE_FORMATTER_FOR_QDEBUG(QColor)

template<>
struct fmt::formatter<tremotesf::SystemColorsProvider::AccentColors> : libtremotesf::SimpleFormatter {
    auto format(const tremotesf::SystemColorsProvider::AccentColors& colors, format_context& ctx) FORMAT_CONST -> decltype(ctx.out()) {
        return format_to(ctx.out(), "AccentColors(accentColor={}, accentColorLight1={})", colors.accentColor, colors.accentColorLight1);
    }
};

#endif // SYSTEMCOLORSPROVIDER_H
