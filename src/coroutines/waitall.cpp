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
            if (mParentCoroutinePromise) {
                coroutine->setRootCoroutine(mParentCoroutinePromise->owningStandaloneCoroutine()->rootCoroutine());
            }
            coroutine->setCompletionCallback([this, coroutine](std::exception_ptr unhandledException) {
                onCoroutineCompleted(coroutine, std::move(unhandledException));
            });
            coroutine->start();
        }
    }

    void MultipleCoroutinesAwaiter::onCoroutineCompleted(
        StandaloneCoroutine* coroutine, std::exception_ptr unhandledException
    ) {
        if (unhandledException && !mUnhandledException) {
            mUnhandledException = std::move(unhandledException);
        }
        const auto found = std::ranges::find_if(mCoroutines, [coroutine](auto& c) { return &c == coroutine; });
        if (found == mCoroutines.end()) {
            fatal().log("Did not find completed coroutine {} in MultipleCoroutinesAwaiter", coroutine->address());
            Q_UNREACHABLE();
        }
        mCoroutines.erase(found);
        if (mCancellingCoroutines) {
            return;
        }
        if (mCoroutines.empty()) {
            onAllCoroutinesCompleted();
        } else if ((mUnhandledException || mCancelAfterFirst) &&
                   !mCancelledCoroutinesAndWaitingForCompletionCallbacks) {
            cancelAll();
        }
    }

    void MultipleCoroutinesAwaiter::onAllCoroutinesCompleted() {
        if (mParentCoroutinePromise && mParentCoroutinePromise->owningStandaloneCoroutine()->completeCancellation()) {
            return;
        }
        resume(mParentCoroutineHandle);
    }

    void MultipleCoroutinesAwaiter::cancelAll() {
        mCancellingCoroutines = true;
        // Copy pointers to handle the case when coroutine is cancelled immediately and is erased from list while we are iterating
        for (auto* coroutine : getPointers(mCoroutines)) {
            coroutine->cancel();
        }
        mCancellingCoroutines = false;
        mCancelledCoroutinesAndWaitingForCompletionCallbacks = true;
        if (mCoroutines.empty()) {
            onAllCoroutinesCompleted();
        }
    }

}
