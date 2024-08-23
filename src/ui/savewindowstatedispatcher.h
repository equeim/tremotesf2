// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
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

#if QT_VERSION_MAJOR >= 6
    class ApplicationQuitEventFilter : public QObject {
        Q_OBJECT
    public:
        explicit ApplicationQuitEventFilter(QObject* parent = nullptr);
        ~ApplicationQuitEventFilter() override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        bool isQuittingApplication{};
    };
#endif

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
#if QT_VERSION_MAJOR >= 6
        ApplicationQuitEventFilter mApplicationEventFilter{};
#endif
    };
}

#endif //TREMOTESF_SAVEWINDOWSTATEDISPATCHER_H
