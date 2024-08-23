// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COROUTINEFWD_H
#define TREMOTESF_COROUTINEFWD_H

#include <concepts>

namespace tremotesf {
    namespace impl {
        template<typename T>
        concept CoroutineReturnValue = std::same_as<T, void> || std::movable<T>;

        class StandaloneCoroutine;
    }

    template<impl::CoroutineReturnValue T = void>
    class Coroutine;
}

#endif // TREMOTESF_COROUTINEFWD_H
