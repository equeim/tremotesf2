// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "savewindowstatedispatcher.h"

#include <functional>
#include <QApplication>
#include <QWidget>

#include "settings.h"
#include "log/log.h"
#include "rpc/servers.h"

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QEvent::Type)

namespace tremotesf {
    namespace {
        constexpr auto windowHasSaveStateHandlerProperty = "tremotesf_windowHasSaveStateHandler";

        QEvent::Type saveStateOnQuitEventType() {
            static const auto type = static_cast<QEvent::Type>(QEvent::registerEventType());
            return type;
        }
    }

    SaveWindowStateDispatcher::SaveWindowStateDispatcher() {
        QObject::connect(
            qApp,
            &QCoreApplication::aboutToQuit,
            this,
            &SaveWindowStateDispatcher::onAboutToQuit,
            Qt::DirectConnection
        );
    }

    void SaveWindowStateDispatcher::onAboutToQuit() {
        debug().log("Received aboutToQuit signal");
        for (QWidget* window : QApplication::topLevelWidgets()) {
            if (window->property(windowHasSaveStateHandlerProperty).toBool()) {
                debug().log("Sending save state event to window {}", *window);
                QEvent event(saveStateOnQuitEventType());
                QCoreApplication::sendEvent(window, &event);
            }
        }
        // On Windows our process might be terminated immediately after returning from this function
        // and QSettings destructors won't be called, so sync them immediately
        debug().log("Syncing settings");
        Settings::instance()->sync();
        Servers::instance()->sync();
    }

#if QT_VERSION_MAJOR >= 6
    bool ApplicationQuitEventFilter::eventFilter(QObject*, QEvent* event) {
        if (event->type() == QEvent::Quit) {
            isQuittingApplication = true;
            QMetaObject::invokeMethod(
                qApp,
                [this] { isQuittingApplication = false; },
                Qt::QueuedConnection
            );
        }
        return false;
    }
#endif

    SaveWindowStateHandler::SaveWindowStateHandler(QWidget* window, std::function<void()> saveState, QObject* parent)
        : QObject(parent), mWindow(window), mSaveState(std::move(saveState)) {
        mWindow->installEventFilter(this);
        mWindow->setProperty(windowHasSaveStateHandlerProperty, true);
    }

    SaveWindowStateHandler::~SaveWindowStateHandler() {
        mWindow->removeEventFilter(this);
        mWindow->setProperty(windowHasSaveStateHandlerProperty, QVariant::Invalid);
    }

    bool SaveWindowStateHandler::eventFilter(QObject* watched, QEvent* event) {
        const auto window = static_cast<QWidget*>(watched);
        // Window can be moved without activation, so we also need to handle Hide event
        switch (event->type()) {
        case QEvent::WindowDeactivate:
        case QEvent::Hide:
            debug().log("Received {} event for {}", event->type(), *window);
#if QT_VERSION_MAJOR >= 6
            if (mApplicationEventFilter.isQuittingApplication) {
                debug().log("Already quitting application, ignore");
                break;
            }
#endif
            if (event->type() == QEvent::WindowDeactivate && window->isHidden()) {
                debug().log("Window is hidden, ignore");
                break;
            }
            mSaveState();
            break;
        default:
            if (event->type() == saveStateOnQuitEventType()) {
                debug().log("Received save state event for {}", *window);
                mSaveState();
            }
            break;
        }
        return false;
    }
}
