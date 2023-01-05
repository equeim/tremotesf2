// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trackersmodel.h"

#include <QCoreApplication>
#include <QMetaEnum>

#include "tremotesf/ui/itemmodels/modelutils.h"
#include "tremotesf/utils.h"

#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"

namespace tremotesf {
    using libtremotesf::Tracker;

    namespace {
        QString trackerStatusString(const Tracker& tracker) {
            switch (tracker.status()) {
            case Tracker::Status::Inactive:
                if (!tracker.errorMessage().isEmpty()) {
                    return qApp->translate("tremotesf", "Error: %1").arg(tracker.errorMessage());
                }
                return qApp->translate("tremotesf", "Inactive", "Tracker status");
            case Tracker::Status::WaitingForUpdate:
                return qApp->translate("tremotesf", "Waiting for update", "Tracker status");
            case Tracker::Status::QueuedForUpdate:
                return qApp->translate("tremotesf", "About to update", "Tracker status");
            case Tracker::Status::Updating:
                return qApp->translate("tremotesf", "Updating", "Tracker status");
            }
            return {};
        }
    }

    TrackersModel::TrackersModel(libtremotesf::Torrent* torrent, QObject* parent)
        : QAbstractTableModel(parent), mTorrent(nullptr) {
        setTorrent(torrent);
    }

    int TrackersModel::columnCount(const QModelIndex&) const { return QMetaEnum::fromType<Column>().keyCount(); }

    QVariant TrackersModel::data(const QModelIndex& index, int role) const {
        const Tracker& tracker = mTrackers[static_cast<size_t>(index.row())];
        if (role == Qt::DisplayRole) {
            switch (static_cast<Column>(index.column())) {
            case Column::Announce:
                return tracker.announce();
            case Column::Status:
                return trackerStatusString(tracker);
            case Column::Peers:
                return tracker.peers();
            case Column::NextUpdate:
                if (tracker.nextUpdateEta() >= 0) {
                    return Utils::formatEta(tracker.nextUpdateEta());
                }
                break;
            }
        } else if (role == SortRole) {
            if (static_cast<Column>(index.column()) == Column::NextUpdate) {
                return tracker.nextUpdateTime();
            }
            return data(index, Qt::DisplayRole);
        }
        return {};
    }

    QVariant TrackersModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (static_cast<Column>(section)) {
        case Column::Announce:
            return qApp->translate("tremotesf", "Address");
        case Column::Status:
            return qApp->translate("tremotesf", "Status");
        case Column::NextUpdate:
            return qApp->translate("tremotesf", "Next Update");
        case Column::Peers:
            return qApp->translate("tremotesf", "Peers");
        default:
            return {};
        }
    }

    int TrackersModel::rowCount(const QModelIndex&) const { return static_cast<int>(mTrackers.size()); }

    libtremotesf::Torrent* TrackersModel::torrent() const { return mTorrent; }

    void TrackersModel::setTorrent(libtremotesf::Torrent* torrent) {
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

    std::vector<int> TrackersModel::idsFromIndexes(const QModelIndexList& indexes) const {
        std::vector<int> ids{};
        ids.reserve(static_cast<size_t>(indexes.size()));
        std::transform(indexes.begin(), indexes.end(), std::back_inserter(ids), [this](const QModelIndex& index) {
            return mTrackers[static_cast<size_t>(index.row())].id();
        });
        return ids;
    }

    const libtremotesf::Tracker& TrackersModel::trackerAtIndex(const QModelIndex& index) const {
        return mTrackers[static_cast<size_t>(index.row())];
    }

    class TrackersModelUpdater : public ModelListUpdater<TrackersModel, Tracker> {
    public:
        inline explicit TrackersModelUpdater(TrackersModel& model) : ModelListUpdater(model) {}

    protected:
        std::vector<Tracker>::iterator
        findNewItemForItem(std::vector<Tracker>& newItems, const Tracker& item) override {
            return std::find_if(newItems.begin(), newItems.end(), [item](const Tracker& tracker) {
                return tracker.id() == item.id();
            });
        }

        bool updateItem(Tracker& item, Tracker&& newItem) override { return item != newItem; }

        Tracker createItemFromNewItem(Tracker&& newItem) override { return std::move(newItem); }
    };

    void TrackersModel::update() {
        TrackersModelUpdater updater(*this);
        updater.update(mTrackers, std::vector(mTorrent->trackers()));
    }
}
