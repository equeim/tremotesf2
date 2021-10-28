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

#ifndef TREMOTESF_ALLTRACKERSMODEL_H
#define TREMOTESF_ALLTRACKERSMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"

namespace tremotesf
{
    class AllTrackersModel : public BaseTorrentsFiltersSettingsModel
    {
        Q_OBJECT
    public:
        enum Role
        {
            TrackerRole = Qt::UserRole,
#ifdef TREMOTESF_SAILFISHOS
            TorrentsRole
#endif
        };

        inline explicit AllTrackersModel(QObject* parent = nullptr) : BaseTorrentsFiltersSettingsModel(parent) {};

        QVariant data(const QModelIndex& index, int role) const override;
        int rowCount(const QModelIndex&) const override;
        bool removeRows(int row, int count, const QModelIndex& parent) override;

        Q_INVOKABLE QModelIndex indexForTracker(const QString& tracker) const;

#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif
    private:
        void update() override;

        struct TrackerItem
        {
            QString tracker;
            int torrents;
        };

        std::vector<TrackerItem> mTrackers;
    };
}

#endif // TREMOTESF_ALLTRACKERSMODEL_H
