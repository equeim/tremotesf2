// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SIGNALHANDLER_H
#define TREMOTESF_SIGNALHANDLER_H

namespace tremotesf {
    namespace signalhandler {
        void initSignalHandler();
        void deinitSignalHandler();
        bool isExitRequested();
    }
}

#endif // TREMOTESF_SIGNALHANDLER_H
