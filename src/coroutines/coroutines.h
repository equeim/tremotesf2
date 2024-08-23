// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_H
#define TREMOTESF_COROUTINES_H

#include <concepts>
#include <coroutine>
#include <functional>
#include <optional>
#include <variant>
#include <utility>

#include <QtGlobal>

#include "coroutinefwd.h"

namespace tremotesf {
    namespace impl {
        template<CoroutineReturnValue T>
        class CoroutinePromise;

        template<CoroutineReturnValue T>
        class CoroutineAwaiter;
    }

    template<impl::CoroutineReturnValue T>
    class [[nodiscard]] Coroutine {
    public:
        using promise_type = impl::CoroutinePromise<T>;

        inline explicit Coroutine(std::coroutine_handle<impl::CoroutinePromise<T>> handle)
            : mHandle(std::move(handle)) {}

        inline Coroutine(Coroutine<T>&& other) noexcept : mHandle(std::exchange(other.mHandle, nullptr)) {}
        inline Coroutine& operator=(Coroutine<T>&& other) noexcept {
            mHandle = std::exchange(other.mHandle, nullptr);
            return *this;
        }

        Q_DISABLE_COPY(Coroutine)

        inline ~Coroutine() {
            if (mHandle) {
                mHandle.destroy();
            }
        }

        inline impl::CoroutineAwaiter<T> operator co_await();

        inline void* address() const { return mHandle.address(); }

    private:
        std::coroutine_handle<impl::CoroutinePromise<T>> mHandle;

        friend class impl::StandaloneCoroutine;
    };

    namespace impl {
        class CoroutinePromiseFinalSuspendAwaiter;

        class CoroutinePromiseBase {
        public:
            inline ~CoroutinePromiseBase() = default;
            Q_DISABLE_COPY_MOVE(CoroutinePromiseBase)

            // promise object contract begin
            inline std::suspend_always initial_suspend() { return {}; }
            inline CoroutinePromiseFinalSuspendAwaiter final_suspend() noexcept;
            inline void unhandled_exception() { mUnhandledException = std::current_exception(); }
            // promise object contract end

            void interruptChildAwaiter();

            struct JustCompleteCancellation {};

            bool onStartedAwaiting(JustCompleteCancellation);
            bool onStartedAwaiting(std::function<void()>&& interruptionCallback);
            inline void onAboutToResume() { mChildAwaiterInterruptionCallback = std::monostate{}; }

            inline StandaloneCoroutine* owningStandaloneCoroutine() const { return mOwningStandaloneCoroutine; }
            inline void setOwningStandaloneCoroutine(StandaloneCoroutine* root) { mOwningStandaloneCoroutine = root; }
            void setParentCoroutineHandle(std::coroutine_handle<> parentCoroutineHandle);

            std::coroutine_handle<> onPerformedFinalSuspendBase();
            void rethrowException() {
                if (mUnhandledException) {
                    std::rethrow_exception(mUnhandledException);
                }
            }

        protected:
            inline CoroutinePromiseBase(std::coroutine_handle<> handle) : mCoroutineHandle(handle) {}

            [[noreturn]]
            void abortNoParent();

            std::coroutine_handle<> mCoroutineHandle;
            std::coroutine_handle<> mParentCoroutineHandle{};
            std::variant<std::monostate, JustCompleteCancellation, std::function<void()>>
                mChildAwaiterInterruptionCallback{};
            std::exception_ptr mUnhandledException{};
            StandaloneCoroutine* mOwningStandaloneCoroutine{};
        };

        template<CoroutineReturnValue T>
        class CoroutinePromise final : public CoroutinePromiseBase {
        public:
            inline CoroutinePromise()
                : CoroutinePromiseBase(std::coroutine_handle<CoroutinePromise<T>>::from_promise(*this)) {}

            // promise object contract begin
            inline Coroutine<T> get_return_object() {
                return Coroutine<T>(std::coroutine_handle<CoroutinePromise<T>>::from_promise(*this));
            }
            inline void return_value(const T& valueToReturn) { mValue = valueToReturn; }
            inline void return_value(T&& valueToReturn) { mValue = std::move(valueToReturn); }
            // promise object contract end

            inline std::coroutine_handle<> onPerformedFinalSuspend() {
                if (const auto handle = onPerformedFinalSuspendBase(); handle) {
                    return handle;
                }
                abortNoParent();
            }

            inline T takeValueOrRethrowException() {
                rethrowException();
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                T value = std::move(mValue).value();
                mValue.reset();
                return value;
            }

        private:
            std::optional<T> mValue{};
        };

        template<>
        class CoroutinePromise<void> final : public CoroutinePromiseBase {
        public:
            inline CoroutinePromise()
                : CoroutinePromiseBase(std::coroutine_handle<CoroutinePromise<void>>::from_promise(*this)) {}

            // promise object contract begin
            inline Coroutine<void> get_return_object() {
                return Coroutine<void>(std::coroutine_handle<CoroutinePromise<void>>::from_promise(*this));
            }
            inline void return_void() {}
            // promise object contract end

