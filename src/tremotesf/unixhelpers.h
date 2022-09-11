#ifndef TREMOTESF_UNIXHELPERS_H
#define TREMOTESF_UNIXHELPERS_H

#include <string_view>

namespace tremotesf {
    /**
     * @brief checkPosixError
     * @param result Return value of POSIX function that returns 0 on success and sets errno otherwise
     * @param functionName Name of function that returned result
     * @throws std::system_error
     */
    void checkPosixError(int result, std::string_view functionName);
}

#endif // TREMOTESF_UNIXHELPERS_H
