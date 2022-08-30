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

#include "basetorrentsfilterssettingsmodel.h"

#include <QTimer>

#include "tremotesf/rpc/trpc.h"

namespace tremotesf
{
    Rpc* BaseTorrentsFiltersSettingsModel::rpc() const
    {
        return mRpc;
    }

    void BaseTorrentsFiltersSettingsModel::setRpc(Rpc* rpc)
    {
        if (rpc != mRpc) {
            if (mRpc) {
                QObject::disconnect(mRpc, nullptr, this, nullptr);
            }
            mRpc = rpc;
            emit rpcChanged();
            if (rpc) {
                QObject::connect(rpc, &Rpc::torrentsUpdated, this, &BaseTorrentsFiltersSettingsModel::updateImpl);
                QTimer::singleShot(0, this, &BaseTorrentsFiltersSettingsModel::updateImpl);
            }
        }
    }

    TorrentsProxyModel* BaseTorrentsFiltersSettingsModel::torrentsProxyModel() const
    {
        return mTorrentsProxyModel;
    }

    void BaseTorrentsFiltersSettingsModel::setTorrentsProxyModel(TorrentsProxyModel* model)
    {
        if (model != mTorrentsProxyModel) {
            mTorrentsProxyModel = model;
            emit torrentsProxyModelChanged();
        }
    }

    bool BaseTorrentsFiltersSettingsModel::isPopulated() const
    {
        return mPopulated;
    }

    void BaseTorrentsFiltersSettingsModel::updateImpl()
    {
        bool populatedChanged = false;
        if (mPopulated != mRpc->isConnected()) {
            mPopulated = mRpc->isConnected();
            populatedChanged = true;
        }
        update();
        if (!indexForTorrentsProxyModelFilter().isValid()) {
            resetTorrentsProxyModelFilter();
        }
        if (populatedChanged) {
            emit this->populatedChanged();
        }
    }
}
