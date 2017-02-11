/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_SETTINGSDIALOG_H
#define TREMOTESF_SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;

namespace tremotesf
{
    class SettingsDialog : public QDialog
    {
    public:
        explicit SettingsDialog(QWidget* parent = nullptr);
        void accept() override;

    private:
        QCheckBox* mConnectOnStartupCheckBox;
        QCheckBox* mNotificationOnDisconnectingCheckBox;
        QCheckBox* mNotificationOnAddingTorrentCheckBox;
        QCheckBox* mNotificationOfFinishedTorrentsCheckBox;
        QCheckBox* mTrayIconCheckBox;
        QCheckBox* mStartMinimizedCheckBox;
    };
}

#endif // TREMOTESF_SETTINGSDIALOG_H
