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
        const Server& server = mServers[index.row()];
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
        case BackgroundUpdateIntervalRole:
            return server.backgroundUpdateInterval;
        case TimeoutRole:
            return server.timeout;
        case MountedDirectoriesRole:
            return server.mountedDirectories;
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
            const QString current(mServers[modelIndex.row()].name);
            if (current != mCurrentServer) {
                mCurrentServer = current;
                emit dataChanged(index(0), index(mServers.size() - 1));
                return true;
            }
        }
        return false;
    }

    const std::vector<Server>& ServersModel::servers() const
    {
        return mServers;
    }

    const QString& ServersModel::currentServerName() const
    {
        return mCurrentServer;
    }

    bool ServersModel::hasServer(const QString& name) const
    {
        return serverRow(name) != -1;
    }

    void ServersModel::setServer(const QString& oldName,
                                 const QString& name,
                                 const QString& address,
                                 int port,
                                 const QString& apiPath,

                                 Server::ProxyType proxyType,
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
                                 int backgroundUpdateInterval,
                                 int timeout,

                                 const QVariantMap& mountedDirectories)
    {
        const int oldRow = serverRow(oldName);
        int row = serverRow(name);

        Server *const server = [=]() -> Server* {
            if (oldRow != -1) {
                return &mServers[oldRow];
            }
            if (row != -1) {
                return &mServers[row];
            }
            return nullptr;
        }();

        if (server) {
            // Overwrite an existing server

            server->name = name;
            server->address = address;
            server->port = port;
            server->apiPath = apiPath;
            server->https = https;
            server->selfSignedCertificateEnabled = selfSignedCertificateEnabled;
            server->selfSignedCertificate = selfSignedCertificate;
            server->clientCertificateEnabled = clientCertificateEnabled;
            server->clientCertificate = clientCertificate;
            server->authentication = authentication;
            server->username = username;
            server->password = password;
            server->updateInterval = updateInterval;
            server->backgroundUpdateInterval = backgroundUpdateInterval;
            server->timeout = timeout;
            server->mountedDirectories = mountedDirectories;
            const QModelIndex modelIndex(index(oldRow));
            emit dataChanged(modelIndex, modelIndex);

            if (oldRow != -1 && row != -1 && row != oldRow) {
                // Remove overwritten server if we overwrite when renaming
                beginRemoveRows(QModelIndex(), row, row);
                mServers.erase(mServers.begin() + row);
                endRemoveRows();
            }

            if (oldName == mCurrentServer) {
                mCurrentServer = name;
            }
        } else {
            row = mServers.size();
            beginInsertRows(QModelIndex(), row, row);
            mServers.emplace_back(name,
                                  address,
                                  port,
                                  apiPath,

                                  proxyType,
                                  proxyHostname,
                                  proxyPort,
                                  proxyUser,
                                  proxyPassword,

                                  https,
                                  selfSignedCertificateEnabled,
                                  selfSignedCertificate,
                                  clientCertificateEnabled,
                                  clientCertificate,

                                  authentication,
                                  username,
                                  password,

                                  updateInterval,
                                  backgroundUpdateInterval,
                                  timeout,

                                  mountedDirectories,
                                  QVariant(),
                                  QVariant());
            endInsertRows();
            if (row == 0) {
                mCurrentServer = name;
            }
        }
    }

    void ServersModel::removeServerAtIndex(const QModelIndex& index)
    {
        removeServerAtRow(index.row());
    }

    void ServersModel::removeServerAtRow(int row)
    {
        const bool current = (mServers[row].name == mCurrentServer);
        beginRemoveRows(QModelIndex(), row, row);
        mServers.erase(mServers.begin() + row);
        endRemoveRows();
        if (current && !mServers.empty()) {
            mCurrentServer = mServers.front().name;
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
                {BackgroundUpdateIntervalRole, "backgroundUpdateInterval"},
                {TimeoutRole, "timeout"},
                {MountedDirectoriesRole, "mountedDirectories"}};
    }
#endif

    int ServersModel::serverRow(const QString& name) const
    {
        for (int i = 0, max = mServers.size(); i < max; ++i) {
            if (mServers[i].name == name) {
                return i;
            }
        }
        return -1;
    }
}
