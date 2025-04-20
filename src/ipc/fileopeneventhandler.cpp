// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileopeneventhandler.h"

#include <QCoreApplication>
#include <QFileOpenEvent>

#include "log/log.h"

namespace tremotesf {
    FileOpenEventHandler::FileOpenEventHandler(QObject* parent) : QObject(parent) {
        QCoreApplication::instance()->installEventFilter(this);
    }

    FileOpenEventHandler::~FileOpenEventHandler() { QCoreApplication::instance()->removeEventFilter(this); }

    bool FileOpenEventHandler::eventFilter(QObject* watched, QEvent* event) {
        // In case of multiple files / urls, Qt send separate QFileOpenEvent per file in a loop
        // Call processPendingEvents() in the next event loop iteration to process all events at once
        if (event->type() == QEvent::FileOpen) {
            auto* const e = static_cast<QFileOpenEvent*>(event);
            if (!e->file().isEmpty() || e->url().isValid()) {
                info().log("Received QEvent::FileOpen");
                auto pendingEvent = [&] {
                    if (!e->file().isEmpty()) {
                        info().log("file = {}", e->file());
                        return PendingEvent{.fileOrUrl = e->file(), .isFile = true};
                    }
                    auto url = e->url().toString();
                    info().log("url = {}", url);
                    return PendingEvent{.fileOrUrl = std::move(url), .isFile = false};
                }();
                if (mPendingEvents.empty()) {
                    QMetaObject::invokeMethod(this, &FileOpenEventHandler::processPendingEvents, Qt::QueuedConnection);
                }
                mPendingEvents.push_back(std::move(pendingEvent));
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void FileOpenEventHandler::processPendingEvents() {
        QStringList files{};
        QStringList urls{};
        for (auto& event : mPendingEvents) {
            if (event.isFile) {
                files.push_back(std::move(event.fileOrUrl));
            } else {
                urls.push_back(std::move(event.fileOrUrl));
            }
        }
        mPendingEvents.clear();
        emit filesOpeningRequested(files, urls);
    }
}
