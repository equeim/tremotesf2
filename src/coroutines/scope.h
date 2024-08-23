// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINES_SCOPE_H
#define TREMOTESF_COROUTINES_SCOPE_H

#if __has_include(<QtClassHelperMacros>)
#    include <QtClassHelperMacros>
#else
#    include <QtGlobal>
#endif

#include <exception>
#include <vector>

#include "coroutinefwd.h"

namespace tremotesf {
    class CoroutineScope {
    public:
        CoroutineScope() = default;
        ~CoroutineScope();
        Q_DISABLE_COPY_MOVE(CoroutineScope)

        void launch(Coroutine<> coroutine);
        void cancelAll();

        inline size_t coroutinesCount() const { return mCoroutines.size(); }

    private:
        void onCoroutineCompleted(impl::StandaloneCoroutine* coroutine, const std::exception_ptr& unhandledException);

        std::vector<impl::StandaloneCoroutine*> mCoroutines{};
    };
}

#endif // TREMOTESF_COROUTINES_SCOPE_H
