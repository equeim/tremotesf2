// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemanagerlauncher.h"

namespace tremotesf::impl {
    FileManagerLauncher* FileManagerLauncher::createInstance() { return new FileManagerLauncher(); }
}
