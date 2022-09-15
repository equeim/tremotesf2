#ifndef TREMOTESF_UNIXHELPERS_H
#define TREMOTESF_UNIXHELPERS_H

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace tremotesf {
    namespace impl {
        int64_t checkPosixError(int64_t result, std::string_view functionName);
    }

    /**
     * @brief checkPosixError
     * @param result Return value of POSIX function that returns -1 on failure and sets errno
     * @param functionName Name of function that returned result
     * @returns result
     * @throws std::system_error
     */
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
    T checkPosixError(T result, std::string_view functionName) {
        return static_cast<T>(impl::checkPosixError(static_cast<int64_t>(result), functionName));
    }
}

#endif // TREMOTESF_UNIXHELPERS_H
