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

#ifndef TREMOTESF_DOWNLOADDIRECTORIESMODEL_H
#define TREMOTESF_DOWNLOADDIRECTORIESMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QStringList>

#ifdef TREMOTESF_SAILFISHOS
#include <QQmlParserStatus>
#endif

namespace tremotesf
{
    class Rpc;
    class TorrentsProxyModel;

#ifdef TREMOTESF_SAILFISHOS
    class DownloadDirectoriesModel : public QAbstractListModel, public QQmlParserStatus
#else
    class DownloadDirectoriesModel : public QAbstractListModel
#endif
    {
        Q_OBJECT
#ifdef TREMOTESF_SAILFISHOS
        Q_INTERFACES(QQmlParserStatus)
#endif
        Q_PROPERTY(tremotesf::Rpc* rpc READ rpc WRITE setRpc NOTIFY rpcChanged)
        Q_PROPERTY(tremotesf::TorrentsProxyModel* torrentsProxyModel READ torrentsProxyModel WRITE setTorrentsProxyModel)
    public:
#ifndef TREMOTESF_SAILFISHOS
        static const int DirectoryRole = Qt::UserRole;
#endif
        explicit DownloadDirectoriesModel(Rpc* rpc = nullptr,
                                          TorrentsProxyModel* torrentsProxyModel = nullptr,
                                          QObject* parent = nullptr);

#ifdef TREMOTESF_SAILFISHOS
        void classBegin() override;
        void componentComplete() override;
#endif

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex&) const override;
        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        TorrentsProxyModel* torrentsProxyModel() const;
        void setTorrentsProxyModel(TorrentsProxyModel* model);

#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif
    private:
        struct DirectoryItem
        {
            QString directory;
            int torrents;
        };

        void update();

        Rpc* mRpc;
        TorrentsProxyModel* mTorrentsProxyModel;
        std::vector<DirectoryItem> mDirectories;

    signals:
        void rpcChanged();
    };
}

#endif // TREMOTESF_DOWNLOADDIRECTORIESMODEL_H
