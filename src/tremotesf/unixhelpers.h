#ifndef TREMOTESF_UNIXHELPERS_H
#define TREMOTESF_UNIXHELPERS_H

#include <string_view>
#include <type_traits>

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
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
    T checkPosixError(T result, std::string_view functionName) {
        if (result == T{-1}) {
            impl::throwWithErrno(functionName);
        }
        return result;
    }
}

#endif // TREMOTESF_UNIXHELPERS_H
