// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MACOSHELPERS_H
#define TREMOTESF_MACOSHELPERS_H

#include <QString>

namespace tremotesf {
    void hideNSApp();
    void unhideNSApp();
    bool isNSAppHidden();

    QString bundleResourcesPath();
}

#endif // TREMOTESF_MACOSHELPERS_H
