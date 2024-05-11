// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_NETWORK_H
#define TREMOTESF_COROUTINES_NETWORK_H

#include <QNetworkReply>

#include "coroutines.h"

namespace tremotesf {
    namespace impl {
        class NetworkReplyAwaitable final {
        public:
            inline explicit NetworkReplyAwaitable(QNetworkReply* reply) : mReply(reply) {}
            inline ~NetworkReplyAwaitable() = default;
            Q_DISABLE_COPY_MOVE(NetworkReplyAwaitable)

            inline bool await_ready() { return mReply->isFinished(); }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (startAwaiting(handle)) {
                    QObject::connect(mReply, &QNetworkReply::finished, &mReceiver, [handle] { resume(handle); });
                }
            }
            inline void await_resume() {};

        private:
            QNetworkReply* mReply;
            QObject mReceiver{};
        };
    }

    inline impl::NetworkReplyAwaitable operator co_await(QNetworkReply& reply) {
        return impl::NetworkReplyAwaitable(&reply);
    }
}

#endif // TREMOTESF_COROUTINES_NETWORK_H
