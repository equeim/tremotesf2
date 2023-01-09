// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trackersmodel.h"

#include <chrono>
#include <optional>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QTimer>

#include "tremotesf/ui/itemmodels/modelutils.h"
#include "tremotesf/utils.h"

#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"

namespace tremotesf {
    using std::chrono::seconds;
    using namespace std::chrono_literals;

    namespace {
        QString trackerStatusString(const libtremotesf::Tracker& tracker) {
            using libtremotesf::Tracker;
            switch (tracker.status()) {
            case Tracker::Status::Inactive:
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

        std::optional<seconds> nextUpdateEtaFor(const libtremotesf::Tracker& tracker) {
            if (!tracker.nextUpdateTime().isValid()) return std::nullopt;
            const auto secs = QDateTime::currentDateTimeUtc().secsTo(tracker.nextUpdateTime());
            if (secs < 0) return std::nullopt;
            return seconds(secs);
        }
    }

    struct TrackersModel::TrackerItem {
        libtremotesf::Tracker tracker;
        std::optional<seconds> nextUpdateEta{};

        TrackerItem(libtremotesf::Tracker tracker)
            : tracker(std::move(tracker)), nextUpdateEta(nextUpdateEtaFor(this->tracker)) {}

        [[nodiscard]] bool operator==(const TrackerItem& other) const {
            return tracker == other.tracker && nextUpdateEta == other.nextUpdateEta;
        }

        [[nodiscard]] bool operator!=(const TrackerItem& other) const { return !(*this == other); }

        [[nodiscard]] TrackerItem withUpdatedEta() const {
            TrackerItem updated = *this;
            updated.nextUpdateEta = nextUpdateEtaFor(tracker);
            return updated;
        }
    };

    TrackersModel::TrackersModel(libtremotesf::Torrent* torrent, QObject* parent)
        : QAbstractTableModel(parent), mTorrent(nullptr), mTrackers{}, mEtaUpdateTimer(new QTimer(this)) {
        mEtaUpdateTimer->setInterval(1s);
        mEtaUpdateTimer->setSingleShot(false);
        QObject::connect(mEtaUpdateTimer, &QTimer::timeout, this, &TrackersModel::updateEtas);
        setTorrent(torrent);
    }

    TrackersModel::~TrackersModel() = default;

    int TrackersModel::columnCount(const QModelIndex&) const { return QMetaEnum::fromType<Column>().keyCount(); }

    QVariant TrackersModel::data(const QModelIndex& index, int role) const {
        //logDebug("data() called with: index = {}, role = {}", index, role);
        //logDebug("data: column = {}", static_cast<Column>(index.column()));
        const auto& tracker = mTrackers.at(static_cast<size_t>(index.row()));
        if (role == Qt::DisplayRole) {
            switch (static_cast<Column>(index.column())) {
            case Column::Announce:
                return tracker.tracker.announce();
            case Column::Status:
                return trackerStatusString(tracker.tracker);
            case Column::Error:
                return tracker.tracker.errorMessage();
            case Column::NextUpdate:
                if (tracker.nextUpdateEta.has_value()) {
                    return Utils::formatEta(static_cast<int>(tracker.nextUpdateEta->count()));
                }
                break;
            case Column::Peers:
                return tracker.tracker.peers();
            }
        } else if (role == SortRole) {
            if (static_cast<Column>(index.column()) == Column::NextUpdate && tracker.nextUpdateEta.has_value()) {
                return static_cast<qint64>(tracker.nextUpdateEta->count());
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
        case Column::Error:
            return qApp->translate("tremotesf", "Error");
        case Column::NextUpdate:
            return qApp->translate("tremotesf", "Next Update");
        case Column::Peers:
            return qApp->translate("tremotesf", "Peers");
        }
        return {};
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
            return mTrackers.at(static_cast<size_t>(index.row())).tracker.id();
        });
        return ids;
    }

    const libtremotesf::Tracker& TrackersModel::trackerAtIndex(const QModelIndex& index) const {
        return mTrackers.at(static_cast<size_t>(index.row())).tracker;
    }

    class TrackersModelUpdater : public ModelListUpdater<TrackersModel, TrackersModel::TrackerItem> {
    public:
        inline explicit TrackersModelUpdater(TrackersModel& model) : ModelListUpdater(model) {}

    protected:
        std::vector<TrackersModel::TrackerItem>::iterator findNewItemForItem(
            std::vector<TrackersModel::TrackerItem>& newItems, const TrackersModel::TrackerItem& item
        ) override {
            return std::find_if(newItems.begin(), newItems.end(), [item](const TrackersModel::TrackerItem& tracker) {
                return tracker.tracker.id() == item.tracker.id();
            });
        }

        bool updateItem(TrackersModel::TrackerItem& item, TrackersModel::TrackerItem&& newItem) override {
            if (newItem != item) {
                item = std::move(newItem);
                return true;
            }
            return false;
        }
    };

    void TrackersModel::update() {
        mEtaUpdateTimer->stop();
        TrackersModelUpdater updater(*this);
        const auto& trackers = mTorrent->trackers();
        updater.update(mTrackers, std::vector<TrackerItem>(trackers.begin(), trackers.end()));
        if (std::any_of(trackers.begin(), trackers.end(), [](const libtremotesf::Tracker& tracker) {
                return tracker.nextUpdateTime().isValid();
            })) {
            mEtaUpdateTimer->start();
        }
    }

    void TrackersModel::updateEtas() {
        TrackersModelUpdater updater(*this);
        std::vector<TrackerItem> newItems{};
        newItems.reserve(mTrackers.size());
        std::transform(
            mTrackers.begin(),
            mTrackers.end(),
            std::back_inserter(newItems),
            std::mem_fn(&TrackerItem::withUpdatedEta)
        );
        updater.update(mTrackers, std::move(newItems));
    }
}
