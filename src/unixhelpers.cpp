// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "unixhelpers.h"

#include <cerrno>
#include <system_error>

#include <fmt/format.h>

namespace tremotesf::impl {
    void throwWithErrno(std::string_view functionName) {
        const auto baseError = std::system_error(errno, std::system_category());
        throw std::system_error(baseError.code(), fmt::format("{} failed with", functionName));
    }
}
