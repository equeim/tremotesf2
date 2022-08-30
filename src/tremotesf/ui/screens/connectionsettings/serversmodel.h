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

#ifndef TREMOTESF_SERVERSMODEL_H
#define TREMOTESF_SERVERSMODEL_H

#include <vector>
#include <QAbstractListModel>

#include "tremotesf/rpc/servers.h"

namespace tremotesf
{
    class ServersModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        explicit ServersModel(QObject* parent = nullptr);

        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        int rowCount(const QModelIndex&) const override;
        bool setData(const QModelIndex& modelIndex, const QVariant& value, int role) override;

        const std::vector<Server>& servers() const;
        const QString& currentServerName() const;

        Q_INVOKABLE bool hasServer(const QString& name) const;

        Q_INVOKABLE void setServer(const QString& oldName,
                                   const QString& name,
                                   const QString& address,
                                   int port,
                                   const QString& apiPath,

                                   libtremotesf::Server::ProxyType proxyType,
                                   const QString& proxyHostname,
                                   int proxyPort,
                                   const QString& proxyUser,
                                   const QString& proxyPassword,

                                   bool https,
                                   bool selfSignedCertificateEnabled,
                                   const QByteArray& selfSignedCertificate,
                                   bool clientCertificateEnabled,
                                   const QByteArray& clientCertificate,

                                   bool authentication,
                                   const QString& username,
                                   const QString& password,

                                   int updateInterval,
                                   int timeout,

                                   bool autoReconnectEnabled,
                                   int autoReconnectInterval,

                                   const QVariantMap& mountedDirectories);
        Q_INVOKABLE void removeServerAtIndex(const QModelIndex& index);
        Q_INVOKABLE void removeServerAtRow(int row);
    private:
        int serverRow(const QString& name) const;

        std::vector<Server> mServers;
        QString mCurrentServer;
    };
}

#endif // TREMOTESF_SERVERSMODEL_H
