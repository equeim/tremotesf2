// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_WAITALL_H
#define TREMOTESF_COROUTINES_WAITALL_H

#include <list>

#include "coroutines.h"

namespace tremotesf {
    namespace impl {
        class MultipleCoroutinesAwaiter final {
        public:
            inline explicit MultipleCoroutinesAwaiter(std::list<impl::StandaloneCoroutine> coroutines)
                : mCoroutines(std::move(coroutines)) {}
            inline ~MultipleCoroutinesAwaiter() = default;
            Q_DISABLE_COPY_MOVE(MultipleCoroutinesAwaiter)

            inline bool await_ready() { return mCoroutines.empty(); }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> parentCoroutineHandle) {
                mParentCoroutineHandle = parentCoroutineHandle;
                if constexpr (std::derived_from<Promise, CoroutinePromiseBase>) {
                    mParentCoroutinePromise = &parentCoroutineHandle.promise();
                }
                awaitSuspendImpl();
            }

            void await_resume();

        private:
            void awaitSuspendImpl();
            void onCoroutineCompleted(impl::StandaloneCoroutine* coroutine, std::exception_ptr unhandledException);
            void onAllCoroutinesCompleted();
            void cancelAll();

            std::list<impl::StandaloneCoroutine> mCoroutines;
            std::coroutine_handle<> mParentCoroutineHandle{};
            CoroutinePromiseBase* mParentCoroutinePromise{};
            std::exception_ptr mUnhandledException{};
            bool mCancellingCoroutines{};
        };
    }

    inline impl::MultipleCoroutinesAwaiter waitAll(std::vector<Coroutine<>>&& coroutines) {
        std::list<impl::StandaloneCoroutine> list{};
        for (auto&& coroutine : std::move(coroutines)) {
            list.emplace_back(std::move(coroutine));
        }
        return impl::MultipleCoroutinesAwaiter(std::move(list));
    }

    template<std::same_as<Coroutine<>>... Coroutines>
        requires(sizeof...(Coroutines) != 0)
    inline impl::MultipleCoroutinesAwaiter waitAll(Coroutines&&... coroutines) {
        std::list<impl::StandaloneCoroutine> list{};
        (list.emplace_back(std::forward<Coroutines>(coroutines)), ...);
        return impl::MultipleCoroutinesAwaiter(std::move(list));
    }
}

#endif // TREMOTESF_COROUTINES_WAITALL_H
