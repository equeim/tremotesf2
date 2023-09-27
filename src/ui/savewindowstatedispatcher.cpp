// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "savewindowstatedispatcher.h"

#include <QCoreApplication>
#include <QWidget>

#include "settings.h"
#include "log/log.h"
#include "rpc/servers.h"

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QEvent::Type)

namespace tremotesf {
    namespace {
        QEvent::Type saveStateOnQuitEventType() {
            static const auto type = static_cast<QEvent::Type>(QEvent::registerEventType());
            return type;
        }

        class SaveStateEventFilter : public QObject {
            Q_OBJECT
        public:
            explicit SaveStateEventFilter(QWidget* window, std::function<void()> saveState)
                : QObject(window), mWindow(window), saveState(std::move(saveState)) {}
            bool eventFilter(QObject* watched, QEvent* event) override {
                switch (event->type()) {
                case QEvent::WindowDeactivate:
                case QEvent::Close:
                    if (watched == mWindow) {
                        logDebug("Received {} event", event->type());
                    } else {
                        return false;
                    }
                    break;
                default:
                    if (event->type() == saveStateOnQuitEventType()) {
                        logDebug("Received save state on quit event, watched is {}", *watched);
                        qApp->removeEventFilter(this);
                    } else {
                        return false;
                    }
                    break;
                }
                logDebug("Saving state for {}", *mWindow);
                saveState();
                return false;
            }

        private:
            QWidget* mWindow;
            std::function<void()> saveState;
        };
    }

    SaveWindowStateDispatcher::SaveWindowStateDispatcher() {
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, [] {
            logDebug("Received QCoreApplication::aboutToQuit signal, sending save state event");
            QEvent event(saveStateOnQuitEventType());
            QCoreApplication::sendEvent(qApp, &event);
            // On Windows our process might be killed immediately after this signal
            // and QSettings destructors won't be called, so sync them immediately
            logDebug("Sent save state event, syncing settings");
            Settings::instance()->sync();
            Servers::instance()->sync();
        });
    }

    void SaveWindowStateDispatcher::registerHandler(QWidget* window, std::function<void()> saveState) {
        const auto filter = new SaveStateEventFilter(window, std::move(saveState));
        qApp->installEventFilter(filter);
        QObject::connect(window, &QObject::destroyed, qApp, [filter] { qApp->removeEventFilter(filter); });
    }
}

#include "savewindowstatedispatcher.moc"
