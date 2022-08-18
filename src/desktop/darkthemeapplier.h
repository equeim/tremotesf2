#ifndef DARKTHEMEAPPLIER_H
#define DARKTHEMEAPPLIER_H

class QWindow;

namespace tremotesf {
    class SystemColorsProvider;
    void applyDarkThemeToPalette(SystemColorsProvider* systemColorsProvider);

    bool isDarkThemeFollowSystemSupported();
}

#endif // DARKTHEMEAPPLIER_H
