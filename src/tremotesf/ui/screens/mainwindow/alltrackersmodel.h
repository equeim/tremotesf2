// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ALLTRACKERSMODEL_H
#define TREMOTESF_ALLTRACKERSMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"

namespace tremotesf {
    class AllTrackersModel : public BaseTorrentsFiltersSettingsModel {
        Q_OBJECT
    public:
        static constexpr auto TrackerRole = Qt::UserRole;

        inline explicit AllTrackersModel(QObject* parent = nullptr) : BaseTorrentsFiltersSettingsModel(parent){};

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        QModelIndex indexForTracker(const QString& tracker) const;

        QModelIndex indexForTorrentsProxyModelFilter() const override;

    protected:
        void resetTorrentsProxyModelFilter() const override;

    private:
        void update() override;

        struct TrackerItem {
            QString tracker;
            int torrents;
        };

        std::vector<TrackerItem> mTrackers;

        template<typename, typename, typename, typename>
        friend class ModelListUpdater;
        friend class AllTrackersModelUpdater;
    };
}

#endif // TREMOTESF_ALLTRACKERSMODEL_H
