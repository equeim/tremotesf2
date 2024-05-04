// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FMT_FORMAT_INCLUDE_WRAPPER_H
#define TREMOTESF_FMT_FORMAT_INCLUDE_WRAPPER_H

#ifdef TREMOTESF_FMT_NEEDS_WARNING_WORKAROUND
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Warray-bounds"
#    pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <fmt/format.h>

#ifdef TREMOTESF_FMT_NEEDS_WARNING_WORKAROUND
#    pragma GCC diagnostic pop
#endif

#endif // TREMOTESF_FMT_FORMAT_INCLUDE_WRAPPER_H
