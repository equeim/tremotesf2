#ifndef TREMOTESF_DARKTHEMEAPPLIER_H
#define TREMOTESF_DARKTHEMEAPPLIER_H

class QWindow;

namespace tremotesf {
    class SystemColorsProvider;
    void applyDarkThemeToPalette(SystemColorsProvider* systemColorsProvider);

    bool isDarkThemeFollowSystemSupported();
}

#endif // TREMOTESF_DARKTHEMEAPPLIER_H
