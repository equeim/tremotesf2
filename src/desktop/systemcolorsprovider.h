#ifndef SYSTEMCOLORSPROVIDER_H
#define SYSTEMCOLORSPROVIDER_H

#include <memory>
#include <QColor>
#include <QObject>

#include <fmt/core.h>

namespace tremotesf {

class SystemColorsProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool darkThemeEnabled READ isDarkThemeEnabled NOTIFY darkThemeEnabledChanged)
    Q_PROPERTY(tremotesf::SystemColorsProvider::AccentColors accentColors READ accentColors NOTIFY accentColorsChanged)
public:
    explicit SystemColorsProvider(QObject* parent = nullptr) : QObject{parent} {}
    static std::unique_ptr<SystemColorsProvider> createInstance();
    virtual bool isDarkThemeEnabled() const { return false; };

    struct AccentColors {
        QColor accentColor{};
        QColor accentColorLight1{};

        bool isValid() const { return accentColor.isValid(); }

        bool operator==(const AccentColors& other) const {
            return accentColor == other.accentColor &&
                    accentColorLight1 == other.accentColorLight1;
        }

        bool operator!=(const AccentColors& other) const {
            return !(*this == other);
        }
    };

    virtual AccentColors accentColors() const { return {}; };

signals:
    void darkThemeEnabledChanged();
    void accentColorsChanged();
};

}

template<>
struct fmt::formatter<tremotesf::SystemColorsProvider::AccentColors> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const tremotesf::SystemColorsProvider::AccentColors& colors, FormatContext& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "AccentColors(accentColor={}, accentColorLight1={})", colors.accentColor, colors.accentColorLight1);
    }
};

#endif // SYSTEMCOLORSPROVIDER_H
