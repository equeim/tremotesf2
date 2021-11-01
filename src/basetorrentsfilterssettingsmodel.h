/*
 * Tremotesf
 * Copyright (C) 2015-2021 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_BASETORRENTSFILTERSSETTINGSMODEL_H
#define TREMOTESF_BASETORRENTSFILTERSSETTINGSMODEL_H

#include <QAbstractListModel>

namespace tremotesf
{
    class Rpc;
    class TorrentsProxyModel;

    class BaseTorrentsFiltersSettingsModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(tremotesf::Rpc* rpc READ rpc WRITE setRpc NOTIFY rpcChanged)
        Q_PROPERTY(tremotesf::TorrentsProxyModel* torrentsProxyModel READ torrentsProxyModel WRITE setTorrentsProxyModel NOTIFY torrentsProxyModelChanged)
        Q_PROPERTY(bool populated READ isPopulated NOTIFY populatedChanged)
    public:
        inline explicit BaseTorrentsFiltersSettingsModel(QObject* parent = nullptr) : QAbstractListModel(parent) {};

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        TorrentsProxyModel* torrentsProxyModel() const;
        void setTorrentsProxyModel(TorrentsProxyModel* model);

        bool isPopulated() const;

        virtual QModelIndex indexForTorrentsProxyModelFilter() const = 0;

    protected:
        virtual void update() = 0;
        virtual void resetTorrentsProxyModelFilter() const = 0;

    private:
        void updateImpl();

        Rpc* mRpc = nullptr;
        TorrentsProxyModel* mTorrentsProxyModel = nullptr;
        bool mPopulated = false;

    signals:
        void rpcChanged();
        void torrentsProxyModelChanged();
        void populatedChanged();
    };
}

#endif // TREMOTESF_BASETORRENTSFILTERSSETTINGSMODEL_H
