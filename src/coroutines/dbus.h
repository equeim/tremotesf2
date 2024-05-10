// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_DBUS_H
#define TREMOTESF_COROUTINES_DBUS_H

#include <QDBusPendingReply>

#include "coroutines.h"

class QDBusPendingCallWatcher;

namespace tremotesf {
    namespace impl {
        template<typename... T>
        class DbusReplyAwaitable final {
        public:
            inline explicit DbusReplyAwaitable(const QDBusPendingReply<T...>& reply) : mReply(reply) {}
            inline ~DbusReplyAwaitable() = default;
            Q_DISABLE_COPY_MOVE(DbusReplyAwaitable)

            inline bool await_ready() { return mReply.isFinished(); }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (startAwaiting(handle)) {
                    QObject::connect(&mWatcher, &QDBusPendingCallWatcher::finished, &mWatcher, [handle] {
                        resume(handle);
                    });
                }
            }
            inline QDBusPendingReply<T...> await_resume() { return mReply; };

        private:
            QDBusPendingReply<T...> mReply;
            QDBusPendingCallWatcher mWatcher{mReply};
        };
    }

    template<typename... T>
    inline impl::DbusReplyAwaitable<T...> operator co_await(const QDBusPendingReply<T...>& reply) {
        return impl::DbusReplyAwaitable(reply);
    }
}

#endif // TREMOTESF_COROUTINES_DBUS_H
