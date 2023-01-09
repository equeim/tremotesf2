// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serversmodel.h"

namespace tremotesf {
    ServersModel::ServersModel(QObject* parent)
        : QAbstractListModel(parent),
          mServers(Servers::instance()->servers()),
          mCurrentServer(Servers::instance()->currentServerName()) {}

    QVariant ServersModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const Server& server = mServers.at(static_cast<size_t>(index.row()));
        switch (role) {
        case Qt::CheckStateRole:
            if (server.name == mCurrentServer) {
                return Qt::Checked;
            }
            return Qt::Unchecked;
        case Qt::DisplayRole:
            return server.name;
        default:
            return {};
        }
    }

    Qt::ItemFlags ServersModel::flags(const QModelIndex& index) const {
        if (!index.isValid()) {
            return {};
        }
        return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
    }

    int ServersModel::rowCount(const QModelIndex&) const { return static_cast<int>(mServers.size()); }

    bool ServersModel::setData(const QModelIndex& modelIndex, const QVariant& value, int role) {
        if (!modelIndex.isValid() || role != Qt::CheckStateRole || value.value<Qt::CheckState>() != Qt::Checked) {
            return false;
        }
        const auto& current = mServers.at(static_cast<size_t>(modelIndex.row()));
        if (current.name != mCurrentServer) {
            mCurrentServer = current.name;
            emit dataChanged(index(0), index(static_cast<int>(mServers.size()) - 1));
            return true;
        }
        return false;
    }

    const std::vector<Server>& ServersModel::servers() const { return mServers; }

    const QString& ServersModel::currentServerName() const { return mCurrentServer; }

    bool ServersModel::hasServer(const QString& name) const { return serverRow(name) != -1; }

    void ServersModel::setServer(
        const QString& oldName,
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
        int timeout,

        bool autoReconnectEnabled,
        int autoReconnectInterval,

        const std::vector<MountedDirectory>& mountedDirectories
    ) {
        const int oldRow = serverRow(oldName);
        int row = serverRow(name);

        Server* const server = [=]() -> Server* {
            if (oldRow != -1) {
                return &mServers.at(static_cast<size_t>(oldRow));
            }
            if (row != -1) {
                return &mServers.at(static_cast<size_t>(row));
            }
            return nullptr;
        }();

        if (server) {
            // Overwrite an existing server

            server->name = name;
            server->address = address;
            server->port = port;
            server->apiPath = apiPath;

            server->proxyType = proxyType;
            server->proxyHostname = proxyHostname;
            server->proxyPort = proxyPort;
            server->proxyUser = proxyUser;
            server->proxyPassword = proxyPassword;

            server->https = https;
            server->selfSignedCertificateEnabled = selfSignedCertificateEnabled;
            server->selfSignedCertificate = selfSignedCertificate;
            server->clientCertificateEnabled = clientCertificateEnabled;
            server->clientCertificate = clientCertificate;

            server->authentication = authentication;
            server->username = username;
            server->password = password;

            server->updateInterval = updateInterval;
            server->timeout = timeout;

            server->autoReconnectEnabled = autoReconnectEnabled;
            server->autoReconnectInterval = autoReconnectInterval;

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
            row = static_cast<int>(mServers.size());
            beginInsertRows(QModelIndex(), row, row);
            mServers.emplace_back(
                name,
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
                timeout,

                autoReconnectEnabled,
                autoReconnectInterval,

                mountedDirectories,
                LastTorrents{},
                QStringList{},
                QString{}
            );
            endInsertRows();
            if (row == 0) {
                mCurrentServer = name;
            }
        }
    }

    void ServersModel::removeServerAtIndex(const QModelIndex& index) { removeServerAtRow(index.row()); }

    void ServersModel::removeServerAtRow(int row) {
        const bool current = (mServers.at(static_cast<size_t>(row)).name == mCurrentServer);
        beginRemoveRows(QModelIndex(), row, row);
        mServers.erase(mServers.begin() + row);
        endRemoveRows();
        if (current && !mServers.empty()) {
            mCurrentServer = mServers.front().name;
            const QModelIndex modelIndex(index(0, 0));
            emit dataChanged(modelIndex, modelIndex);
        }
    }

    int ServersModel::serverRow(const QString& name) const {
        for (size_t i = 0, max = mServers.size(); i < max; ++i) {
            if (mServers[i].name == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
}
