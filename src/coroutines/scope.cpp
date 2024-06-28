// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "coroutines/scope.h"
#include "log/log.h"

namespace tremotesf {
    namespace {
        void handleException(const std::exception_ptr& unhandledException) {
            if (!unhandledException) return;
            warning().log("Unhandled exception in coroutine");
            // Make sure we terminate immediately
            try {
                std::rethrow_exception(unhandledException);
            } catch (...) {
                std::terminate();
            }
        }
    }

    CoroutineScope::~CoroutineScope() {
        if (mCoroutines.empty()) return;
        for (auto* coroutine : mCoroutines) {
            // NOLINTNEXTLINE(bugprone-unhandled-exception-at-new)
            coroutine->setCompletionCallback([coroutine](const std::exception_ptr& unhandledException) {
                handleException(unhandledException);
                delete coroutine;
            });
            coroutine->cancel();
        }
    }

    void CoroutineScope::launch(Coroutine<> coroutine) {
        auto* root = new impl::RootCoroutine(std::move(coroutine));
        mCoroutines.push_back(root);
        root->setCompletionCallback([this, root](const std::exception_ptr& unhandledException) {
            onCoroutineCompleted(root, std::move(unhandledException));
        });
        root->start();
    }

    void CoroutineScope::cancelAll() {
        for (auto* coroutine : mCoroutines) {
            coroutine->cancel();
        }
    }

    void
    CoroutineScope::onCoroutineCompleted(impl::RootCoroutine* coroutine, const std::exception_ptr& unhandledException) {
        handleException(unhandledException);
        const auto found = std::ranges::find(mCoroutines, coroutine);
        if (found == mCoroutines.end()) {
            warning().log("Did not find completed coroutine {} in CoroutineScope", *coroutine);
            std::abort();
        }
        mCoroutines.erase(found);
        delete coroutine;
    }
}
