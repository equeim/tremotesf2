// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_UNIXHELPERS_H
#define TREMOTESF_UNIXHELPERS_H

#include <concepts>
#include <string_view>

namespace tremotesf {
    namespace impl {
        void throwWithErrno(std::string_view functionName);
    }

    /**
     * @brief checkPosixError
     * @param result Return value of POSIX function that returns -1 on failure and sets errno
     * @param functionName Name of function that returned result
     * @returns result
     * @throws std::system_error
     */
    template<std::signed_integral T>
    T checkPosixError(T result, std::string_view functionName) {
        if (result == T{-1}) {
            impl::throwWithErrno(functionName);
        }
        return result;
    }

    inline constexpr auto xdgActivationTokenEnvVariable = "XDG_ACTIVATION_TOKEN";
}

#endif // TREMOTESF_UNIXHELPERS_H
