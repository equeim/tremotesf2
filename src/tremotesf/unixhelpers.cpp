#include "unixhelpers.h"

#include <cerrno>
#include <stdexcept>

#include <fmt/format.h>

#include "libtremotesf/log.h"

namespace tremotesf::impl {
    int64_t checkPosixError(int64_t result, std::string_view functionName) {
        if (result != -1) return result;
        const auto baseError = std::system_error(errno, std::system_category());
        throw std::system_error(
            baseError.code(),
            fmt::format("{} failed with", functionName)
        );
    }
}
