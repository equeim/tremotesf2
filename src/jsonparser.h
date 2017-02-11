/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_JSONPARSER_H
#define TREMOTESF_JSONPARSER_H

#include <QObject>

class QThread;

namespace tremotesf
{
    class JsonParser : public QObject
    {
        Q_OBJECT
    public:
        explicit JsonParser(const QByteArray& data, QObject* parent);
        ~JsonParser();
    private:
        QThread* mWorkerThread;
    signals:
        void requestParse(const QByteArray& data);
        void done(const QVariantMap& parseResult, bool success);
    };
}

#endif // TREMOTESF_JSONPARSER_H
