// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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

    void CoroutinePromiseBase::abortNoParent(std::coroutine_handle<> handle) {
        fatal().log("No parent coroutine when completing coroutine {}", handle.address());
        Q_UNREACHABLE();
    }

    void CoroutinePromise<void>::invokeCompletionCallbackForStandaloneCoroutine() {
        // Completion callback will destroy Coroutine<> object, but coroutine itself will be destroyed later by compiler's injected machinery
        // because CoroutinePromiseFinalSuspendAwaiter::await_ready will return false
        // Pass true for coroutineWillBeDestroyedAutomatically parameter here so that Coroutine<>'s destructor won't destroy coroutine resulting in double free
        mOwningStandaloneCoroutine->invokeCompletionCallback(std::move(mUnhandledException), true);
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
            invokeCompletionCallback({}, false);
            return true;
        case CancellationState::Cancelled:
            return true;
        }
        return false;
    }

    void StandaloneCoroutine::invokeCompletionCallback(
        std::exception_ptr&& unhandledException, bool coroutineWillBeDestroyedAutomatically
    ) {
        if (coroutineWillBeDestroyedAutomatically) {
            mCoroutine.mHandle = nullptr;
        }
        mCompletionCallback(std::move(unhandledException));
    }

    void StandaloneCoroutine::setCompletionCallback(std::function<void(std::exception_ptr)>&& callback) {
        mCompletionCallback = std::move(callback);
    }
}
