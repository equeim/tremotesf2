#include "filemanagerlauncher.h"

namespace tremotesf::impl {
    FileManagerLauncher* FileManagerLauncher::createInstance() {
        return new FileManagerLauncher();
    }
}
