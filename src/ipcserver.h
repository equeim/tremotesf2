/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_IPCSERVER_H
#define TREMOTESF_IPCSERVER_H

#include <QObject>

namespace tremotesf
{
    class IpcServer : public QObject
    {
        Q_OBJECT
    public:
        static IpcServer* createInstance(QObject* parent = nullptr);

        inline explicit IpcServer(QObject* parent = nullptr) : QObject(parent) {};

    signals:
        void windowActivationRequested(const QString& torrentHash, const QByteArray& startupNoficationId);
        void torrentsAddingRequested(const QStringList& files, const QStringList& urls, const QByteArray& startupNoficationId);
    };
}

#endif // TREMOTESF_IPCSERVER_H
