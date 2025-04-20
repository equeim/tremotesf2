// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_NETWORK_H
#define TREMOTESF_COROUTINES_NETWORK_H

#include <QNetworkReply>

#include "qobjectsignal.h"

namespace tremotesf {
    namespace impl {
        class NetworkReplyAwaitable final : public SignalAwaitable<QNetworkReply, decltype(&QNetworkReply::finished)> {
        public:
            inline explicit NetworkReplyAwaitable(QNetworkReply* reply)
                : SignalAwaitable(reply, &QNetworkReply::finished) {}
            inline ~NetworkReplyAwaitable() = default;
            Q_DISABLE_COPY_MOVE(NetworkReplyAwaitable)

            inline bool await_ready() { return mSender->isFinished(); }

        private:
            QObject mReceiver{};
        };
    }

    inline impl::NetworkReplyAwaitable operator co_await(QNetworkReply& reply) {
        return impl::NetworkReplyAwaitable(&reply);
    }
}

#endif // TREMOTESF_COROUTINES_NETWORK_H
