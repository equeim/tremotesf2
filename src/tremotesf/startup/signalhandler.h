// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SIGNALHANDLER_H
#define TREMOTESF_SIGNALHANDLER_H

#include <memory>
#include <QtGlobal>

namespace tremotesf {
    class SignalHandler final {
    public:
        SignalHandler();
        ~SignalHandler();
        Q_DISABLE_COPY_MOVE(SignalHandler)

        bool isExitRequested() const;

    private:
        class Impl;
        std::unique_ptr<Impl> mImpl;
    };
}

#endif // TREMOTESF_SIGNALHANDLER_H
