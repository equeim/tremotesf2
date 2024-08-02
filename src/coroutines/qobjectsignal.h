// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_QOBJECTSIGNAL_H
#define TREMOTESF_COROUTINES_QOBJECTSIGNAL_H

#include <QObject>

#include "coroutines.h"

namespace tremotesf {
    namespace impl {
        template<typename Object, typename PointerToMemberFunction>
        class SignalAwaitable {
        public:
            inline explicit SignalAwaitable(const Object* sender, PointerToMemberFunction signal)
                : mSender(sender), mSignal(signal) {}
            inline ~SignalAwaitable() = default;
            Q_DISABLE_COPY_MOVE(SignalAwaitable)

            inline bool await_ready() { return false; }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (startAwaiting(handle)) {
                    QObject::connect(mSender, mSignal, &mReceiver, [handle] { resume(handle); });
                }
            }

            inline void await_resume() {};

        protected:
            const Object* mSender;
            PointerToMemberFunction mSignal;
            QObject mReceiver{};
        };
    }

    template<std::derived_from<QObject> Object, typename PointerToMemberFunction>
        requires(std::is_member_function_pointer_v<PointerToMemberFunction>)
    inline Coroutine<> waitForSignal(const Object* object, PointerToMemberFunction signal) {
        co_await impl::SignalAwaitable(object, signal);
    }
}

#endif // TREMOTESF_COROUTINES_QOBJECTSIGNAL_H
