#include "systemcolorsprovider.h"

namespace tremotesf {
    std::unique_ptr<SystemColorsProvider> SystemColorsProvider::createInstance() {
        return std::make_unique<SystemColorsProvider>();
    }

    bool SystemColorsProvider::isDarkThemeFollowSystemSupported() {
        return false;
    }

    bool SystemColorsProvider::isAccentColorsSupported() {
        return false;
    }
}
