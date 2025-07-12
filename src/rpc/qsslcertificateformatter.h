// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_QSSLCERTIFICATEFORMATTER_H
#define TREMOTESF_QSSLCERTIFICATEFORMATTER_H

#include "log/formatters.h"

class QSslCertificate;

namespace fmt {
    template<>
    struct formatter<QSslCertificate> : tremotesf::SimpleFormatter {
        fmt::format_context::iterator format(const QSslCertificate& certificate, fmt::format_context& ctx) const;
    };
}

#endif // TREMOTESF_QSSLCERTIFICATEFORMATTER_H
