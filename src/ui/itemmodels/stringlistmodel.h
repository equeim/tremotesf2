// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_STRINGLISTMODEL_H
#define TREMOTESF_STRINGLISTMODEL_H

#include <optional>
#include <vector>

#include <QAbstractListModel>
#include <QIcon>
#include <QString>

namespace tremotesf {
    class StringListModel final : public QAbstractListModel {
        Q_OBJECT

    public:
        explicit inline StringListModel(std::optional<QString> header, std::optional<QIcon> decoration, QObject* parent)
            : QAbstractListModel(parent), mHeader(std::move(header)), mDecoration(std::move(decoration)) {};

        inline QVariant data(const QModelIndex& index, int role) const override {
            if (!index.isValid()) return {};
            switch (role) {
            case Qt::DisplayRole:
                return mStringList.at(static_cast<size_t>(index.row()));
            case Qt::DecorationRole:
                if (mDecoration) {
                    return *mDecoration;
                }
                break;
            }
            return {};
        };
        inline QVariant headerData(int, Qt::Orientation orientation, int role) const override {
            return (orientation == Qt::Horizontal && role == Qt::DisplayRole && mHeader) ? *mHeader : QVariant{};
        };
        inline int rowCount(const QModelIndex& = {}) const override { return static_cast<int>(mStringList.size()); };

        void setStringList(const std::vector<QString>& stringList);

        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

    private:
        std::optional<QString> mHeader;
        std::optional<QIcon> mDecoration;
        std::vector<QString> mStringList;
    };
}

#endif // TREMOTESF_STRINGLISTMODEL_H
