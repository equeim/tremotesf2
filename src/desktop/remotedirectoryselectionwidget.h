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

#ifndef TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H
#define TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H

#include "fileselectionwidget.h"

namespace tremotesf
{
    class Rpc;

    class RemoteDirectorySelectionWidget : public FileSelectionWidget
    {
    public:
        RemoteDirectorySelectionWidget(const QString& directory,
                                       const Rpc* rpc,
                                       bool comboBox,
                                       QWidget* parent = nullptr);
        void updateComboBox(const QString& setAsCurrent);

    private:
        const Rpc* mRpc;
    };
}

#endif // TREMOTESF_REMOTEDIRECTORYSELECTIONWIDGET_H
