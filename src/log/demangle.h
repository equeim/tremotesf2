// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LOG_DEMANGLE_H
#define TREMOTESF_LOG_DEMANGLE_H

#include <string>
#include <typeinfo>

namespace tremotesf {
    namespace impl {
        std::string demangleTypeName(const char* typeName);
    }

    template<typename T>
    std::string typeName(const T& t) {
        return impl::demangleTypeName(typeid(t).name());
    }
}

#endif // TREMOTESF_LOG_DEMANGLE_H
