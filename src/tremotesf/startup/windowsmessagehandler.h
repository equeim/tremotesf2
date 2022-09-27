// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_WINDOWSMESSAGEHANDLER_H
#define TREMOTESF_WINDOWSMESSAGEHANDLER_H

#include <QtMessageHandler>

namespace tremotesf {
    void windowsMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
}

#endif
