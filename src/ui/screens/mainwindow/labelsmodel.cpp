// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "labelsmodel.h"

#include <map>

#include <QCoreApplication>
#include <QIcon>

#include "rpc/rpc.h"
#include "rpc/serversettings.h"
#include "rpc/torrent.h"
#include "ui/itemmodels/modelutils.h"
#include "torrentsproxymodel.h"
#include "desktoputils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    QVariant LabelsModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const LabelItem& item = mLabels.at(static_cast<size_t>(index.row()));
        switch (role) {
        case LabelRole:
            return item.label;
        case Qt::DecorationRole: {
            if (item.label.isEmpty()) {
                return desktoputils::standardDirIcon();
            }
            static const auto tagIcon = QIcon::fromTheme("tag"_L1);
            return tagIcon;
        }
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (item.label.isEmpty()) {
                //: Filter option of torrents list's label filter. %L1 is total number of torrents
                return qApp->translate("tremotesf", "All (%L1)").arg(item.torrents);
            }
            //: Filter option of torrents list's label filter. %1 is label, %L2 is number of torrents with that label
            return qApp->translate("tremotesf", "%1 (%L2)").arg(item.label).arg(item.torrents);
        default:
            return {};
        }
    }

    int LabelsModel::rowCount(const QModelIndex&) const { return static_cast<int>(mLabels.size()); }

    QModelIndex LabelsModel::indexForTorrentsProxyModelFilter() const {
        if (!torrentsProxyModel()) {
            return {};
        }
        const auto filter = torrentsProxyModel()->labelFilter();
        for (size_t i = 0, max = mLabels.size(); i < max; ++i) {
            const auto& item = mLabels[i];
            if (item.label == filter) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

    void LabelsModel::resetTorrentsProxyModelFilter() const {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setLabelFilter({});
        }
    }

    class LabelsModelUpdater : public ModelListUpdater<LabelsModel, LabelsModel::LabelItem, std::map<QString, int>> {
    public:
        inline explicit LabelsModelUpdater(LabelsModel& model) : ModelListUpdater(model) {}

    protected:
        std::map<QString, int>::iterator
        findNewItemForItem(std::map<QString, int>& newLabels, const LabelsModel::LabelItem& item) override {
            return newLabels.find(item.label);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        bool updateItem(LabelsModel::LabelItem& item, std::pair<const QString, int>&& newItem) override {
            const auto& [label, torrents] = newItem;
            if (item.torrents != torrents) {
                item.torrents = torrents;
                return true;
            }
            return false;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        LabelsModel::LabelItem createItemFromNewItem(std::pair<const QString, int>&& newItem) override {
            return LabelsModel::LabelItem{.label = newItem.first, .torrents = newItem.second};
        }
    };

    void LabelsModel::update() {
        std::map<QString, int> labels{};
        const bool serverSupportsLabels =
            rpc()->isConnected() ? rpc()->serverSettings()->data().hasLabelsProperty() : true;
        if (serverSupportsLabels) {
            labels.emplace(QString(), rpc()->torrentsCount());
            for (const auto& torrent : rpc()->torrents()) {
                for (const QString& label : torrent->data().labels) {
                    auto found = labels.find(label);
                    if (found == labels.end()) {
                        labels.emplace(label, 1);
                    } else {
                        ++(found->second);
                    }
                }
            }
        }
        LabelsModelUpdater updater(*this);
        updater.update(mLabels, std::move(labels));
    }
}
