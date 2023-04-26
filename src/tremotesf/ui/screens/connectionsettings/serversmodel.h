// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERSMODEL_H
#define TREMOTESF_SERVERSMODEL_H

#include <vector>
#include <QAbstractListModel>

#include "tremotesf/rpc/servers.h"

namespace tremotesf {
    class ServersModel final : public QAbstractListModel {
        Q_OBJECT

    public:
        explicit ServersModel(QObject* parent = nullptr);

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        int rowCount(const QModelIndex& = {}) const override;
        bool setData(const QModelIndex& modelIndex, const QVariant& value, int role = Qt::EditRole) override;

        const std::vector<Server>& servers() const;
        const QString& currentServerName() const;

        bool hasServer(const QString& name) const;

        void setServer(
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
        );
        void removeServerAtIndex(const QModelIndex& index);
        void removeServerAtRow(int row);

    private:
        int serverRow(const QString& name) const;

        std::vector<Server> mServers;
        QString mCurrentServer;
    };
}

#endif // TREMOTESF_SERVERSMODEL_H
