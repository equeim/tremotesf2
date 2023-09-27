// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SAVEWINDOWSTATEDISPATCHER_H
#define TREMOTESF_SAVEWINDOWSTATEDISPATCHER_H

#include <functional>
#include <QObject>

class QWidget;

namespace tremotesf {
    class SaveWindowStateDispatcher : public QObject {
        Q_OBJECT
    public:
        SaveWindowStateDispatcher();

        static void registerHandler(QWidget* window, std::function<void()> saveState);
    };

}

#endif //TREMOTESF_SAVEWINDOWSTATEDISPATCHER_H
