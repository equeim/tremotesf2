// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trackersmodel.h"

#include <chrono>
#include <optional>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QTimer>

#include "ui/itemmodels/modelutils.h"
#include "formatutils.h"

#include "stdutils.h"
#include "rpc/torrent.h"
#include "rpc/tracker.h"

namespace tremotesf {
    using std::chrono::seconds;
    using namespace std::chrono_literals;

    namespace {
        QString trackerStatusString(const Tracker& tracker) {
            switch (tracker.status()) {
            case Tracker::Status::Inactive:
                //: Tracker status
                return qApp->translate("tremotesf", "Inactive");
            case Tracker::Status::WaitingForUpdate:
                //: Tracker status
                return qApp->translate("tremotesf", "Waiting for update");
            case Tracker::Status::QueuedForUpdate:
                //: Tracker status
                return qApp->translate("tremotesf", "About to update");
            case Tracker::Status::Updating:
                //: Tracker status
                return qApp->translate("tremotesf", "Updating");
            }
            return {};
        }

        std::optional<seconds> nextUpdateEtaFor(const Tracker& tracker) {
            if (!tracker.nextUpdateTime().isValid()) return std::nullopt;
            const auto secs = QDateTime::currentDateTimeUtc().secsTo(tracker.nextUpdateTime());
            if (secs < 0) return std::nullopt;
            return seconds(secs);
        }
    }

    struct TrackersModel::TrackerItem {
        Tracker tracker;
        std::optional<seconds> nextUpdateEta{};

        TrackerItem(Tracker tracker) : tracker(std::move(tracker)), nextUpdateEta(nextUpdateEtaFor(this->tracker)) {}

        [[nodiscard]] bool operator==(const TrackerItem& other) const = default;

        [[nodiscard]] TrackerItem withUpdatedEta() const {
            TrackerItem updated = *this;
            updated.nextUpdateEta = nextUpdateEtaFor(tracker);
            return updated;
        }
    };

    TrackersModel::TrackersModel(Torrent* torrent, QObject* parent)
        : QAbstractTableModel(parent), mEtaUpdateTimer(new QTimer(this)) {
        mEtaUpdateTimer->setInterval(1s);
        mEtaUpdateTimer->setSingleShot(false);
        QObject::connect(mEtaUpdateTimer, &QTimer::timeout, this, &TrackersModel::updateEtas);
        setTorrent(torrent);
    }

    TrackersModel::~TrackersModel() = default;

    int TrackersModel::columnCount(const QModelIndex&) const { return QMetaEnum::fromType<Column>().keyCount(); }

    QVariant TrackersModel::data(const QModelIndex& index, int role) const {
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
                    return formatutils::formatEta(static_cast<int>(tracker.nextUpdateEta->count()));
                }
                break;
            case Column::Peers:
                return tracker.tracker.peers();
            case Column::Seeders:
                return tracker.tracker.seeders();
            case Column::Leechers:
                return tracker.tracker.leechers();
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
            //: Trackers list column title
            return qApp->translate("tremotesf", "Address");
        case Column::Status:
            //: Trackers list column title
            return qApp->translate("tremotesf", "Status");
        case Column::Error:
            //: Trackers list column title
            return qApp->translate("tremotesf", "Error");
        case Column::NextUpdate:
            //: Trackers list column title
            return qApp->translate("tremotesf", "Next Update");
        case Column::Peers:
            //: Trackers list column title
            return qApp->translate("tremotesf", "Peers");
        case Column::Seeders:
            //: Trackers list column title
            return qApp->translate("tremotesf", "Seeders");
        case Column::Leechers:
            //: Trackers list column title
            return qApp->translate("tremotesf", "Leechers");
        }
        return {};
    }

    int TrackersModel::rowCount(const QModelIndex&) const { return static_cast<int>(mTrackers.size()); }

    Torrent* TrackersModel::torrent() const { return mTorrent; }

    void TrackersModel::setTorrent(Torrent* torrent) {
        if (torrent != mTorrent) {
            if (const auto oldTorrent = mTorrent.data(); oldTorrent) {
                QObject::disconnect(oldTorrent, nullptr, this, nullptr);
            }

            mTorrent = torrent;

            if (mTorrent) {
                update();
                QObject::connect(mTorrent, &Torrent::updated, this, &TrackersModel::update);
            } else {
                beginResetModel();
                mTrackers.clear();
                endResetModel();
            }
        }
    }

    std::vector<int> TrackersModel::idsFromIndexes(const QModelIndexList& indexes) const {
        return createTransforming<std::vector<int>>(indexes, [this](const QModelIndex& index) {
            return mTrackers.at(static_cast<size_t>(index.row())).tracker.id();
        });
    }

    const Tracker& TrackersModel::trackerAtIndex(const QModelIndex& index) const {
        return mTrackers.at(static_cast<size_t>(index.row())).tracker;
    }

    class TrackersModelUpdater
        : public ModelListUpdater<TrackersModel, TrackersModel::TrackerItem, std::vector<TrackersModel::TrackerItem>> {
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
        const auto& trackers = mTorrent->data().trackers;
        updater.update(mTrackers, std::vector<TrackerItem>(trackers.begin(), trackers.end()));
        if (std::any_of(trackers.begin(), trackers.end(), [](const Tracker& tracker) {
                return tracker.nextUpdateTime().isValid();
            })) {
            mEtaUpdateTimer->start();
        }
    }

    void TrackersModel::updateEtas() {
        TrackersModelUpdater updater(*this);
        updater.update(
            mTrackers,
            createTransforming<std::vector<TrackerItem>>(mTrackers, std::mem_fn(&TrackerItem::withUpdatedEta))
        );
    }
}
