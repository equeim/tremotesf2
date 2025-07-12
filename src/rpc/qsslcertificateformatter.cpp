// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QSslCertificate>
#include <QSslSocket>

#include "qsslcertificateformatter.h"

using namespace Qt::StringLiterals;

namespace fmt {
    format_context::iterator
    formatter<QSslCertificate>::format(const QSslCertificate& certificate, format_context& ctx) const {
        // QSslCertificate::toText is implemented only for OpenSSL backend
        static const bool isOpenSSL = (QSslSocket::activeBackend() == "openssl"_L1);
        if (!isOpenSSL) {
            return tremotesf::impl::QDebugFormatter<QSslCertificate>{}.format(certificate, ctx);
        }
        return formatter<QString>{}.format(certificate.toText(), ctx);
    }
}
