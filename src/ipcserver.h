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

#include <QLocalServer>

namespace tremotesf
{
    struct ArgumentsParseResult
    {
        QStringList files;
        QStringList urls;
    };

    class IpcServer : public QLocalServer
    {
        Q_OBJECT
    public:
        explicit IpcServer(QObject* parent = nullptr);

        static bool tryToConnect();
        static void ping();
        static void sendArguments(const QStringList& arguments);
        static ArgumentsParseResult parseArguments(const QStringList& arguments);
    signals:
        void pinged();
        void filesReceived(const QStringList& files);
        void urlsReceived(const QStringList& urls);
    };
}
