// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_DBUS_H
#define TREMOTESF_COROUTINES_DBUS_H

#include <QDBusPendingReply>

#include "qobjectsignal.h"

class QDBusPendingCallWatcher;

namespace tremotesf {
    namespace impl {
        template<typename... T>
        class DbusReplyAwaitable final
            : public SignalAwaitable<QDBusPendingCallWatcher, decltype(&QDBusPendingCallWatcher::finished)> {
        public:
            inline explicit DbusReplyAwaitable(const QDBusPendingReply<T...>& reply)
                : SignalAwaitable(nullptr, &QDBusPendingCallWatcher::finished), mReply(reply) {
                mSender = &mWatcher;
            }
            inline ~DbusReplyAwaitable() = default;
            Q_DISABLE_COPY_MOVE(DbusReplyAwaitable)

            inline bool await_ready() { return mReply.isFinished(); }

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
