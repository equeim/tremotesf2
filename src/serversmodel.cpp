/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "serversmodel.h"

namespace tremotesf
{
    ServersModel::ServersModel(QObject* parent)
        : QAbstractListModel(parent),
          mServers(Servers::instance()->servers()),
          mCurrentServer(Servers::instance()->currentServerName())
    {
    }

    QVariant ServersModel::data(const QModelIndex& index, int role) const
    {
        const Server& server = mServers.at(index.row());
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return server.name;
        case IsCurrentRole:
            return (server.name == mCurrentServer);
        case AddressRole:
            return server.address;
        case PortRole:
            return server.port;
        case ApiPathRole:
            return server.apiPath;
        case HttpsRole:
            return server.https;
        case SelfSignedCertificateEnabledRole:
            return server.selfSignedCertificateEnabled;
        case SelfSignedCertificateRole:
            return server.selfSignedCertificate;
        case ClientCertificateEnabledRole:
            return server.clientCertificateEnabled;
        case ClientCertificateRole:
            return server.clientCertificate;
        case AuthenticationRole:
            return server.authentication;
        case UsernameRole:
            return server.username;
        case PasswordRole:
            return server.password;
        case UpdateIntervalRole:
            return server.updateInterval;
        case TimeoutRole:
            return server.timeout;
        }
#else
        switch (role) {
        case Qt::CheckStateRole:
            if (server.name == mCurrentServer) {
                return Qt::Checked;
            }
            return Qt::Unchecked;
        case Qt::DisplayRole:
            return server.name;
        }
#endif
        return QVariant();
    }

    Qt::ItemFlags ServersModel::flags(const QModelIndex& index) const
    {
        return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
    }

    int ServersModel::rowCount(const QModelIndex&) const
    {
        return mServers.size();
    }

    bool ServersModel::setData(const QModelIndex& modelIndex, const QVariant& value, int role)
    {
#ifdef TREMOTESF_SAILFISHOS
        if (role == IsCurrentRole && value.toBool()) {
#else
        if (role == Qt::CheckStateRole && value.toInt() == Qt::Checked) {
#endif
            const QString current(mServers.at(modelIndex.row()).name);
            if (current != mCurrentServer) {
                mCurrentServer = current;
                emit dataChanged(index(0), index(mServers.size() - 1));
                return true;
            }
        }
        return false;
    }

    const QList<Server>& ServersModel::servers() const
    {
        return mServers;
    }

    const QString& ServersModel::currentServerName() const
    {
        return mCurrentServer;
    }

    bool ServersModel::hasServer(const QString& name) const
    {
        if (serverRow(name) == -1) {
            return false;
        }
        return true;
    }

    void ServersModel::setServer(const QString& oldName,
                                 const QString& name,
                                 const QString& address,
                                 int port,
                                 const QString& apiPath,
                                 bool https,
                                 bool selfSignedCertificateEnabled,
                                 const QByteArray& selfSignedCertificate,
                                 bool clientCertificateEnabled,
                                 const QByteArray& clientCertificate,
                                 bool authentication,
                                 const QString& username,
                                 const QString& password,
                                 int updateInterval,
                                 int timeout)
    {
        if (!oldName.isEmpty() && name != oldName) {
            const int row = serverRow(oldName);
            if (row != -1) {
                beginRemoveRows(QModelIndex(), row, row);
                mServers.removeAt(row);
                endRemoveRows();
            }
        }

        int row = serverRow(name);
        if (row == -1) {
            row = mServers.size();
            beginInsertRows(QModelIndex(), row, row);
            if (row == 0) {
                mCurrentServer = name;
            }
            mServers.append(Server{name,
                                   address,
                                   port,
                                   apiPath,
                                   https,
                                   selfSignedCertificateEnabled,
                                   selfSignedCertificate,
                                   clientCertificateEnabled,
                                   clientCertificate,
                                   authentication,
                                   username,
                                   password,
                                   updateInterval,
                                   timeout});
            endInsertRows();
        } else {
            Server& server = mServers[row];
            server.address = address;
            server.port = port;
            server.apiPath = apiPath;
            server.https = https;
            server.selfSignedCertificateEnabled = selfSignedCertificateEnabled;
            server.selfSignedCertificate = selfSignedCertificate;
            server.clientCertificateEnabled = clientCertificateEnabled;
            server.clientCertificate = clientCertificate;
            server.authentication = authentication;
            server.username = username;
            server.password = password;
            server.updateInterval = updateInterval;
            server.timeout = timeout;
            if (oldName == mCurrentServer) {
                mCurrentServer = name;
            }
            const QModelIndex modelIndex(index(row));
            emit dataChanged(modelIndex, modelIndex);
        }
    }

    void ServersModel::removeServerAtIndex(const QModelIndex& index)
    {
        removeServerAtRow(index.row());
    }

    void ServersModel::removeServerAtRow(int row)
    {
        const bool current = (mServers.at(row).name == mCurrentServer);
        beginRemoveRows(QModelIndex(), row, row);
        mServers.removeAt(row);
        endRemoveRows();
        if (current && !mServers.isEmpty()) {
            mCurrentServer = mServers.first().name;
            const QModelIndex modelIndex(index(0, 0));
            emit dataChanged(modelIndex, modelIndex);
        }
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> ServersModel::roleNames() const
    {
        return {{NameRole, "name"},
                {IsCurrentRole, "current"},
                {AddressRole, "address"},
                {PortRole, "port"},
                {ApiPathRole, "apiPath"},
                {HttpsRole, "https"},
                {SelfSignedCertificateEnabledRole, "selfSignedCertificateEnabled"},
                {SelfSignedCertificateRole, "selfSignedCertificate"},
                {ClientCertificateEnabledRole, "clientCertificateEnabled"},
                {ClientCertificateRole, "clientCertificate"},
                {AuthenticationRole, "authentication"},
                {UsernameRole, "username"},
                {PasswordRole, "password"},
                {UpdateIntervalRole, "updateInterval"},
                {TimeoutRole, "timeout"}};
    }
#endif

    int ServersModel::serverRow(const QString& name) const
    {
        for (int i = 0, max = mServers.size(); i < max; ++i) {
            if (mServers.at(i).name == name) {
                return i;
            }
        }
        return -1;
    }
}
