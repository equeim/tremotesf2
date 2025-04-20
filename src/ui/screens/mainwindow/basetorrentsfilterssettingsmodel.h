// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_BASETORRENTSFILTERSSETTINGSMODEL_H
#define TREMOTESF_BASETORRENTSFILTERSSETTINGSMODEL_H

#include <QAbstractListModel>
#include <QPointer>

namespace tremotesf {
    class Rpc;
    class TorrentsProxyModel;

    class BaseTorrentsFiltersSettingsModel : public QAbstractListModel {
        Q_OBJECT

    public:
        inline explicit BaseTorrentsFiltersSettingsModel(QObject* parent = nullptr) : QAbstractListModel(parent) {};

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        TorrentsProxyModel* torrentsProxyModel() const;
        void setTorrentsProxyModel(TorrentsProxyModel* model);

        bool isPopulated() const;

        virtual QModelIndex indexForTorrentsProxyModelFilter() const = 0;

        // Needed for ModelListUpdater
        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

    protected:
        virtual void update() = 0;
        virtual void resetTorrentsProxyModelFilter() const = 0;

    private:
        void updateImpl();

        QPointer<Rpc> mRpc{};
        TorrentsProxyModel* mTorrentsProxyModel{};
        bool mPopulated = false;

    signals:
        void populatedChanged();
    };
}

#endif // TREMOTESF_BASETORRENTSFILTERSSETTINGSMODEL_H
