// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H
#define TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H

#include "fileselectionwidget.h"

namespace tremotesf {
    class Rpc;

    class RemoteDirectorySelectionWidget : public FileSelectionWidget {
        Q_OBJECT

    public:
        RemoteDirectorySelectionWidget(
            const QString& directory, const Rpc* rpc, bool comboBox, QWidget* parent = nullptr
        );
        void updateComboBox(const QString& setAsCurrent);

    private:
        const Rpc* mRpc;
    };
}

#endif // TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H
