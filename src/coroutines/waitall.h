// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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
            inline explicit MultipleCoroutinesAwaiter(std::vector<Coroutine<>>&& coroutines, bool cancelAfterFirst)
                : mCancelAfterFirst(cancelAfterFirst) {
                for (auto&& coroutine : std::move(coroutines)) {
                    mCoroutines.emplace_back(std::move(coroutine));
                }
            }

            template<typename... Coroutines>
            inline explicit MultipleCoroutinesAwaiter(bool cancelAfterFirst, Coroutines&&... coroutines)
                : mCancelAfterFirst(cancelAfterFirst) {
                (mCoroutines.emplace_back(std::forward<Coroutines>(coroutines)), ...);
            }

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
            void
            onCoroutineCompleted(impl::StandaloneCoroutine* coroutine, const std::exception_ptr& unhandledException);
            void onAllCoroutinesCompleted();
            void cancelAll();

            std::list<impl::StandaloneCoroutine> mCoroutines{};
            bool mCancelAfterFirst;
            std::coroutine_handle<> mParentCoroutineHandle{};
            CoroutinePromiseBase* mParentCoroutinePromise{};
            // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
            std::exception_ptr mUnhandledException{};
            bool mCancellingCoroutines{};
            bool mCancelledCoroutinesAndWaitingForCompletionCallbacks{};
        };
    }

    inline impl::MultipleCoroutinesAwaiter waitAll(std::vector<Coroutine<>>&& coroutines) {
        return impl::MultipleCoroutinesAwaiter(std::move(coroutines), false);
    }

    template<std::same_as<Coroutine<>>... Coroutines>
        requires(sizeof...(Coroutines) != 0)
    inline impl::MultipleCoroutinesAwaiter waitAll(Coroutines&&... coroutines) {
        return impl::MultipleCoroutinesAwaiter(false, std::forward<Coroutines>(coroutines)...);
    }

    inline impl::MultipleCoroutinesAwaiter waitAny(std::vector<Coroutine<>>&& coroutines) {
        return impl::MultipleCoroutinesAwaiter(std::move(coroutines), true);
    }

    template<std::same_as<Coroutine<>>... Coroutines>
        requires(sizeof...(Coroutines) != 0)
    inline impl::MultipleCoroutinesAwaiter waitAny(Coroutines&&... coroutines) {
        return impl::MultipleCoroutinesAwaiter(true, std::forward<Coroutines>(coroutines)...);
    }
}

#endif // TREMOTESF_COROUTINES_WAITALL_H
