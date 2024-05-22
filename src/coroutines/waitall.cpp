// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "coroutines/waitall.h"
#include "log/log.h"

namespace tremotesf::impl {
    void MultipleCoroutinesAwaiter::await_resume() {
        if (mUnhandledException) {
            std::rethrow_exception(mUnhandledException);
        }
    }

    void MultipleCoroutinesAwaiter::awaitSuspendImpl() {
        if (mParentCoroutinePromise && !mParentCoroutinePromise->onStartedAwaiting([this] { cancelAll(); })) {
            return;
        }
        for (auto& coroutine : mCoroutines) {
            coroutine.setCompletionCallback([this, coroutinePointer = &coroutine](std::exception_ptr unhandledException
                                            ) { onCoroutineCompleted(coroutinePointer, std::move(unhandledException)); }
            );
            coroutine.start();
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
        for (auto* coroutine : getPointers(mCoroutines)) {
            coroutine->cancel();
        }
        mCancelling = false;
        if (mCoroutines.empty()) {
            onAllCoroutinesCompleted();
        }
    }

}
