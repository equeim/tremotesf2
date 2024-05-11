// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_TIMER_H
#define TREMOTESF_COROUTINES_TIMER_H

#include <chrono>
#include <QTimer>

#include "coroutines.h"

namespace tremotesf {
    namespace impl {
        class TimerAwaitable final {
        public:
            inline explicit TimerAwaitable(std::chrono::milliseconds duration) : mDuration(duration) {}
            inline ~TimerAwaitable() = default;
            Q_DISABLE_COPY_MOVE(TimerAwaitable)

            inline bool await_ready() { return mDuration.count() == 0; }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (startAwaiting(handle)) {
                    QTimer::singleShot(mDuration, &mReceiver, [handle] { resume(handle); });
                }
            }
            inline void await_resume() {};

        private:
            std::chrono::milliseconds mDuration;
            QObject mReceiver{};
        };
    }

    inline impl::TimerAwaitable waitFor(std::chrono::milliseconds duration) { return impl::TimerAwaitable(duration); }
}

#endif // TREMOTESF_COROUTINES_TIMER_H
