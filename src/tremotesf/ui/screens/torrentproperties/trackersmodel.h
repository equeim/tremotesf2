// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TRACKERSMODEL_H
#define TREMOTESF_TRACKERSMODEL_H

#include <vector>

#include <QAbstractTableModel>

namespace libtremotesf {
    class Torrent;
    class Tracker;
}

namespace tremotesf {
    class TrackersModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        enum class Column { Announce, Status, Error, NextUpdate, Peers };
        Q_ENUM(Column)

        static constexpr auto SortRole = Qt::UserRole;

        explicit TrackersModel(libtremotesf::Torrent* torrent = nullptr, QObject* parent = nullptr);

        int columnCount(const QModelIndex& = {}) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

        QVariantList idsFromIndexes(const QModelIndexList& indexes) const;
        const libtremotesf::Tracker& trackerAtIndex(const QModelIndex& index) const;

    private:
        void update();

        libtremotesf::Torrent* mTorrent;
        std::vector<libtremotesf::Tracker> mTrackers;

        template<typename, typename, typename, typename>
        friend class ModelListUpdater;
        friend class TrackersModelUpdater;
    };
}

#endif // TREMOTESF_TRACKERSMODEL_H
