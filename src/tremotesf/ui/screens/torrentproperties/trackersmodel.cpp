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

#include "trackersmodel.h"

#include <QCoreApplication>

#include "tremotesf/ui/itemmodels/modelutils.h"
#include "tremotesf/utils.h"

#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"

namespace tremotesf
{
    using libtremotesf::Tracker;

    namespace
    {
        QString trackerStatusString(const Tracker& tracker)
        {
            switch (tracker.status()) {
            case Tracker::Inactive:
                //: Tracker status
                return qApp->translate("tremotesf", "Inactive");
            case Tracker::Active:
                return qApp->translate("tremotesf", "Active", "Tracker status");
            case Tracker::Queued:
                return qApp->translate("tremotesf", "Queued", "Tracker status");
            case Tracker::Updating:
                //: Tracker status
                return qApp->translate("tremotesf", "Updating");
            case Tracker::Error:
            {
                if (tracker.errorMessage().isEmpty()) {
                    return qApp->translate("tremotesf", "Error");
                }
                return qApp->translate("tremotesf", "Error: %1").arg(tracker.errorMessage());
            }
            default:
                return QString();
            }
        }
    }

    TrackersModel::TrackersModel(libtremotesf::Torrent* torrent, QObject* parent)
        : QAbstractTableModel(parent),
          mTorrent(nullptr)
    {
        setTorrent(torrent);
    }

    QVariant TrackersModel::data(const QModelIndex& index, int role) const
    {
        const Tracker& tracker = mTrackers[static_cast<size_t>(index.row())];
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case AnnounceColumn:
                return tracker.announce();
            case StatusColumn:
                return trackerStatusString(tracker);
            case PeersColumn:
                return tracker.peers();
            case NextUpdateColumn:
                if (tracker.nextUpdateEta() >= 0) {
                    return Utils::formatEta(tracker.nextUpdateEta());
                }
                break;
            }
        } else if (role == SortRole) {
            if (index.column() == NextUpdateColumn) {
                return tracker.nextUpdateTime();
            }
            return data(index, Qt::DisplayRole);
        }
        return QVariant();
    }

    QVariant TrackersModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (section) {
        case AnnounceColumn:
            return qApp->translate("tremotesf", "Address");
        case StatusColumn:
            return qApp->translate("tremotesf", "Status");
        case NextUpdateColumn:
            return qApp->translate("tremotesf", "Next Update");
        case PeersColumn:
            return qApp->translate("tremotesf", "Peers");
        default:
            return {};
        }
    }

    int TrackersModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mTrackers.size());
    }

    libtremotesf::Torrent* TrackersModel::torrent() const
    {
        return mTorrent;
    }

    void TrackersModel::setTorrent(libtremotesf::Torrent* torrent)
    {
        if (torrent != mTorrent) {
            if (mTorrent) {
                QObject::disconnect(mTorrent, nullptr, this, nullptr);
            }

            mTorrent = torrent;

            if (mTorrent) {
                update();
                QObject::connect(mTorrent, &libtremotesf::Torrent::updated, this, &TrackersModel::update);
            } else {
                beginResetModel();
                mTrackers.clear();
                endResetModel();
            }
        }
    }

    QVariantList TrackersModel::idsFromIndexes(const QModelIndexList& indexes) const
    {
        QVariantList ids;
        ids.reserve(indexes.size());
        for (const QModelIndex& index : indexes) {
            ids.append(mTrackers[static_cast<size_t>(index.row())].id());
        }
        return ids;
    }

    const libtremotesf::Tracker& TrackersModel::trackerAtIndex(const QModelIndex& index) const
    {
        return mTrackers[static_cast<size_t>(index.row())];
    }

    class TrackersModelUpdater : public ModelListUpdater<TrackersModel, Tracker> {
    public:
        inline explicit TrackersModelUpdater(TrackersModel& model) : ModelListUpdater(model) {}

    protected:
        std::vector<Tracker>::iterator findNewItemForItem(std::vector<Tracker>& newItems, const Tracker& item) override {
            return std::find_if(newItems.begin(), newItems.end(), [item](const Tracker& tracker) { return tracker.id() == item.id(); });
        }

        bool updateItem(Tracker& item, Tracker&& newItem) override {
            return item != newItem;
        }

        Tracker createItemFromNewItem(Tracker&& newItem) override {
            return std::move(newItem);
        }
    };

    void TrackersModel::update()
    {
        TrackersModelUpdater updater(*this);
        updater.update(mTrackers, std::vector(mTorrent->trackers()));
    }
}
