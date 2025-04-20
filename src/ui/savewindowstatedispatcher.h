// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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

    private:
        void onAboutToQuit();
    };

    class ApplicationQuitEventFilter : public QObject {
        Q_OBJECT
    public:
        explicit ApplicationQuitEventFilter(QObject* parent = nullptr);
        ~ApplicationQuitEventFilter() override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        bool isQuittingApplication{};
    };

    class SaveWindowStateHandler : public QObject {
        Q_OBJECT
    public:
        explicit SaveWindowStateHandler(QWidget* window, std::function<void()> saveState, QObject* parent = nullptr);
        ~SaveWindowStateHandler() override;
        Q_DISABLE_COPY_MOVE(SaveWindowStateHandler)
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        QWidget* mWindow{};
        std::function<void()> mSaveState{};
        ApplicationQuitEventFilter mApplicationEventFilter{};
    };
}

#endif //TREMOTESF_SAVEWINDOWSTATEDISPATCHER_H
