#include "systemcolorsprovider.h"

namespace tremotesf {
    std::unique_ptr<SystemColorsProvider> SystemColorsProvider::createInstance() {
        return std::make_unique<SystemColorsProvider>();
    }
}
