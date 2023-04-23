// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
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

        libtremotesf::ConnectionConfiguration::ProxyType proxyType,
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

        Server* const server = [=, this]() -> Server* {
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
            server->connectionConfiguration.address = address;
            server->connectionConfiguration.port = port;
            server->connectionConfiguration.apiPath = apiPath;

            server->connectionConfiguration.proxyType = proxyType;
            server->connectionConfiguration.proxyHostname = proxyHostname;
            server->connectionConfiguration.proxyPort = proxyPort;
            server->connectionConfiguration.proxyUser = proxyUser;
            server->connectionConfiguration.proxyPassword = proxyPassword;

            server->connectionConfiguration.https = https;
            server->connectionConfiguration.selfSignedCertificateEnabled = selfSignedCertificateEnabled;
            server->connectionConfiguration.selfSignedCertificate = selfSignedCertificate;
            server->connectionConfiguration.clientCertificateEnabled = clientCertificateEnabled;
            server->connectionConfiguration.clientCertificate = clientCertificate;

            server->connectionConfiguration.authentication = authentication;
            server->connectionConfiguration.username = username;
            server->connectionConfiguration.password = password;

            server->connectionConfiguration.updateInterval = updateInterval;
            server->connectionConfiguration.timeout = timeout;

            server->connectionConfiguration.autoReconnectEnabled = autoReconnectEnabled;
            server->connectionConfiguration.autoReconnectInterval = autoReconnectInterval;

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
            mServers.push_back(Server{
                .name = name,
                .connectionConfiguration =
                    libtremotesf::ConnectionConfiguration{
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
                        autoReconnectInterval},
                .mountedDirectories = mountedDirectories,
                .lastTorrents = {},
                .lastDownloadDirectories = {},
                .lastDownloadDirectory = {}});
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