            std::coroutine_handle<> onPerformedFinalSuspend();

            inline void takeValueOrRethrowException() { rethrowException(); }
        };

        class CoroutinePromiseFinalSuspendAwaiter final {
        public:
            inline bool await_ready() noexcept { return false; }

            template<std::derived_from<CoroutinePromiseBase> Promise>
            inline std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) noexcept {
                return handle.promise().onPerformedFinalSuspend();
            }
            inline void await_resume() noexcept {}
        };

        CoroutinePromiseFinalSuspendAwaiter CoroutinePromiseBase::final_suspend() noexcept { return {}; }

        template<CoroutineReturnValue T>
        class CoroutineAwaiter final {
        public:
            inline explicit CoroutineAwaiter(std::coroutine_handle<CoroutinePromise<T>> handle) : mHandle(handle) {}
            inline ~CoroutineAwaiter() = default;
            Q_DISABLE_COPY_MOVE(CoroutineAwaiter)

            inline bool await_ready() { return false; }

            template<typename Promise>
            inline std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> parentCoroutineHandle) {
                if constexpr (std::derived_from<Promise, CoroutinePromiseBase>) {
                    mParentCoroutinePromise = &parentCoroutineHandle.promise();
                    if (!mParentCoroutinePromise->onStartedAwaiting([this] {
                            mHandle.promise().interruptChildAwaiter();
                        })) {
                        return std::noop_coroutine();
                    }
                    mHandle.promise().setOwningStandaloneCoroutine(mParentCoroutinePromise->owningStandaloneCoroutine()
                    );
                }
                mHandle.promise().setParentCoroutineHandle(parentCoroutineHandle);
                return mHandle;
            }

            inline T await_resume() {
                if (mParentCoroutinePromise) {
                    mParentCoroutinePromise->onAboutToResume();
                }
                return mHandle.promise().takeValueOrRethrowException();
            }

        private:
            std::coroutine_handle<CoroutinePromise<T>> mHandle;
            CoroutinePromiseBase* mParentCoroutinePromise{};
        };

        template<typename Promise>
        [[nodiscard]]
        bool startAwaiting(std::coroutine_handle<Promise> handle) {
            if constexpr (std::derived_from<Promise, CoroutinePromiseBase>) {
                auto& promise = handle.promise();
                return promise.onStartedAwaiting(CoroutinePromiseBase::JustCompleteCancellation{});
            } else {
                return true;
            }
        }

        template<typename Promise>
        void resume(std::coroutine_handle<Promise> handle) {
            if constexpr (std::derived_from<Promise, CoroutinePromiseBase>) {
                handle.promise().onAboutToResume();
            }
            handle.resume();
        }

        class StandaloneCoroutine {
        public:
            inline explicit StandaloneCoroutine(Coroutine<void>&& coroutine) : mCoroutine(std::move(coroutine)) {
                mCoroutine.mHandle.promise().setOwningStandaloneCoroutine(this);
            }
            inline ~StandaloneCoroutine() = default;
            Q_DISABLE_COPY_MOVE(StandaloneCoroutine)

            inline void* address() const { return mCoroutine.address(); }

            inline void start() { mCoroutine.mHandle.resume(); }

            inline StandaloneCoroutine* rootCoroutine() const { return mRootCoroutine; }
            inline void setRootCoroutine(StandaloneCoroutine* coroutine) { mRootCoroutine = coroutine; }

            void cancel();
            bool completeCancellation();

            inline void invokeCompletionCallback(std::exception_ptr&& unhandledException) {
                mCompletionCallback(std::move(unhandledException));
            }
            inline void setCompletionCallback(std::function<void(std::exception_ptr)>&& callback) {
                mCompletionCallback = std::move(callback);
            }

        private:
            Coroutine<void> mCoroutine;
            StandaloneCoroutine* mRootCoroutine{};
            std::function<void(std::exception_ptr)> mCompletionCallback{};
            enum class CancellationState : char { NotCancelled, Cancelling, Cancelled };
            CancellationState mCancellationState{CancellationState::NotCancelled};
        };

        class [[nodiscard]] CancellationAwaiter final {
        public:
            inline bool await_ready() { return false; }

            template<std::derived_from<CoroutinePromiseBase> Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (startAwaiting(handle)) {
                    handle.promise().owningStandaloneCoroutine()->rootCoroutine()->cancel();
                }
            }

            inline void await_resume() {}
        };
    }

    template<impl::CoroutineReturnValue T>
    impl::CoroutineAwaiter<T> Coroutine<T>::operator co_await() {
        return impl::CoroutineAwaiter<T>(mHandle);
    }
}

#define cancelCoroutine()                                         \
    do {                                                          \
        co_await tremotesf::impl::CancellationAwaiter{};          \
        std::abort(); /* CancellationAwaiter must never resume */ \
    } while (false);

#endif // TREMOTESF_COROUTINES_H
