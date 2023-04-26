// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_STRINGLISTMODEL_H
#define TREMOTESF_STRINGLISTMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QString>

namespace tremotesf {
    class StringListModel final : public QAbstractListModel {
        Q_OBJECT

    public:
        explicit inline StringListModel(QString header = {}, QObject* parent = nullptr)
            : QAbstractListModel(parent), mHeader(std::move(header)){};

        inline QVariant data(const QModelIndex& index, int role) const override {
            return index.isValid() && role == Qt::DisplayRole ? mStringList.at(static_cast<size_t>(index.row()))
                                                              : QVariant{};
        };
        inline QVariant headerData(int, Qt::Orientation orientation, int role) const override {
            return orientation == Qt::Horizontal && role == Qt::DisplayRole ? mHeader : QVariant{};
        };
        inline int rowCount(const QModelIndex& = {}) const override { return static_cast<int>(mStringList.size()); };

        void setStringList(const std::vector<QString>& stringList);

        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

    private:
        QString mHeader;
        std::vector<QString> mStringList;
    };
}

#endif // TREMOTESF_STRINGLISTMODEL_H
