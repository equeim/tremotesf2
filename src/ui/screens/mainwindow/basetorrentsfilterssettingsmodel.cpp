// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basetorrentsfilterssettingsmodel.h"

#include <QTimer>

#include "rpc/rpc.h"

namespace tremotesf {
    Rpc* BaseTorrentsFiltersSettingsModel::rpc() const { return mRpc; }

    void BaseTorrentsFiltersSettingsModel::setRpc(Rpc* rpc) {
        if (rpc != mRpc.data()) {
            if (const auto oldRpc = mRpc.data(); oldRpc) {
                QObject::disconnect(oldRpc, nullptr, this, nullptr);
            }
            mRpc = rpc;
            if (rpc) {
                QObject::connect(rpc, &Rpc::torrentsUpdated, this, &BaseTorrentsFiltersSettingsModel::updateImpl);
                QTimer::singleShot(0, this, &BaseTorrentsFiltersSettingsModel::updateImpl);
            }
        }
    }

    TorrentsProxyModel* BaseTorrentsFiltersSettingsModel::torrentsProxyModel() const { return mTorrentsProxyModel; }

    void BaseTorrentsFiltersSettingsModel::setTorrentsProxyModel(TorrentsProxyModel* model) {
        mTorrentsProxyModel = model;
    }

    bool BaseTorrentsFiltersSettingsModel::isPopulated() const { return mPopulated; }

    void BaseTorrentsFiltersSettingsModel::updateImpl() {
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
