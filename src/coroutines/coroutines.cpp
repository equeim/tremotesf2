// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "coroutines.h"

#include "log/log.h"

namespace tremotesf::impl {
    void CoroutinePromiseBase::setParentCoroutineHandle(std::coroutine_handle<> parentCoroutineHandle) {
        mParentCoroutineHandle = parentCoroutineHandle;
    }

    void CoroutinePromiseBase::interruptChildAwaiter() {
        std::visit(
            [&](auto& callback) {
                using Type = std::decay_t<decltype(callback)>;
                if constexpr (std::same_as<Type, JustCompleteCancellation>) {
                    mOwningStandaloneCoroutine->completeCancellation();
                } else if constexpr (std::same_as<Type, std::function<void()>>) {
                    callback();
                } else if constexpr (std::same_as<Type, std::monostate>) {
                }
            },
            mChildAwaiterInterruptionCallback
        );
    }

    bool CoroutinePromiseBase::onStartedAwaiting(JustCompleteCancellation) {
        if (mOwningStandaloneCoroutine->completeCancellation()) {
            return false;
        }
        mChildAwaiterInterruptionCallback = JustCompleteCancellation{};
        return true;
    }

    bool CoroutinePromiseBase::onStartedAwaiting(std::function<void()>&& interruptionCallback) {
        if (mOwningStandaloneCoroutine->completeCancellation()) {
            return false;
        }
        mChildAwaiterInterruptionCallback = std::move(interruptionCallback);
        return true;
    }

    std::coroutine_handle<> CoroutinePromiseBase::onPerformedFinalSuspendBase() {
        if (mOwningStandaloneCoroutine->completeCancellation()) {
            return std::noop_coroutine();
        }
        return mParentCoroutineHandle;
    }

    void CoroutinePromiseBase::abortNoParent() {
        warning().log("No parent coroutine when completing coroutine {}", mCoroutineHandle.address());
        std::abort();
    }

    std::coroutine_handle<> CoroutinePromise<void>::onPerformedFinalSuspend() {
        if (const auto handle = onPerformedFinalSuspendBase(); handle) {
            return handle;
        }
        mOwningStandaloneCoroutine->invokeCompletionCallback(std::move(mUnhandledException));
        return std::noop_coroutine();
    }

    void StandaloneCoroutine::cancel() {
        if (mCancellationState != CancellationState::NotCancelled) {
            return;
        }
        mCancellationState = CancellationState::Cancelling;
        mCoroutine.mHandle.promise().interruptChildAwaiter();
    }

    bool StandaloneCoroutine::completeCancellation() {
        switch (mCancellationState) {
        case CancellationState::NotCancelled:
            return false;
        case CancellationState::Cancelling:
            mCancellationState = CancellationState::Cancelled;
            invokeCompletionCallback({});
            return true;
        case CancellationState::Cancelled:
            return true;
        }
        return false;
    }
}
