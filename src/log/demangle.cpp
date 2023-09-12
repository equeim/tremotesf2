// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "demangle.h"

#include <cstdlib>
#include <memory>
#include <string_view>

#if __has_include(<cxxabi.h>)
#    include <cxxabi.h>
#    define TREMOTESF_HAVE_CXXABI_H
#endif

namespace tremotesf::impl {
#ifdef TREMOTESF_HAVE_CXXABI_H
    std::string demangleTypeName(const char* typeName) {
        const std::unique_ptr<char, decltype(&free)> demangled(
            abi::__cxa_demangle(typeName, nullptr, nullptr, nullptr),
            &free
        );
        return demangled ? demangled.get() : typeName;
    }
#else
    namespace {
        using namespace std::string_view_literals;
        constexpr auto structPrefix = "struct "sv;
        constexpr auto classPrefix = "class "sv;

        void removeSubstring(std::string& str, std::string_view substring) {
            size_t pos{};
            while ((pos = str.find(substring, pos)) != std::string::npos) {
                str.erase(pos, substring.size());
            }
        }
    }

    std::string demangleTypeName(const char* typeName) {
        std::string demangled = typeName;
        removeSubstring(demangled, structPrefix);
        removeSubstring(demangled, classPrefix);
        return demangled;
    }
#endif
}
