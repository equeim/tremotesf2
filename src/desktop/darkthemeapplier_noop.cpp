#include "darkthemeapplier.h"

namespace tremotesf {
    void applyDarkThemeToTitleBar(QWindow*, SystemColorsProvider*) {}
    void applyDarkThemeToPalette(SystemColorsProvider*) {}
    bool isDarkThemeFollowSystemSupported() { return false; }
}
