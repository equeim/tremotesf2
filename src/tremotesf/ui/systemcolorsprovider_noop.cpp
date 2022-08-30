#include "systemcolorsprovider.h"

namespace tremotesf {
    SystemColorsProvider* SystemColorsProvider::createInstance(QObject* parent) {
        return new SystemColorsProvider(parent);
    }

    bool SystemColorsProvider::isDarkThemeFollowSystemSupported() {
        return false;
    }

    bool SystemColorsProvider::isAccentColorsSupported() {
        return false;
    }
}
