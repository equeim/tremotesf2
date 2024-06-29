// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "coroutines/waitall.h"
#include "log/log.h"

namespace tremotesf::impl {
    namespace {
        template<typename T>
        inline std::vector<T*> getPointers(std::list<T>& list) {
            std::vector<T*> pointers{};
            pointers.reserve(list.size());
            for (auto& item : list) {
                pointers.push_back(&item);
            }
            return pointers;
        }
    }

    void MultipleCoroutinesAwaiter::await_resume() {
        if (mUnhandledException) {
            std::rethrow_exception(mUnhandledException);
        }
    }

    void MultipleCoroutinesAwaiter::awaitSuspendImpl() {
        if (mParentCoroutinePromise && !mParentCoroutinePromise->onStartedAwaiting([this] { cancelAll(); })) {
            return;
        }
        // Copy pointers to handle the case when coroutine completes immediately and is erased from list while we are iterating
        for (auto* coroutine : getPointers(mCoroutines)) {
            coroutine->setCompletionCallback([this, coroutine](std::exception_ptr unhandledException) {
                onCoroutineCompleted(coroutine, std::move(unhandledException));
            });
            coroutine->start();
        }
    }

    void
    MultipleCoroutinesAwaiter::onCoroutineCompleted(RootCoroutine* coroutine, std::exception_ptr unhandledException) {
        if (!mUnhandledException) {
            mUnhandledException = std::move(unhandledException);
        }
        const auto found = std::ranges::find_if(mCoroutines, [coroutine](auto& c) { return &c == coroutine; });
        if (found == mCoroutines.end()) {
            warning().log("Did not find completed coroutine {} in MultipleCoroutinesAwaiter", *coroutine);
            std::abort();
        }
        mCoroutines.erase(found);
        if (mCancelling) {
            return;
        }
        if (mCoroutines.empty()) {
            onAllCoroutinesCompleted();
        } else if (mUnhandledException) {
            cancelAll();
        }
    }

    void MultipleCoroutinesAwaiter::onAllCoroutinesCompleted() { resume(mParentCoroutineHandle); }

    void MultipleCoroutinesAwaiter::cancelAll() {
        mCancelling = true;
        // Copy pointers to handle the case when coroutine is cancelled immediately and is erased from list while we are iterating
        for (auto* coroutine : getPointers(mCoroutines)) {
            coroutine->cancel();
        }
        mCancelling = false;
        if (mCoroutines.empty()) {
            onAllCoroutinesCompleted();
        }
    }

}
