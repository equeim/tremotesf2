// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_THREADPOOL_H
#define TREMOTESF_COROUTINES_THREADPOOL_H

#include <atomic>
#include <memory>

#include <QThreadPool>
#include <QRunnable>

#include "coroutines.h"

#ifdef Q_OS_WIN
#    include "startup/windowsfatalerrorhandlers.h"
#endif

namespace tremotesf {
    namespace impl {
        template<std::invocable Function, typename T = std::invoke_result_t<Function>>
        class ThreadPoolAwaitable final {
        public:
            inline explicit ThreadPoolAwaitable(QThreadPool* threadPool, Function&& function)
                : mThreadPool(threadPool), mFunction(std::move(function)) {}
            inline ~ThreadPoolAwaitable() {
                if (mSharedData) {
                    mSharedData->cancelled = true;
                }
            }
            Q_DISABLE_COPY_MOVE(ThreadPoolAwaitable)

            inline bool await_ready() { return false; }

            template<typename Promise>
            inline void await_suspend(std::coroutine_handle<Promise> handle) {
                if (!startAwaiting(handle)) {
                    return;
                }
                mSharedData = std::make_shared<SharedData>();
                mSharedData->coroutineHandle = handle;
                mThreadPool->start(new Runnable(std::move(mFunction), mSharedData));
            }

            T await_resume() {
                if (mSharedData->unhandledException) {
                    std::rethrow_exception(mSharedData->unhandledException);
                }
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                return std::move(mSharedData->result).value();
            }

        private:
            struct SharedData {
                QObject receiver{};
                std::atomic_bool cancelled{};
                std::optional<T> result{};
                std::exception_ptr unhandledException{};
                std::coroutine_handle<> coroutineHandle{};
            };

            class Runnable : public QRunnable {
            public:
                explicit Runnable(Function&& function, const std::shared_ptr<SharedData>& sharedData)
                    : mFunction(std::move(function)), mSharedData(sharedData) {}
                inline ~Runnable() override = default;
                Q_DISABLE_COPY_MOVE(Runnable)

                void run() override {
                    if (mSharedData->cancelled) {
                        return;
                    }
#ifdef Q_OS_WIN
                    windowsSetUpFatalErrorHandlersInThread();
#endif
                    try {
                        mSharedData->result = mFunction();
                    } catch (...) {
                        mSharedData->unhandledException = std::current_exception();
                    }
                    if (mSharedData->cancelled) {
                        return;
                    }
                    QMetaObject::invokeMethod(&(mSharedData->receiver), [sharedData = mSharedData] {
                        if (!sharedData->cancelled) {
                            sharedData->coroutineHandle.resume();
                        }
                    });
                }

            private:
                Function mFunction;
                std::shared_ptr<SharedData> mSharedData;
            };

            QThreadPool* mThreadPool;
            Function mFunction;
            std::shared_ptr<SharedData> mSharedData{};
        };
    }

    template<std::invocable Function>
    inline auto runOnThreadPool(QThreadPool* threadPool, Function&& function) {
        return impl::ThreadPoolAwaitable(threadPool, std::forward<Function>(function));
    }

    template<std::invocable Function>
    inline auto runOnThreadPool(Function&& function) {
        return runOnThreadPool(QThreadPool::globalInstance(), std::forward<Function>(function));
    }

    template<typename... Args, std::invocable<Args...> Function>
        requires(sizeof...(Args) != 0)
    inline auto runOnThreadPool(QThreadPool* threadPool, Function&& function, Args&&... args) {
        return impl::ThreadPoolAwaitable(
            threadPool,
            [function = std::forward<Function>(function), ... args = std::forward<Args>(args)]() mutable {
                return function(std::forward<Args>(args)...);
            }
        );
    }

    template<typename... Args, std::invocable<Args...> Function>
        requires(sizeof...(Args) != 0)
    inline auto runOnThreadPool(Function&& function, Args&&... args) {
        return runOnThreadPool(
            QThreadPool::globalInstance(),
            std::forward<Function>(function),
            std::forward<Args>(args)...
        );
    }
}

#endif // TREMOTESF_COROUTINES_THREADPOOL_H
