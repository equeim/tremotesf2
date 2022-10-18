// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_DROPPEDTORRENTS_H
#define TREMOTESF_DROPPEDTORRENTS_H

#include <QStringList>

class QMimeData;
class QUrl;

namespace tremotesf {
    struct DroppedTorrents {
        explicit DroppedTorrents(const QMimeData* mime);

        [[nodiscard]] bool isEmpty() const { return files.isEmpty() && urls.isEmpty(); }

        QStringList files{};
        QStringList urls{};

    private:
        void processUrl(const QUrl& url);
    };
}

#endif // TREMOTESF_DROPPEDTORRENTS_H
