// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_HOSTINFO_H
#define TREMOTESF_COROUTINES_HOSTINFO_H

#include <optional>
#include <QHostInfo>

#include "coroutines.h"

namespace tremotesf {
    namespace impl {
        class HostInfoAwaitable final {
        public:
            inline explicit HostInfoAwaitable(QString name) : mName(std::move(name)) {}
            inline ~HostInfoAwaitable() {
                if (mLookupId) {
                    QHostInfo::abortHostLookup(*mLookupId);
                }
            };
            Q_DISABLE_COPY_MOVE(HostInfoAwaitable)

            inline bool await_ready() { return false; }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (!startAwaiting(handle)) {
                    return;
                }
                mLookupId = QHostInfo::lookupHost(mName, &mReceiver, [this, handle](QHostInfo hostInfo) {
                    mLookupId = std::nullopt;
                    mResult = std::move(hostInfo);
                    resume(handle);
                });
            }
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            inline QHostInfo await_resume() { return std::move(mResult).value(); };

        private:
            QString mName;
            QObject mReceiver{};
            std::optional<int> mLookupId{};
            std::optional<QHostInfo> mResult{};
        };
    }

    inline impl::HostInfoAwaitable lookupHost(QString name) { return impl::HostInfoAwaitable(std::move(name)); }
}

#endif // TREMOTESF_COROUTINES_HOSTINFO_H
