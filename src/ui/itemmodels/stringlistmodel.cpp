// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "stringlistmodel.h"

#include "ui/itemmodels/modelutils.h"

namespace tremotesf {
    void StringListModel::setStringList(const std::vector<QString>& stringList) {
        ModelListUpdater<StringListModel, QString> updater(*this);
        updater.update(mStringList, std::vector(stringList));
    }
}
