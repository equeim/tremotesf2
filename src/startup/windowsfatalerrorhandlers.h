// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_WINDOWSFATALERRORHANDLERS_H
#define TREMOTESF_WINDOWSFATALERRORHANDLERS_H

#include <string>

class QMessageLogContext;
class QString;

namespace tremotesf {
    void windowsSetUpFatalErrorHandlers();
    void windowsSetUpFatalErrorHandlersInThread();

    std::string makeFatalErrorReportFromLogMessage(const QString& message, const QMessageLogContext& context);
    void showFatalErrorReportInDialog(std::string report);
}

#endif // TREMOTESF_WINDOWSFATALERRORHANDLERS_H
