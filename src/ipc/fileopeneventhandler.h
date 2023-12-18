// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_FILEOPENEVENTHANDLER_H
#define TREMOTESF_FILEOPENEVENTHANDLER_H

#include <vector>
#include <QObject>

class QFileOpenEvent;

namespace tremotesf {
    class FileOpenEventHandler : public QObject {
        Q_OBJECT
    public:
        explicit FileOpenEventHandler(QObject* parent = nullptr);
        ~FileOpenEventHandler() override;
        Q_DISABLE_COPY_MOVE(FileOpenEventHandler)
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        void processPendingEvents();

        struct PendingEvent {
            QString fileOrUrl{};
            bool isFile{};
        };
        std::vector<PendingEvent> mPendingEvents{};

    signals:
        void filesOpeningRequested(const QStringList& files, const QStringList& urls);
    };
}

#endif // TREMOTESF_FILEOPENEVENTHANDLER_H
