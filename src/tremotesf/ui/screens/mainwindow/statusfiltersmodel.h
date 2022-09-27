#ifndef TREMOTESF_STATUSFILTERSMODEL_H
#define TREMOTESF_STATUSFILTERSMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf
{
    class StatusFiltersModel : public BaseTorrentsFiltersSettingsModel
    {
        Q_OBJECT
    public:
        static constexpr auto FilterRole = Qt::UserRole;

        inline explicit StatusFiltersModel(QObject* parent = nullptr) : BaseTorrentsFiltersSettingsModel(parent) {};

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        QModelIndex indexForStatusFilter(tremotesf::TorrentsProxyModel::StatusFilter filter) const;
        QModelIndex indexForTorrentsProxyModelFilter() const override;

    protected:
        void resetTorrentsProxyModelFilter() const override;

    private:
        void update() override;

        struct Item
        {
            TorrentsProxyModel::StatusFilter filter;
            int torrents;
        };

        std::vector<Item> mItems;

        template<typename, typename, typename, typename> friend class ModelListUpdater;
        friend class StatusFiltersModelUpdater;
    };
}

#endif // TREMOTESF_STATUSFILTERSMODEL_H
